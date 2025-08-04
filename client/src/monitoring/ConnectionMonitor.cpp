#include "ConnectionMonitor.h"
#include "../utils/LogManager.h"
#include <QRandomGenerator>

Q_LOGGING_CATEGORY(connectionMonitor, "qkchat.client.monitor")

ConnectionMonitor::ConnectionMonitor(QObject *parent)
    : QObject(parent)
    , _isMonitoring(false)
    , _currentQuality(Good)
    , _monitoringInterval(DEFAULT_MONITORING_INTERVAL)
    , _metricRetentionHours(DEFAULT_RETENTION_HOURS)
{
    // 创建定时器
    _monitoringTimer = new QTimer(this);
    connect(_monitoringTimer, &QTimer::timeout, this, &ConnectionMonitor::onMonitoringTimer);
    
    _cleanupTimer = new QTimer(this);
    connect(_cleanupTimer, &QTimer::timeout, this, &ConnectionMonitor::onCleanupTimer);
    _cleanupTimer->start(CLEANUP_INTERVAL);
    
    // 初始化统计数据
    memset(&_stats, 0, sizeof(_stats));
    _stats.minLatency = LLONG_MAX;
    
    // 设置默认告警阈值
    _alertThresholds[ConnectionSuccess] = 95.0;  // 成功率低于95%
    _alertThresholds[Latency] = 1000.0;          // 延迟超过1秒
    _alertThresholds[PacketLoss] = 5.0;          // 丢包率超过5%
    _alertThresholds[Reconnection] = 5.0;        // 重连次数超过5次/小时
    
    // 默认启用所有告警
    _alertEnabled[ConnectionSuccess] = true;
    _alertEnabled[ConnectionFailure] = true;
    _alertEnabled[Latency] = true;
    _alertEnabled[PacketLoss] = true;
    _alertEnabled[Reconnection] = true;
    _alertEnabled[Error] = true;
    
    qCInfo(connectionMonitor) << "ConnectionMonitor initialized";
}

ConnectionMonitor::~ConnectionMonitor()
{
    stopMonitoring();
}

void ConnectionMonitor::startMonitoring()
{
    if (_isMonitoring) {
        return;
    }
    
    _isMonitoring = true;
    _monitoringStartTime = QDateTime::currentDateTime();
    _monitoringTimer->start(_monitoringInterval);
    
    qCInfo(connectionMonitor) << "Connection monitoring started";
    LogManager::instance()->writePerformanceLog("MONITORING_STARTED", 1, "");
}

void ConnectionMonitor::stopMonitoring()
{
    if (!_isMonitoring) {
        return;
    }
    
    _isMonitoring = false;
    _monitoringTimer->stop();
    
    qCInfo(connectionMonitor) << "Connection monitoring stopped";
    LogManager::instance()->writePerformanceLog("MONITORING_STOPPED", 1, "");
}

bool ConnectionMonitor::isMonitoring() const
{
    return _isMonitoring;
}

void ConnectionMonitor::recordConnectionAttempt()
{
    _lastConnectionAttempt = QDateTime::currentDateTime();
    _stats.totalConnections++;
    
    addMetric(ConnectionSuccess, 0, "attempt", "Connection attempt recorded");
    
    qCDebug(connectionMonitor) << "Connection attempt recorded";
    LogManager::instance()->writePerformanceLog("CONNECTION_ATTEMPT", _stats.totalConnections, "total");
}

void ConnectionMonitor::recordConnectionSuccess(qint64 latency)
{
    _lastSuccessfulConnection = QDateTime::currentDateTime();
    _stats.successfulConnections++;
    
    if (latency > 0) {
        recordLatency(latency);
    }
    
    updateStatistics();
    
    addMetric(ConnectionSuccess, 1, "success", "Connection established successfully", 
              QVariantMap{{"latency", latency}});
    
    qCDebug(connectionMonitor) << "Connection success recorded, latency:" << latency << "ms";
    LogManager::instance()->writePerformanceLog("CONNECTION_SUCCESS", _stats.successfulConnections, "total");
}

void ConnectionMonitor::recordConnectionFailure(const QString &reason)
{
    _lastFailedConnection = QDateTime::currentDateTime();
    _stats.failedConnections++;
    
    updateStatistics();
    
    addMetric(ConnectionFailure, 1, "failure", "Connection failed: " + reason, 
              QVariantMap{{"reason", reason}});
    
    // 检查是否需要生成告警
    if (_stats.successRate < _alertThresholds[ConnectionSuccess]) {
        generateAlert(Warning, 
                     QString("Connection success rate dropped to %1%").arg(_stats.successRate, 0, 'f', 1),
                     ConnectionSuccess);
    }
    
    qCWarning(connectionMonitor) << "Connection failure recorded:" << reason;
    LogManager::instance()->writePerformanceLog("CONNECTION_FAILURE", _stats.failedConnections, "total");
}

void ConnectionMonitor::recordLatency(qint64 latency)
{
    // 更新延迟统计
    if (latency < _stats.minLatency) {
        _stats.minLatency = latency;
    }
    if (latency > _stats.maxLatency) {
        _stats.maxLatency = latency;
    }
    
    // 计算平均延迟（简单移动平均）
    if (_stats.averageLatency == 0) {
        _stats.averageLatency = latency;
    } else {
        _stats.averageLatency = (_stats.averageLatency + latency) / 2;
    }
    
    addMetric(Latency, latency, "ms", "Network latency measurement");
    
    // 检查延迟告警
    if (latency > _alertThresholds[Latency]) {
        generateAlert(Warning, 
                     QString("High latency detected: %1ms").arg(latency),
                     Latency, QVariantMap{{"latency", latency}});
    }
    
    qCDebug(connectionMonitor) << "Latency recorded:" << latency << "ms";
    LogManager::instance()->writePerformanceLog("LATENCY", latency, "ms");
}

void ConnectionMonitor::recordPacketLoss(int lostPackets, int totalPackets)
{
    if (totalPackets <= 0) {
        return;
    }
    
    double lossRate = static_cast<double>(lostPackets) / totalPackets * 100.0;
    _stats.packetLossRate = lossRate;
    
    addMetric(PacketLoss, lossRate, "%", "Packet loss measurement", 
              QVariantMap{{"lost", lostPackets}, {"total", totalPackets}});
    
    // 检查丢包告警
    if (lossRate > _alertThresholds[PacketLoss]) {
        generateAlert(Warning, 
                     QString("High packet loss detected: %1%").arg(lossRate, 0, 'f', 1),
                     PacketLoss, QVariantMap{{"lossRate", lossRate}});
    }
    
    qCDebug(connectionMonitor) << "Packet loss recorded:" << lossRate << "%";
    LogManager::instance()->writePerformanceLog("PACKET_LOSS", lossRate, "%");
}

void ConnectionMonitor::recordThroughput(qint64 bytes, qint64 timeMs)
{
    if (timeMs <= 0) {
        return;
    }
    
    double throughput = static_cast<double>(bytes) / timeMs * 1000.0; // bytes/second
    _stats.totalBytesTransferred += bytes;
    
    addMetric(Throughput, throughput, "bytes/s", "Network throughput measurement", 
              QVariantMap{{"bytes", bytes}, {"timeMs", timeMs}});
    
    qCDebug(connectionMonitor) << "Throughput recorded:" << throughput << "bytes/s";
    LogManager::instance()->writePerformanceLog("THROUGHPUT", throughput, "bytes/s");
}

void ConnectionMonitor::recordReconnection(const QString &reason)
{
    _stats.totalReconnections++;
    
    addMetric(Reconnection, 1, "count", "Reconnection occurred: " + reason, 
              QVariantMap{{"reason", reason}});
    
    // 检查重连频率
    QDateTime oneHourAgo = QDateTime::currentDateTime().addSecs(-3600);
    int recentReconnections = 0;
    
    if (_metrics.contains(Reconnection)) {
        for (const MetricData &metric : _metrics[Reconnection]) {
            if (metric.timestamp >= oneHourAgo) {
                recentReconnections++;
            }
        }
    }
    
    if (recentReconnections > _alertThresholds[Reconnection]) {
        generateAlert(Critical, 
                     QString("Frequent reconnections detected: %1 in the last hour").arg(recentReconnections),
                     Reconnection, QVariantMap{{"count", recentReconnections}});
    }
    
    qCWarning(connectionMonitor) << "Reconnection recorded:" << reason;
    LogManager::instance()->writePerformanceLog("RECONNECTION", _stats.totalReconnections, "total");
}

void ConnectionMonitor::recordError(const QString &error, const QString &category)
{
    addMetric(Error, 1, "count", "Error occurred: " + error, 
              QVariantMap{{"error", error}, {"category", category}});
    
    generateAlert(Warning, QString("Error in %1: %2").arg(category, error), Error, 
                 QVariantMap{{"error", error}, {"category", category}});
    
    qCWarning(connectionMonitor) << "Error recorded:" << category << error;
    LogManager::instance()->writeErrorLog(QString("[%1] %2").arg(category, error), "ConnectionMonitor");
}

ConnectionMonitor::ConnectionStats ConnectionMonitor::getConnectionStats() const
{
    return _stats;
}

QList<ConnectionMonitor::MetricData> ConnectionMonitor::getMetrics(MetricType type, int maxCount) const
{
    QList<MetricData> result;
    
    if (_metrics.contains(type)) {
        const QQueue<MetricData> &queue = _metrics[type];
        int count = qMin(maxCount, queue.size());
        
        for (int i = queue.size() - count; i < queue.size(); ++i) {
            result.append(queue.at(i));
        }
    }
    
    return result;
}

QList<ConnectionMonitor::MetricData> ConnectionMonitor::getRecentMetrics(int minutes) const
{
    QList<MetricData> result;
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-minutes * 60);
    
    for (auto it = _metrics.begin(); it != _metrics.end(); ++it) {
        for (const MetricData &metric : it.value()) {
            if (metric.timestamp >= cutoff) {
                result.append(metric);
            }
        }
    }
    
    // 按时间排序
    std::sort(result.begin(), result.end(), [](const MetricData &a, const MetricData &b) {
        return a.timestamp < b.timestamp;
    });
    
    return result;
}

QList<ConnectionMonitor::Alert> ConnectionMonitor::getAlerts(AlertLevel minLevel, int maxCount) const
{
    QList<Alert> result;
    
    for (const Alert &alert : _alerts) {
        if (alert.level >= minLevel) {
            result.append(alert);
        }
    }
    
    // 按时间倒序排序（最新的在前）
    std::sort(result.begin(), result.end(), [](const Alert &a, const Alert &b) {
        return a.timestamp > b.timestamp;
    });
    
    if (result.size() > maxCount) {
        result = result.mid(0, maxCount);
    }
    
    return result;
}

ConnectionMonitor::QualityLevel ConnectionMonitor::getConnectionQuality() const
{
    return _currentQuality;
}

QString ConnectionMonitor::getQualityDescription() const
{
    switch (_currentQuality) {
    case Excellent:
        return "优秀 - 连接质量非常好";
    case Good:
        return "良好 - 连接质量正常";
    case Fair:
        return "一般 - 连接质量可接受";
    case Poor:
        return "较差 - 连接质量不佳";
    case Bad:
        return "很差 - 连接质量严重问题";
    default:
        return "未知";
    }
}

double ConnectionMonitor::getQualityScore() const
{
    return calculateQualityScore();
}

QStringList ConnectionMonitor::diagnoseConnection() const
{
    QStringList diagnosis;
    
    // 检查连接成功率
    if (_stats.successRate < 90.0) {
        diagnosis << QString("连接成功率较低: %1%").arg(_stats.successRate, 0, 'f', 1);
    }
    
    // 检查延迟
    if (_stats.averageLatency > 500) {
        diagnosis << QString("平均延迟较高: %1ms").arg(_stats.averageLatency);
    }
    
    // 检查丢包率
    if (_stats.packetLossRate > 2.0) {
        diagnosis << QString("丢包率较高: %1%").arg(_stats.packetLossRate, 0, 'f', 1);
    }
    
    // 检查重连频率
    if (_stats.totalReconnections > 10) {
        diagnosis << QString("重连次数较多: %1次").arg(_stats.totalReconnections);
    }
    
    if (diagnosis.isEmpty()) {
        diagnosis << "连接状态正常";
    }
    
    return diagnosis;
}

QStringList ConnectionMonitor::getRecommendations() const
{
    QStringList recommendations;
    
    if (_stats.successRate < 90.0) {
        recommendations << "建议检查网络连接和服务器状态";
    }
    
    if (_stats.averageLatency > 500) {
        recommendations << "建议检查网络延迟，考虑使用更近的服务器";
    }
    
    if (_stats.packetLossRate > 2.0) {
        recommendations << "建议检查网络稳定性，可能需要更换网络环境";
    }
    
    if (_stats.totalReconnections > 10) {
        recommendations << "建议检查网络稳定性和服务器配置";
    }
    
    return recommendations;
}

bool ConnectionMonitor::isConnectionStable() const
{
    // 连接稳定性判断标准
    return _stats.successRate >= 95.0 && 
           _stats.averageLatency <= 300 && 
           _stats.packetLossRate <= 1.0;
}

void ConnectionMonitor::setMonitoringInterval(int intervalMs)
{
    _monitoringInterval = intervalMs;
    
    if (_monitoringTimer->isActive()) {
        _monitoringTimer->setInterval(_monitoringInterval);
    }
}

void ConnectionMonitor::setMetricRetentionTime(int hours)
{
    _metricRetentionHours = hours;
}

void ConnectionMonitor::setAlertThresholds(const QHash<MetricType, double> &thresholds)
{
    _alertThresholds = thresholds;
}

void ConnectionMonitor::enableAlert(MetricType type, bool enabled)
{
    _alertEnabled[type] = enabled;
}

void ConnectionMonitor::onMonitoringTimer()
{
    updateStatistics();
    checkAlerts();

    // 检查连接质量变化
    QualityLevel newQuality = calculateQuality();
    if (newQuality != _currentQuality) {
        _currentQuality = newQuality;
        emit qualityChanged(_currentQuality);

        qCInfo(connectionMonitor) << "Connection quality changed to:" << static_cast<int>(_currentQuality);
        LogManager::instance()->writePerformanceLog("QUALITY_CHANGED", static_cast<int>(_currentQuality), "level");
    }

    // 检查连接稳定性
    bool stable = isConnectionStable();
    static bool lastStable = true;
    if (stable != lastStable) {
        lastStable = stable;
        emit connectionStabilityChanged(stable);

        qCInfo(connectionMonitor) << "Connection stability changed to:" << stable;
        LogManager::instance()->writePerformanceLog("STABILITY_CHANGED", stable ? 1 : 0, "stable");
    }

    emit statisticsUpdated(_stats);
}

void ConnectionMonitor::onCleanupTimer()
{
    cleanupOldData();
}

void ConnectionMonitor::addMetric(MetricType type, double value, const QString &unit,
                                 const QString &description, const QVariantMap &metadata)
{
    MetricData metric;
    metric.type = type;
    metric.timestamp = QDateTime::currentDateTime();
    metric.value = value;
    metric.unit = unit;
    metric.description = description;
    metric.metadata = metadata;

    // 添加到队列
    if (!_metrics.contains(type)) {
        _metrics[type] = QQueue<MetricData>();
    }

    _metrics[type].enqueue(metric);

    // 限制队列大小
    while (_metrics[type].size() > MAX_METRICS_PER_TYPE) {
        _metrics[type].dequeue();
    }

    emit metricRecorded(metric);
}

void ConnectionMonitor::updateStatistics()
{
    // 更新成功率
    if (_stats.totalConnections > 0) {
        _stats.successRate = static_cast<double>(_stats.successfulConnections) / _stats.totalConnections * 100.0;
    }

    // 更新运行时间
    if (_monitoringStartTime.isValid()) {
        _stats.totalUptime = _monitoringStartTime.msecsTo(QDateTime::currentDateTime());
    }
}

void ConnectionMonitor::checkAlerts()
{
    // 这里可以添加更多的告警检查逻辑
    // 目前告警在记录指标时已经检查
}

void ConnectionMonitor::generateAlert(AlertLevel level, const QString &message, MetricType relatedMetric,
                                     const QVariantMap &data)
{
    if (!_alertEnabled.value(relatedMetric, true)) {
        return;
    }

    Alert alert;
    alert.level = level;
    alert.message = message;
    alert.timestamp = QDateTime::currentDateTime();
    alert.relatedMetric = relatedMetric;
    alert.data = data;

    _alerts.enqueue(alert);

    // 限制告警数量
    while (_alerts.size() > MAX_ALERTS) {
        _alerts.dequeue();
    }

    emit alertGenerated(alert);

    QString levelStr;
    switch (level) {
    case Info: levelStr = "INFO"; break;
    case Warning: levelStr = "WARNING"; break;
    case Critical: levelStr = "CRITICAL"; break;
    }

    qCWarning(connectionMonitor) << "Alert generated:" << levelStr << message;
    LogManager::instance()->writeErrorLog(QString("[%1] %2").arg(levelStr, message), "ConnectionMonitor");
}

void ConnectionMonitor::cleanupOldData()
{
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-_metricRetentionHours * 3600);

    // 清理过期指标
    for (auto it = _metrics.begin(); it != _metrics.end(); ++it) {
        QQueue<MetricData> &queue = it.value();
        while (!queue.isEmpty() && queue.first().timestamp < cutoff) {
            queue.dequeue();
        }
    }

    // 清理过期告警
    while (!_alerts.isEmpty() && _alerts.first().timestamp < cutoff) {
        _alerts.dequeue();
    }

    qCDebug(connectionMonitor) << "Old data cleaned up, cutoff:" << cutoff;
}

ConnectionMonitor::QualityLevel ConnectionMonitor::calculateQuality() const
{
    double score = calculateQualityScore();

    if (score >= 90.0) {
        return Excellent;
    } else if (score >= 75.0) {
        return Good;
    } else if (score >= 60.0) {
        return Fair;
    } else if (score >= 40.0) {
        return Poor;
    } else {
        return Bad;
    }
}

double ConnectionMonitor::calculateQualityScore() const
{
    double score = 100.0;

    // 成功率权重: 40%
    if (_stats.totalConnections > 0) {
        double successWeight = 0.4;
        double successScore = _stats.successRate;
        score = score * (1 - successWeight) + successScore * successWeight;
    }

    // 延迟权重: 30%
    if (_stats.averageLatency > 0) {
        double latencyWeight = 0.3;
        double latencyScore = qMax(0.0, 100.0 - _stats.averageLatency / 10.0); // 1000ms = 0分
        score = score * (1 - latencyWeight) + latencyScore * latencyWeight;
    }

    // 丢包率权重: 20%
    double packetLossWeight = 0.2;
    double packetLossScore = qMax(0.0, 100.0 - _stats.packetLossRate * 10.0); // 10% = 0分
    score = score * (1 - packetLossWeight) + packetLossScore * packetLossWeight;

    // 稳定性权重: 10%
    double stabilityWeight = 0.1;
    double stabilityScore = _stats.totalReconnections > 0 ?
                           qMax(0.0, 100.0 - _stats.totalReconnections * 5.0) : 100.0;
    score = score * (1 - stabilityWeight) + stabilityScore * stabilityWeight;

    return qBound(0.0, score, 100.0);
}
