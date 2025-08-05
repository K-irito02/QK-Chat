#include "CacheManagerV2.h"
#include <QDebug>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QMutexLocker>

Q_LOGGING_CATEGORY(cacheManagerV2, "qkchat.server.cachemanagerv2")

CacheManagerV2* CacheManagerV2::instance()
{
    static CacheManagerV2* instance = nullptr;
    static QMutex mutex;
    
    if (!instance) {
        QMutexLocker locker(&mutex);
        if (!instance) {
            instance = new CacheManagerV2();
        }
    }
    
    return instance;
}

CacheManagerV2::CacheManagerV2(QObject *parent)
    : QObject(parent)
    , m_cleanupTimer(new QTimer(this))
{
    connect(m_cleanupTimer, &QTimer::timeout, this, &CacheManagerV2::performCleanup);
    qCInfo(cacheManagerV2) << "CacheManagerV2 created";
}

CacheManagerV2::~CacheManagerV2()
{
    shutdown();
    qCInfo(cacheManagerV2) << "CacheManagerV2 destroyed";
}

bool CacheManagerV2::initialize(const CacheConfig& config)
{
    qCInfo(cacheManagerV2) << "Initializing CacheManagerV2...";
    
    m_config = config;
    
    try {
        // 初始化多级缓存
        m_multiLevelCache = std::make_unique<MultiLevelCache>(this);
        if (!m_multiLevelCache->initialize(config.multiLevelConfig)) {
            qCCritical(cacheManagerV2) << "Failed to initialize MultiLevelCache";
            return false;
        }
        
        // 初始化策略管理器
        if (config.enableAdvancedFeatures) {
            m_strategyManager = std::make_unique<CacheStrategyManager>(m_multiLevelCache.get(), this);
            if (!m_strategyManager->initialize(config.strategyConfig)) {
                qCWarning(cacheManagerV2) << "Failed to initialize CacheStrategyManager";
            }
        }
        
        // 初始化预加载器
        if (config.enableAdvancedFeatures) {
            m_preloader = std::make_unique<CachePreloader>(m_multiLevelCache.get(), this);
            if (!m_preloader->initialize(config.preloaderConfig)) {
                qCWarning(cacheManagerV2) << "Failed to initialize CachePreloader";
            }
        }
        
        // 设置连接
        setupConnections();
        
        // 启动清理定时器
        if (config.enableLegacyAPI) {
            m_cleanupTimer->start(300000); // 5分钟
        }
        
        m_initialized.storeRelease(1);
        
        qCInfo(cacheManagerV2) << "CacheManagerV2 initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        qCCritical(cacheManagerV2) << "Failed to initialize CacheManagerV2:" << e.what();
        return false;
    }
}

void CacheManagerV2::shutdown()
{
    if (m_initialized.testAndSetOrdered(1, 0)) {
        qCInfo(cacheManagerV2) << "Shutting down CacheManagerV2...";
        
        m_cleanupTimer->stop();
        
        if (m_preloader) {
            m_preloader->shutdown();
            m_preloader.reset();
        }
        
        if (m_strategyManager) {
            m_strategyManager->shutdown();
            m_strategyManager.reset();
        }
        
        if (m_multiLevelCache) {
            m_multiLevelCache->shutdown();
            m_multiLevelCache.reset();
        }
        
        qCInfo(cacheManagerV2) << "CacheManagerV2 shutdown complete";
    }
}

bool CacheManagerV2::isInitialized() const
{
    return m_initialized.loadAcquire() > 0;
}

// === 兼容的基本API实现 ===

bool CacheManagerV2::set(const QString &key, const QVariant &value, int ttlSeconds, const QString &category)
{
    if (!isInitialized() || !m_multiLevelCache) {
        return false;
    }
    
    bool success = m_multiLevelCache->set(key, value, ttlSeconds, category);
    if (success) {
        updateLegacyStats(false); // 这是一个写操作，不算命中
        if (!category.isEmpty()) {
            addToCategory(category, key);
        }
        emit itemAdded(key, category);
    }
    
    return success;
}

QVariant CacheManagerV2::get(const QString &key, const QVariant &defaultValue)
{
    if (!isInitialized() || !m_multiLevelCache) {
        return defaultValue;
    }
    
    auto result = m_multiLevelCache->get<QVariant>(key);
    bool hit = result.has_value();
    updateLegacyStats(hit);
    
    return hit ? result.value() : defaultValue;
}

bool CacheManagerV2::remove(const QString &key)
{
    if (!isInitialized() || !m_multiLevelCache) {
        return false;
    }
    
    bool success = m_multiLevelCache->remove(key);
    if (success) {
        // 从所有分类中移除
        QMutexLocker locker(&m_categoriesMutex);
        for (auto it = m_categories.begin(); it != m_categories.end(); ++it) {
            it.value().removeAll(key);
        }
        emit itemRemoved(key, QString());
    }
    
    return success;
}

bool CacheManagerV2::exists(const QString &key)
{
    if (!isInitialized() || !m_multiLevelCache) {
        return false;
    }
    
    return m_multiLevelCache->exists(key);
}

void CacheManagerV2::clear()
{
    if (!isInitialized() || !m_multiLevelCache) {
        return;
    }
    
    m_multiLevelCache->clear();
    
    {
        QMutexLocker locker(&m_categoriesMutex);
        m_categories.clear();
    }
    
    m_legacyStats = CacheStats();
    emit cacheCleared();
}

void CacheManagerV2::clearCategory(const QString &category)
{
    if (!isInitialized() || !m_multiLevelCache) {
        return;
    }
    
    QStringList keys;
    {
        QMutexLocker locker(&m_categoriesMutex);
        keys = m_categories.value(category);
        m_categories.remove(category);
    }
    
    for (const QString& key : keys) {
        m_multiLevelCache->remove(key);
    }
    
    emit categoryCleared(category);
}

bool CacheManagerV2::setMultiple(const QHash<QString, QVariant> &items, int ttlSeconds, const QString &category)
{
    if (!isInitialized() || !m_multiLevelCache) {
        return false;
    }
    
    bool allSuccess = true;
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (!set(it.key(), it.value(), ttlSeconds, category)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

QHash<QString, QVariant> CacheManagerV2::getMultiple(const QStringList &keys)
{
    QHash<QString, QVariant> result;
    for (const QString& key : keys) {
        QVariant value = get(key);
        if (value.isValid()) {
            result[key] = value;
        }
    }
    return result;
}

bool CacheManagerV2::removeMultiple(const QStringList &keys)
{
    bool allSuccess = true;
    for (const QString& key : keys) {
        if (!remove(key)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

// === 分类缓存实现 ===

bool CacheManagerV2::setInCategory(const QString &category, const QString &key, const QVariant &value, int ttlSeconds)
{
    QString fullKey = category + ":" + key;
    return set(fullKey, value, ttlSeconds, category);
}

QVariant CacheManagerV2::getFromCategory(const QString &category, const QString &key, const QVariant &defaultValue)
{
    QString fullKey = category + ":" + key;
    return get(fullKey, defaultValue);
}

QStringList CacheManagerV2::getCategoryKeys(const QString &category)
{
    QMutexLocker locker(&m_categoriesMutex);
    return m_categories.value(category);
}

QHash<QString, QVariant> CacheManagerV2::getCategoryData(const QString &category)
{
    QStringList keys = getCategoryKeys(category);
    QHash<QString, QVariant> result;
    
    for (const QString& key : keys) {
        QVariant value = get(key);
        if (value.isValid()) {
            result[key] = value;
        }
    }
    
    return result;
}

// === 用户相关缓存实现 ===

bool CacheManagerV2::cacheUserInfo(qint64 userId, const QVariantMap &userInfo, int ttlSeconds)
{
    QString key = generateUserKey(userId);
    return set(key, QVariant(userInfo), ttlSeconds, "users");
}

QVariantMap CacheManagerV2::getUserInfo(qint64 userId)
{
    QString key = generateUserKey(userId);
    QVariant value = get(key);
    return value.toMap();
}

void CacheManagerV2::invalidateUserCache(qint64 userId)
{
    QString key = generateUserKey(userId);
    remove(key);
}

// === 群组相关缓存实现 ===

bool CacheManagerV2::cacheGroupInfo(qint64 groupId, const QVariantMap &groupInfo, int ttlSeconds)
{
    QString key = generateGroupKey(groupId);
    return set(key, QVariant(groupInfo), ttlSeconds, "groups");
}

QVariantMap CacheManagerV2::getGroupInfo(qint64 groupId)
{
    QString key = generateGroupKey(groupId);
    QVariant value = get(key);
    return value.toMap();
}

void CacheManagerV2::invalidateGroupCache(qint64 groupId)
{
    QString key = generateGroupKey(groupId);
    remove(key);
}

bool CacheManagerV2::cacheGroupMembers(qint64 groupId, const QVariantList &members, int ttlSeconds)
{
    QString key = generateGroupKey(groupId) + ":members";
    return set(key, QVariant(members), ttlSeconds, "group_members");
}

QVariantList CacheManagerV2::getGroupMembers(qint64 groupId)
{
    QString key = generateGroupKey(groupId) + ":members";
    QVariant value = get(key);
    return value.toList();
}

// === 消息相关缓存实现 ===

bool CacheManagerV2::cacheRecentMessages(qint64 chatId, const QVariantList &messages, int ttlSeconds)
{
    QString key = generateMessageKey(chatId);
    return set(key, QVariant(messages), ttlSeconds, "messages");
}

QVariantList CacheManagerV2::getRecentMessages(qint64 chatId)
{
    QString key = generateMessageKey(chatId);
    QVariant value = get(key);
    return value.toList();
}

void CacheManagerV2::invalidateMessageCache(qint64 chatId)
{
    QString key = generateMessageKey(chatId);
    remove(key);
}

// === 会话相关缓存实现 ===

bool CacheManagerV2::cacheUserSession(const QString &sessionToken, qint64 userId, int ttlSeconds)
{
    QString key = generateSessionKey(sessionToken);
    return set(key, QVariant(userId), ttlSeconds, "sessions");
}

qint64 CacheManagerV2::getUserFromSession(const QString &sessionToken)
{
    QString key = generateSessionKey(sessionToken);
    QVariant value = get(key);
    return value.toLongLong();
}

void CacheManagerV2::invalidateSession(const QString &sessionToken)
{
    QString key = generateSessionKey(sessionToken);
    remove(key);
}

// === 数据库查询缓存实现 ===

bool CacheManagerV2::cacheQuery(const QString &sql, const QVariantMap &parameters, const QVariantList &result, int ttlSeconds)
{
    QString key = generateQueryCacheKey(sql, parameters);
    return set(key, QVariant(result), ttlSeconds, "queries");
}

QVariantList CacheManagerV2::getCachedQuery(const QString &sql, const QVariantMap &parameters)
{
    QString key = generateQueryCacheKey(sql, parameters);
    QVariant value = get(key);
    return value.toList();
}

// === 兼容的统计和配置API实现 ===

CacheManagerV2::CacheStats CacheManagerV2::getStats() const
{
    return m_legacyStats;
}

double CacheManagerV2::getHitRate() const
{
    return m_legacyStats.hitRate;
}

qint64 CacheManagerV2::getTotalSize() const
{
    if (m_multiLevelCache) {
        auto stats = m_multiLevelCache->getStatistics();
        return stats.l1Size.loadAcquire() + stats.l2Size.loadAcquire() + stats.l3Size.loadAcquire();
    }
    return 0;
}

int CacheManagerV2::getItemCount() const
{
    if (m_multiLevelCache) {
        auto stats = m_multiLevelCache->getStatistics();
        return stats.l1Count.loadAcquire() + stats.l2Count.loadAcquire() + stats.l3Count.loadAcquire();
    }
    return 0;
}

QStringList CacheManagerV2::getKeys() const
{
    // 这个方法在多级缓存中比较复杂，暂时返回空列表
    return QStringList();
}

QStringList CacheManagerV2::getCategories() const
{
    QMutexLocker locker(&m_categoriesMutex);
    return m_categories.keys();
}

void CacheManagerV2::setDefaultPolicy(CachePolicy policy)
{
    // 将兼容的策略映射到新的策略系统
    if (m_strategyManager) {
        CacheStrategy newStrategy;
        switch (policy) {
        case LRU: newStrategy = CacheStrategy::LRU; break;
        case LFU: newStrategy = CacheStrategy::LFU; break;
        default: newStrategy = CacheStrategy::LRU; break;
        }
        // 这里可以调用策略管理器的方法
    }
}

void CacheManagerV2::setMaxSize(qint64 maxSizeBytes)
{
    if (m_multiLevelCache) {
        auto config = m_multiLevelCache->getCurrentConfig();
        config.l1MaxSize = maxSizeBytes * 0.3; // 30% L1
        config.l2MaxSize = maxSizeBytes * 0.7; // 70% L2
        m_multiLevelCache->updateConfig(config);
    }
}

void CacheManagerV2::setMaxItems(int maxItems)
{
    if (m_multiLevelCache) {
        auto config = m_multiLevelCache->getCurrentConfig();
        config.l1MaxItems = maxItems * 0.3; // 30% L1
        config.l2MaxItems = maxItems * 0.7; // 70% L2
        m_multiLevelCache->updateConfig(config);
    }
}

void CacheManagerV2::setDefaultTTL(int seconds)
{
    m_defaultTTL.storeRelease(seconds);
    if (m_multiLevelCache) {
        auto config = m_multiLevelCache->getCurrentConfig();
        config.defaultTTL = seconds;
        m_multiLevelCache->updateConfig(config);
    }
}

void CacheManagerV2::setCleanupInterval(int seconds)
{
    if (m_cleanupTimer) {
        m_cleanupTimer->setInterval(seconds * 1000);
    }
}

// === 新的高级API实现 ===

std::future<bool> CacheManagerV2::setAsync(const QString& key, const QVariant& value, int ttlSeconds)
{
    return std::async(std::launch::async, [this, key, value, ttlSeconds]() {
        return set(key, value, ttlSeconds);
    });
}

std::future<QVariant> CacheManagerV2::getAsync(const QString& key)
{
    return std::async(std::launch::async, [this, key]() {
        return get(key);
    });
}

std::future<bool> CacheManagerV2::removeAsync(const QString& key)
{
    return std::async(std::launch::async, [this, key]() {
        return remove(key);
    });
}

void CacheManagerV2::warmup(const QStringList& keys)
{
    if (m_multiLevelCache) {
        m_multiLevelCache->warmup(keys);
    }
}

void CacheManagerV2::warmupCategory(const QString& category)
{
    if (m_multiLevelCache) {
        m_multiLevelCache->warmupCategory(category);
    }
}

QStringList CacheManagerV2::predictNextAccess(int count) const
{
    if (m_strategyManager) {
        return m_strategyManager->predictNextAccess(count);
    }
    return QStringList();
}

QStringList CacheManagerV2::recommendPrefetch(const QString& key, int count) const
{
    if (m_strategyManager) {
        return m_strategyManager->recommendPrefetch(key, count);
    }
    return QStringList();
}

void CacheManagerV2::enableAdaptiveOptimization(bool enabled)
{
    if (m_strategyManager) {
        m_strategyManager->enableAdaptiveOptimization(enabled);
    }
}

QJsonObject CacheManagerV2::getDetailedMetrics() const
{
    if (m_multiLevelCache) {
        return m_multiLevelCache->getMetrics();
    }
    return QJsonObject();
}

QJsonObject CacheManagerV2::getPerformanceReport() const
{
    if (m_strategyManager) {
        return m_strategyManager->getDetailedAnalysis();
    }
    return QJsonObject();
}

QStringList CacheManagerV2::getHotKeys(int count) const
{
    if (m_multiLevelCache) {
        return m_multiLevelCache->getHotKeys(count);
    }
    return QStringList();
}

QStringList CacheManagerV2::getColdKeys(int count) const
{
    if (m_multiLevelCache) {
        return m_multiLevelCache->getColdKeys(count);
    }
    return QStringList();
}

void CacheManagerV2::cleanup()
{
    performCleanup();
}

void CacheManagerV2::compactMemory()
{
    if (m_multiLevelCache) {
        m_multiLevelCache->compact();
    }
}

void CacheManagerV2::optimize()
{
    if (m_multiLevelCache) {
        m_multiLevelCache->optimize();
    }
}

// === 私有方法实现 ===

void CacheManagerV2::setupConnections()
{
    if (m_multiLevelCache) {
        connect(m_multiLevelCache.get(), &MultiLevelCache::itemCached,
                this, &CacheManagerV2::itemCached);
        connect(m_multiLevelCache.get(), &MultiLevelCache::itemEvicted,
                this, &CacheManagerV2::itemEvicted);
        connect(m_multiLevelCache.get(), &MultiLevelCache::itemPromoted,
                this, &CacheManagerV2::itemPromoted);
        connect(m_multiLevelCache.get(), &MultiLevelCache::performanceAlert,
                this, &CacheManagerV2::performanceAlert);
    }
    
    if (m_strategyManager) {
        connect(m_strategyManager.get(), &CacheStrategyManager::optimizationCompleted,
                this, &CacheManagerV2::onOptimizationCompleted);
        connect(m_strategyManager.get(), &CacheStrategyManager::performanceAlert,
                this, &CacheManagerV2::onPerformanceAlert);
    }
}

void CacheManagerV2::updateLegacyStats(bool hit)
{
    m_legacyStats.totalRequests++;
    if (hit) {
        m_legacyStats.hitCount++;
    } else {
        m_legacyStats.missCount++;
    }
    
    if (m_legacyStats.totalRequests > 0) {
        m_legacyStats.hitRate = static_cast<double>(m_legacyStats.hitCount) / m_legacyStats.totalRequests;
    }
}

QString CacheManagerV2::generateUserKey(qint64 userId) const
{
    return QString("user:%1").arg(userId);
}

QString CacheManagerV2::generateGroupKey(qint64 groupId) const
{
    return QString("group:%1").arg(groupId);
}

QString CacheManagerV2::generateMessageKey(qint64 chatId) const
{
    return QString("messages:%1").arg(chatId);
}

QString CacheManagerV2::generateSessionKey(const QString &sessionToken) const
{
    return QString("session:%1").arg(sessionToken);
}

QString CacheManagerV2::generateQueryCacheKey(const QString &sql, const QVariantMap &parameters) const
{
    QJsonObject json;
    json["sql"] = sql;
    json["params"] = QJsonObject::fromVariantMap(parameters);
    
    QByteArray data = QJsonDocument(json).toJson(QJsonDocument::Compact);
    QByteArray hash = QCryptographicHash::hash(data, QCryptographicHash::Md5);
    
    return QString("query:%1").arg(QString(hash.toHex()));
}

void CacheManagerV2::addToCategory(const QString &category, const QString &key)
{
    QMutexLocker locker(&m_categoriesMutex);
    if (!m_categories[category].contains(key)) {
        m_categories[category].append(key);
    }
}

void CacheManagerV2::removeFromCategory(const QString &category, const QString &key)
{
    QMutexLocker locker(&m_categoriesMutex);
    m_categories[category].removeAll(key);
}

void CacheManagerV2::performCleanup()
{
    // 兼容的清理方法
    if (m_multiLevelCache) {
        // 多级缓存有自己的清理机制
    }
}

void CacheManagerV2::onCacheEvent()
{
    // 处理缓存事件
}

void CacheManagerV2::onOptimizationCompleted(const QJsonObject& results)
{
    emit optimizationCompleted(results);
}

void CacheManagerV2::onPerformanceAlert(const QString& message)
{
    emit performanceAlert(message);
}

void CacheManagerV2::logCacheManagerEvent(const QString& event, const QString& details) const
{
    if (details.isEmpty()) {
        qCDebug(cacheManagerV2) << event;
    } else {
        qCDebug(cacheManagerV2) << event << ":" << details;
    }
}
