#include "CacheStrategyManager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QDateTime>
#include <QMutexLocker>
#include <algorithm>
#include <cmath>

Q_LOGGING_CATEGORY(cacheStrategyManager, "qkchat.server.cachestrategymanager")

CacheStrategyManager::CacheStrategyManager(MultiLevelCache* cache, QObject *parent)
    : QObject(parent)
    , m_cache(cache)
    , m_optimizationTimer(new QTimer(this))
    , m_analysisTimer(new QTimer(this))
    , m_cleanupTimer(new QTimer(this))
{
    connect(m_optimizationTimer, &QTimer::timeout, this, &CacheStrategyManager::performAdaptiveOptimization);
    connect(m_analysisTimer, &QTimer::timeout, this, &CacheStrategyManager::analyzePeriodically);
    connect(m_cleanupTimer, &QTimer::timeout, this, &CacheStrategyManager::cleanupOldData);
    
    qCInfo(cacheStrategyManager) << "CacheStrategyManager created";
}

CacheStrategyManager::~CacheStrategyManager()
{
    shutdown();
    qCInfo(cacheStrategyManager) << "CacheStrategyManager destroyed";
}

bool CacheStrategyManager::initialize(const StrategyConfig& config)
{
    qCInfo(cacheStrategyManager) << "Initializing CacheStrategyManager...";
    
    if (!m_cache) {
        qCCritical(cacheStrategyManager) << "Cache is null";
        return false;
    }
    
    m_config = config;
    
    // 启动定时器
    if (config.enableAdaptive) {
        m_optimizationTimer->start(config.adaptiveInterval * 1000);
    }
    
    m_analysisTimer->start(60000); // 每分钟分析一次
    m_cleanupTimer->start(300000); // 每5分钟清理一次
    
    m_enabled.storeRelease(1);
    
    qCInfo(cacheStrategyManager) << "CacheStrategyManager initialized successfully";
    return true;
}

void CacheStrategyManager::shutdown()
{
    if (m_enabled.testAndSetOrdered(1, 0)) {
        qCInfo(cacheStrategyManager) << "Shutting down CacheStrategyManager...";
        
        m_optimizationTimer->stop();
        m_analysisTimer->stop();
        m_cleanupTimer->stop();
        
        qCInfo(cacheStrategyManager) << "CacheStrategyManager shutdown complete";
    }
}

bool CacheStrategyManager::isEnabled() const
{
    return m_enabled.loadAcquire() > 0;
}

void CacheStrategyManager::recordAccess(const QString& key, const QString& category, 
                                       CacheLevel level, bool hit, qint64 latency)
{
    if (!isEnabled()) {
        return;
    }
    
    AccessStats stats;
    stats.timestamp = QDateTime::currentDateTime();
    stats.key = key;
    stats.category = category;
    stats.level = level;
    stats.hit = hit;
    stats.latency = latency;
    stats.frequency = 1;
    
    QMutexLocker locker(&m_historyMutex);
    
    // 添加到总历史
    m_accessHistory.append(stats);
    if (m_accessHistory.size() > m_config.historyWindow) {
        m_accessHistory.removeFirst();
    }
    
    // 添加到键历史
    m_keyHistory[key].append(stats);
    if (m_keyHistory[key].size() > 100) { // 每个键最多保留100条记录
        m_keyHistory[key].removeFirst();
    }
    
    // 添加到分类历史
    if (!category.isEmpty()) {
        m_categoryHistory[category].append(stats);
        if (m_categoryHistory[category].size() > 500) { // 每个分类最多保留500条记录
            m_categoryHistory[category].removeFirst();
        }
    }
    
    // 更新性能指标
    m_metrics.totalRequests++;
    if (hit) {
        switch (level) {
        case CacheLevel::L1_Memory: m_metrics.l1HitRate++; break;
        case CacheLevel::L2_Local: m_metrics.l2HitRate++; break;
        case CacheLevel::L3_Distributed: m_metrics.l3HitRate++; break;
        }
    }
    
    m_metrics.averageLatency = (m_metrics.averageLatency + latency) / 2;
    if (latency > m_metrics.maxLatency) {
        m_metrics.maxLatency = latency;
    }
    
    m_metrics.lastUpdate = QDateTime::currentDateTime();
}

QStringList CacheStrategyManager::predictNextAccess(int count) const
{
    if (!isEnabled()) {
        return QStringList();
    }
    
    switch (m_config.model) {
    case PredictionModel::LRU_K:
        return predictLRU_K(QString(), 2, count);
    case PredictionModel::ARC:
        return predictARC(QString(), count);
    case PredictionModel::LIRS:
        return predictLIRS(QString(), count);
    case PredictionModel::ML_Based:
        return predictML(QString(), count);
    default:
        return predictLRU_K(QString(), 2, count);
    }
}

QStringList CacheStrategyManager::recommendPrefetch(const QString& key, int count) const
{
    if (!isEnabled()) {
        return QStringList();
    }
    
    QMutexLocker locker(&m_historyMutex);
    
    // 查找与该键相关的访问模式
    QStringList recommendations;
    
    if (m_keyHistory.contains(key)) {
        const auto& history = m_keyHistory[key];
        
        // 分析顺序访问模式
        QStringList sequentialKeys = extractSequentialKeys(history);
        recommendations.append(sequentialKeys);
        
        // 分析时间相关性
        for (const auto& stats : history) {
            // 查找在相似时间访问的其他键
            for (auto it = m_keyHistory.begin(); it != m_keyHistory.end(); ++it) {
                if (it.key() != key && !recommendations.contains(it.key())) {
                    // 检查时间相关性
                    for (const auto& otherStats : it.value()) {
                        if (qAbs(stats.timestamp.msecsTo(otherStats.timestamp)) < 60000) { // 1分钟内
                            recommendations.append(it.key());
                            break;
                        }
                    }
                }
            }
        }
    }
    
    // 限制返回数量
    if (recommendations.size() > count) {
        recommendations = recommendations.mid(0, count);
    }
    
    return recommendations;
}

CacheStrategy CacheStrategyManager::recommendStrategy(const QString& category) const
{
    if (!isEnabled()) {
        return CacheStrategy::LRU;
    }
    
    QMutexLocker locker(&m_historyMutex);
    
    QList<AccessStats> history;
    if (category.isEmpty()) {
        history = m_accessHistory;
    } else if (m_categoryHistory.contains(category)) {
        history = m_categoryHistory[category];
    }
    
    if (history.isEmpty()) {
        return CacheStrategy::LRU;
    }
    
    AccessPattern pattern = detectAccessPattern(history);
    
    switch (pattern) {
    case AccessPattern::Sequential:
        return CacheStrategy::LRU;
    case AccessPattern::Random:
        return CacheStrategy::ARC;
    case AccessPattern::Temporal:
        return CacheStrategy::LRU;
    case AccessPattern::Burst:
        return CacheStrategy::LFU;
    case AccessPattern::Periodic:
        return CacheStrategy::CLOCK;
    default:
        return CacheStrategy::LRU;
    }
}

void CacheStrategyManager::enableAdaptiveOptimization(bool enabled)
{
    if (enabled && m_config.enableAdaptive) {
        m_optimizationTimer->start(m_config.adaptiveInterval * 1000);
    } else {
        m_optimizationTimer->stop();
    }
    
    logStrategyEvent("ADAPTIVE_OPTIMIZATION", enabled ? "ENABLED" : "DISABLED");
}

void CacheStrategyManager::triggerOptimization()
{
    if (!isEnabled()) {
        return;
    }
    
    performAdaptiveOptimization();
}

void CacheStrategyManager::analyzeAccessPatterns()
{
    if (!isEnabled()) {
        return;
    }
    
    QMutexLocker locker(&m_historyMutex);
    
    // 分析整体访问模式
    AccessPattern overallPattern = detectAccessPattern(m_accessHistory);
    
    // 分析各分类的访问模式
    for (auto it = m_categoryHistory.begin(); it != m_categoryHistory.end(); ++it) {
        AccessPattern categoryPattern = detectAccessPattern(it.value());
        
        if (categoryPattern != overallPattern) {
            logStrategyEvent("PATTERN_DETECTED", 
                           QString("Category: %1, Pattern: %2").arg(it.key()).arg(static_cast<int>(categoryPattern)));
        }
    }
    
    // 更新预测模型
    if (m_config.model == PredictionModel::ML_Based) {
        trainMLModel();
    }
}

CacheStrategyManager::PerformanceMetrics CacheStrategyManager::getPerformanceMetrics() const
{
    return m_metrics;
}

QJsonObject CacheStrategyManager::getDetailedAnalysis() const
{
    QJsonObject analysis;
    
    // 基础指标
    analysis["total_requests"] = m_metrics.totalRequests;
    analysis["hit_rate"] = m_metrics.hitRate;
    analysis["average_latency"] = m_metrics.averageLatency;
    analysis["max_latency"] = m_metrics.maxLatency;
    
    // 访问模式分析
    QMutexLocker locker(&m_historyMutex);
    AccessPattern pattern = detectAccessPattern(m_accessHistory);
    analysis["access_pattern"] = static_cast<int>(pattern);
    
    // 热点键分析
    QJsonArray hotKeys;
    QStringList topKeys = identifyHotKeys();
    for (const QString& key : topKeys) {
        hotKeys.append(key);
    }
    analysis["hot_keys"] = hotKeys;
    
    // 分类分析
    QJsonObject categories;
    for (auto it = m_categoryHistory.begin(); it != m_categoryHistory.end(); ++it) {
        QJsonObject categoryInfo;
        categoryInfo["access_count"] = it.value().size();
        categoryInfo["pattern"] = static_cast<int>(detectAccessPattern(it.value()));
        categories[it.key()] = categoryInfo;
    }
    analysis["categories"] = categories;
    
    return analysis;
}

void CacheStrategyManager::performAdaptiveOptimization()
{
    if (!isEnabled()) {
        return;
    }
    
    qCDebug(cacheStrategyManager) << "Performing adaptive optimization...";
    
    analyzeAccessPatterns();
    
    // 生成优化建议
    QStringList optimizations = generateCacheOptimizations();
    QStringList prefetchOptimizations = generatePrefetchOptimizations();
    QStringList strategyOptimizations = generateStrategyOptimizations();
    
    QJsonObject results;
    results["cache_optimizations"] = QJsonArray::fromStringList(optimizations);
    results["prefetch_optimizations"] = QJsonArray::fromStringList(prefetchOptimizations);
    results["strategy_optimizations"] = QJsonArray::fromStringList(strategyOptimizations);
    results["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    emit optimizationCompleted(results);
    
    qCDebug(cacheStrategyManager) << "Adaptive optimization completed";
}

void CacheStrategyManager::analyzePeriodically()
{
    analyzeAccessPatterns();
    
    // 检查性能警报
    if (m_metrics.hitRate < 0.5) {
        emit performanceAlert("Low cache hit rate detected");
    }
    
    if (m_metrics.averageLatency > 10000) { // 10ms
        emit performanceAlert("High cache latency detected");
    }
}

void CacheStrategyManager::cleanupOldData()
{
    QMutexLocker locker(&m_historyMutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-3600); // 1小时前
    
    // 清理访问历史
    m_accessHistory.erase(
        std::remove_if(m_accessHistory.begin(), m_accessHistory.end(),
                      [cutoff](const AccessStats& stats) {
                          return stats.timestamp < cutoff;
                      }),
        m_accessHistory.end());
    
    // 清理键历史
    for (auto it = m_keyHistory.begin(); it != m_keyHistory.end();) {
        it.value().erase(
            std::remove_if(it.value().begin(), it.value().end(),
                          [cutoff](const AccessStats& stats) {
                              return stats.timestamp < cutoff;
                          }),
            it.value().end());
        
        if (it.value().isEmpty()) {
            it = m_keyHistory.erase(it);
        } else {
            ++it;
        }
    }
    
    // 清理分类历史
    for (auto it = m_categoryHistory.begin(); it != m_categoryHistory.end();) {
        it.value().erase(
            std::remove_if(it.value().begin(), it.value().end(),
                          [cutoff](const AccessStats& stats) {
                              return stats.timestamp < cutoff;
                          }),
            it.value().end());
        
        if (it.value().isEmpty()) {
            it = m_categoryHistory.erase(it);
        } else {
            ++it;
        }
    }
}

AccessPattern CacheStrategyManager::detectAccessPattern(const QList<AccessStats>& history) const
{
    if (history.size() < 10) {
        return AccessPattern::Random;
    }
    
    if (isSequentialPattern(history)) {
        return AccessPattern::Sequential;
    }
    
    if (isTemporalPattern(history)) {
        return AccessPattern::Temporal;
    }
    
    if (isBurstPattern(history)) {
        return AccessPattern::Burst;
    }
    
    if (isPeriodicPattern(history)) {
        return AccessPattern::Periodic;
    }
    
    return AccessPattern::Random;
}

bool CacheStrategyManager::isSequentialPattern(const QList<AccessStats>& history) const
{
    // 简化的顺序模式检测
    int sequentialCount = 0;
    for (int i = 1; i < history.size(); ++i) {
        QString prevKey = history[i-1].key;
        QString currKey = history[i].key;
        
        // 检查键是否有数字后缀且递增
        QRegularExpression re("(\\w+)(\\d+)");
        auto prevMatch = re.match(prevKey);
        auto currMatch = re.match(currKey);
        
        if (prevMatch.hasMatch() && currMatch.hasMatch()) {
            if (prevMatch.captured(1) == currMatch.captured(1)) {
                int prevNum = prevMatch.captured(2).toInt();
                int currNum = currMatch.captured(2).toInt();
                if (currNum == prevNum + 1) {
                    sequentialCount++;
                }
            }
        }
    }
    
    return sequentialCount > history.size() * 0.3; // 30%以上是顺序的
}

bool CacheStrategyManager::isTemporalPattern(const QList<AccessStats>& history) const
{
    // 检查时间局部性
    QHash<QString, QDateTime> lastAccess;
    int temporalCount = 0;
    
    for (const auto& stats : history) {
        if (lastAccess.contains(stats.key)) {
            qint64 timeDiff = lastAccess[stats.key].msecsTo(stats.timestamp);
            if (timeDiff < 60000) { // 1分钟内重复访问
                temporalCount++;
            }
        }
        lastAccess[stats.key] = stats.timestamp;
    }
    
    return temporalCount > history.size() * 0.2; // 20%以上有时间局部性
}

bool CacheStrategyManager::isBurstPattern(const QList<AccessStats>& history) const
{
    // 检查突发访问模式
    QHash<QString, int> keyCount;
    for (const auto& stats : history) {
        keyCount[stats.key]++;
    }
    
    int burstKeys = 0;
    for (auto it = keyCount.begin(); it != keyCount.end(); ++it) {
        if (it.value() > history.size() * 0.1) { // 某个键占总访问的10%以上
            burstKeys++;
        }
    }
    
    return burstKeys > 0 && burstKeys < keyCount.size() * 0.3; // 少数键占大部分访问
}

bool CacheStrategyManager::isPeriodicPattern(const QList<AccessStats>& history) const
{
    // 简化的周期性检测
    // 实际实现需要更复杂的时间序列分析
    return false;
}

QStringList CacheStrategyManager::predictLRU_K(const QString& key, int k, int count) const
{
    // LRU-K预测的简化实现
    QStringList predictions;
    
    QMutexLocker locker(&m_historyMutex);
    
    // 获取最近访问的键
    QHash<QString, QDateTime> lastAccess;
    for (const auto& stats : m_accessHistory) {
        lastAccess[stats.key] = stats.timestamp;
    }
    
    // 按最后访问时间排序
    QList<QPair<QDateTime, QString>> sortedKeys;
    for (auto it = lastAccess.begin(); it != lastAccess.end(); ++it) {
        sortedKeys.append({it.value(), it.key()});
    }
    
    std::sort(sortedKeys.begin(), sortedKeys.end(), 
             [](const auto& a, const auto& b) { return a.first > b.first; });
    
    for (int i = 0; i < qMin(count, sortedKeys.size()); ++i) {
        predictions.append(sortedKeys[i].second);
    }
    
    return predictions;
}

QStringList CacheStrategyManager::predictARC(const QString& key, int count) const
{
    // ARC预测的简化实现
    return predictLRU_K(key, 1, count);
}

QStringList CacheStrategyManager::predictLIRS(const QString& key, int count) const
{
    // LIRS预测的简化实现
    return predictLRU_K(key, 2, count);
}

QStringList CacheStrategyManager::predictML(const QString& key, int count) const
{
    // 机器学习预测的简化实现
    return predictLRU_K(key, 1, count);
}

void CacheStrategyManager::trainMLModel()
{
    // 机器学习模型训练的简化实现
    qCDebug(cacheStrategyManager) << "Training ML model...";
}

QStringList CacheStrategyManager::generateCacheOptimizations() const
{
    QStringList optimizations;
    
    if (m_metrics.hitRate < 0.7) {
        optimizations.append("Increase cache size to improve hit rate");
    }
    
    if (m_metrics.averageLatency > 5000) {
        optimizations.append("Consider using faster storage for L2 cache");
    }
    
    return optimizations;
}

QStringList CacheStrategyManager::generatePrefetchOptimizations() const
{
    QStringList optimizations;
    
    QMutexLocker locker(&m_historyMutex);
    
    if (isSequentialPattern(m_accessHistory)) {
        optimizations.append("Enable sequential prefetching");
    }
    
    if (isTemporalPattern(m_accessHistory)) {
        optimizations.append("Increase prefetch window for temporal locality");
    }
    
    return optimizations;
}

QStringList CacheStrategyManager::generateStrategyOptimizations() const
{
    QStringList optimizations;
    
    AccessPattern pattern = detectAccessPattern(m_accessHistory);
    
    switch (pattern) {
    case AccessPattern::Sequential:
        optimizations.append("Use LRU strategy for sequential access pattern");
        break;
    case AccessPattern::Burst:
        optimizations.append("Use LFU strategy for burst access pattern");
        break;
    case AccessPattern::Random:
        optimizations.append("Use ARC strategy for random access pattern");
        break;
    default:
        break;
    }
    
    return optimizations;
}

QStringList CacheStrategyManager::extractSequentialKeys(const QList<AccessStats>& history) const
{
    QStringList keys;
    
    for (const auto& stats : history) {
        if (!keys.contains(stats.key)) {
            keys.append(stats.key);
        }
    }
    
    return keys;
}

QStringList CacheStrategyManager::identifyHotKeys() const
{
    QHash<QString, int> keyFrequency;
    
    QMutexLocker locker(&m_historyMutex);
    for (const auto& stats : m_accessHistory) {
        keyFrequency[stats.key]++;
    }
    
    QList<QPair<int, QString>> sortedKeys;
    for (auto it = keyFrequency.begin(); it != keyFrequency.end(); ++it) {
        sortedKeys.append({it.value(), it.key()});
    }
    
    std::sort(sortedKeys.begin(), sortedKeys.end(), 
             [](const auto& a, const auto& b) { return a.first > b.first; });
    
    QStringList hotKeys;
    for (int i = 0; i < qMin(10, sortedKeys.size()); ++i) {
        hotKeys.append(sortedKeys[i].second);
    }
    
    return hotKeys;
}

void CacheStrategyManager::logStrategyEvent(const QString& event, const QString& details) const
{
    if (details.isEmpty()) {
        qCDebug(cacheStrategyManager) << event;
    } else {
        qCDebug(cacheStrategyManager) << event << ":" << details;
    }
}
