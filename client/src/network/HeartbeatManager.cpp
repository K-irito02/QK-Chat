#include "HeartbeatManager.h"
#include "../utils/LogManager.h"

Q_LOGGING_CATEGORY(heartbeatManager, "qkchat.client.heartbeat")

HeartbeatManager::HeartbeatManager(QObject *parent)
    : QObject(parent)
    , _state(Stopped)
    , _interval(DEFAULT_INTERVAL)
    , _timeout(DEFAULT_TIMEOUT)
    , _maxMissedBeats(DEFAULT_MAX_MISSED_BEATS)
    , _adaptiveMode(false)
    , _latencyThreshold(DEFAULT_LATENCY_THRESHOLD)
    , _missedBeats(0)
    , _lastLatency(0)
    , _totalSent(0)
    , _totalReceived(0)
    , _currentQuality(Good)
    , _adaptiveBaseInterval(DEFAULT_INTERVAL)
    , _adaptiveMinInterval(ADAPTIVE_MIN_INTERVAL)
    , _adaptiveMaxInterval(ADAPTIVE_MAX_INTERVAL)
{
    // 创建定时器
    _heartbeatTimer = new QTimer(this);
    connect(_heartbeatTimer, &QTimer::timeout, this, &HeartbeatManager::onHeartbeatTimer);
    
    _timeoutTimer = new QTimer(this);
    _timeoutTimer->setSingleShot(true);
    connect(_timeoutTimer, &QTimer::timeout, this, &HeartbeatManager::onTimeoutTimer);
    
    _qualityCheckTimer = new QTimer(this);
    connect(_qualityCheckTimer, &QTimer::timeout, this, &HeartbeatManager::onQualityCheckTimer);
    _qualityCheckTimer->start(QUALITY_CHECK_INTERVAL);
    
    qCInfo(heartbeatManager) << "HeartbeatManager initialized";
}

HeartbeatManager::~HeartbeatManager()
{
    stop();
}

void HeartbeatManager::start()
{
    if (_state == Running) {
        return;
    }
    
    _state = Running;
    _missedBeats = 0;
    
    _heartbeatTimer->start(_interval);
    
    qCInfo(heartbeatManager) << "Heartbeat started with interval:" << _interval << "ms";
    LogManager::instance()->writeHeartbeatLog("STARTED", _interval);
}

void HeartbeatManager::stop()
{
    if (_state == Stopped) {
        return;
    }
    
    _state = Stopped;
    _heartbeatTimer->stop();
    _timeoutTimer->stop();
    
    qCInfo(heartbeatManager) << "Heartbeat stopped";
    LogManager::instance()->writeHeartbeatLog("STOPPED");
}

void HeartbeatManager::pause()
{
    if (_state != Running) {
        return;
    }
    
    _heartbeatTimer->stop();
    _timeoutTimer->stop();
    
    qCInfo(heartbeatManager) << "Heartbeat paused";
    LogManager::instance()->writeHeartbeatLog("PAUSED");
}

void HeartbeatManager::resume()
{
    if (_state != Running) {
        return;
    }
    
    _heartbeatTimer->start(_interval);
    
    qCInfo(heartbeatManager) << "Heartbeat resumed";
    LogManager::instance()->writeHeartbeatLog("RESUMED");
}

bool HeartbeatManager::isRunning() const
{
    return _state == Running;
}

HeartbeatManager::HeartbeatState HeartbeatManager::getState() const
{
    return _state;
}

void HeartbeatManager::sendHeartbeat()
{
    if (_state != Running) {
        return;
    }
    
    _lastSentTime = QDateTime::currentDateTime();
    _totalSent++;
    _state = WaitingResponse;
    
    // 启动超时定时器
    _timeoutTimer->start(_timeout);
    
    qCDebug(heartbeatManager) << "Heartbeat sent at:" << _lastSentTime.toString();
    LogManager::instance()->writeHeartbeatLog("SENT", -1);
    
    emit heartbeatSent(_lastSentTime);
}

void HeartbeatManager::handleHeartbeatResponse(const QDateTime &serverTime)
{
    if (_state != WaitingResponse) {
        qCWarning(heartbeatManager) << "Received heartbeat response but not waiting for one";
        return;
    }
    
    _lastReceivedTime = QDateTime::currentDateTime();
    _timeoutTimer->stop();
    _state = Running;
    _missedBeats = 0;
    _totalReceived++;
    
    // 计算延迟
    if (_lastSentTime.isValid()) {
        _lastLatency = _lastSentTime.msecsTo(_lastReceivedTime);
    } else {
        _lastLatency = 0;
    }
    
    // 记录心跳记录
    HeartbeatRecord record;
    record.sentTime = _lastSentTime;
    record.receivedTime = _lastReceivedTime;
    record.latency = _lastLatency;
    record.successful = true;
    record.errorMessage = "";
    
    updateStatistics(record);
    
    qCDebug(heartbeatManager) << "Heartbeat response received, latency:" << _lastLatency << "ms";
    LogManager::instance()->writeHeartbeatLog("RECEIVED", _lastLatency);
    
    emit heartbeatReceived(_lastReceivedTime, _lastLatency);
    emit latencyChanged(_lastLatency);
    
    // 自适应调整
    if (_adaptiveMode) {
        adjustAdaptiveInterval();
    }
}

void HeartbeatManager::setInterval(int intervalMs)
{
    _interval = intervalMs;
    _adaptiveBaseInterval = intervalMs;
    
    if (_heartbeatTimer->isActive()) {
        _heartbeatTimer->setInterval(_interval);
    }
    
    qCInfo(heartbeatManager) << "Heartbeat interval set to:" << _interval << "ms";
}

void HeartbeatManager::setTimeout(int timeoutMs)
{
    _timeout = timeoutMs;
    qCInfo(heartbeatManager) << "Heartbeat timeout set to:" << _timeout << "ms";
}

void HeartbeatManager::setMaxMissedBeats(int maxMissed)
{
    _maxMissedBeats = maxMissed;
    qCInfo(heartbeatManager) << "Max missed beats set to:" << _maxMissedBeats;
}

void HeartbeatManager::setAdaptiveMode(bool enabled)
{
    _adaptiveMode = enabled;
    qCInfo(heartbeatManager) << "Adaptive mode:" << (enabled ? "enabled" : "disabled");
}

void HeartbeatManager::setLatencyThreshold(qint64 thresholdMs)
{
    _latencyThreshold = thresholdMs;
    qCInfo(heartbeatManager) << "Latency threshold set to:" << _latencyThreshold << "ms";
}

int HeartbeatManager::getInterval() const
{
    return _interval;
}

int HeartbeatManager::getTimeout() const
{
    return _timeout;
}

int HeartbeatManager::getMissedBeats() const
{
    return _missedBeats;
}

int HeartbeatManager::getMaxMissedBeats() const
{
    return _maxMissedBeats;
}

qint64 HeartbeatManager::getLastLatency() const
{
    return _lastLatency;
}

qint64 HeartbeatManager::getAverageLatency() const
{
    return calculateAverageLatency();
}

double HeartbeatManager::getPacketLossRate() const
{
    return calculatePacketLossRate();
}

HeartbeatManager::ConnectionQuality HeartbeatManager::getConnectionQuality() const
{
    return _currentQuality;
}

QString HeartbeatManager::getQualityDescription() const
{
    switch (_currentQuality) {
    case Excellent:
        return "优秀";
    case Good:
        return "良好";
    case Fair:
        return "一般";
    case Poor:
        return "较差";
    case Bad:
        return "很差";
    default:
        return "未知";
    }
}

int HeartbeatManager::getTotalSent() const
{
    return _totalSent;
}

int HeartbeatManager::getTotalReceived() const
{
    return _totalReceived;
}

QList<HeartbeatManager::HeartbeatRecord> HeartbeatManager::getRecentRecords(int count) const
{
    QList<HeartbeatRecord> records;
    int start = qMax(0, _recentRecords.size() - count);
    
    for (int i = start; i < _recentRecords.size(); ++i) {
        records.append(_recentRecords.at(i));
    }
    
    return records;
}

void HeartbeatManager::clearStatistics()
{
    _totalSent = 0;
    _totalReceived = 0;
    _recentRecords.clear();
    _missedBeats = 0;
    _lastLatency = 0;
    
    qCInfo(heartbeatManager) << "Statistics cleared";
}

void HeartbeatManager::onHeartbeatTimer()
{
    sendHeartbeat();
}

void HeartbeatManager::onTimeoutTimer()
{
    _missedBeats++;
    _state = Running;
    
    // 记录超时记录
    HeartbeatRecord record;
    record.sentTime = _lastSentTime;
    record.receivedTime = QDateTime();
    record.latency = -1;
    record.successful = false;
    record.errorMessage = "Timeout";
    
    updateStatistics(record);
    
    qCWarning(heartbeatManager) << "Heartbeat timeout, missed beats:" << _missedBeats;
    LogManager::instance()->writeHeartbeatLog("TIMEOUT", _missedBeats);
    
    emit heartbeatTimeout();
    
    if (_missedBeats >= _maxMissedBeats) {
        qCCritical(heartbeatManager) << "Maximum missed beats reached:" << _maxMissedBeats;
        LogManager::instance()->writeHeartbeatLog("MAX_MISSED", _maxMissedBeats);
        
        emit maxMissedBeatsReached();
    }
}

void HeartbeatManager::onQualityCheckTimer()
{
    checkConnectionQuality();
}

void HeartbeatManager::updateStatistics(const HeartbeatRecord &record)
{
    _recentRecords.enqueue(record);
    
    // 限制记录数量
    while (_recentRecords.size() > MAX_RECENT_RECORDS) {
        _recentRecords.dequeue();
    }
}

void HeartbeatManager::checkConnectionQuality()
{
    qint64 avgLatency = calculateAverageLatency();
    double lossRate = calculatePacketLossRate();
    
    ConnectionQuality newQuality = evaluateQuality(avgLatency, lossRate);
    
    if (newQuality != _currentQuality) {
        ConnectionQuality oldQuality = _currentQuality;
        _currentQuality = newQuality;
        
        qCInfo(heartbeatManager) << "Connection quality changed from" 
                                 << static_cast<int>(oldQuality) << "to" << static_cast<int>(newQuality);
        LogManager::instance()->writeHeartbeatLog("QUALITY_CHANGED", avgLatency);
        
        emit connectionQualityChanged(_currentQuality);
    }
    
    emit packetLossChanged(lossRate);
}

void HeartbeatManager::adjustAdaptiveInterval()
{
    if (!_adaptiveMode || _recentRecords.isEmpty()) {
        return;
    }
    
    qint64 avgLatency = calculateAverageLatency();
    double lossRate = calculatePacketLossRate();
    
    int newInterval = _adaptiveBaseInterval;
    
    // 根据延迟调整
    if (avgLatency > _latencyThreshold) {
        // 延迟高，增加间隔
        newInterval = static_cast<int>(newInterval * 1.2);
    } else if (avgLatency < _latencyThreshold / 2) {
        // 延迟低，减少间隔
        newInterval = static_cast<int>(newInterval * 0.9);
    }
    
    // 根据丢包率调整
    if (lossRate > 0.05) { // 5%
        // 丢包率高，增加间隔
        newInterval = static_cast<int>(newInterval * 1.3);
    } else if (lossRate < 0.01) { // 1%
        // 丢包率低，可以减少间隔
        newInterval = static_cast<int>(newInterval * 0.95);
    }
    
    // 限制范围
    newInterval = qBound(_adaptiveMinInterval, newInterval, _adaptiveMaxInterval);
    
    if (newInterval != _interval) {
        qCInfo(heartbeatManager) << "Adaptive interval changed from" << _interval << "to" << newInterval;
        setInterval(newInterval);
    }
}

qint64 HeartbeatManager::calculateAverageLatency() const
{
    if (_recentRecords.isEmpty()) {
        return 0;
    }
    
    qint64 totalLatency = 0;
    int validRecords = 0;
    
    for (const HeartbeatRecord &record : _recentRecords) {
        if (record.successful && record.latency >= 0) {
            totalLatency += record.latency;
            validRecords++;
        }
    }
    
    return validRecords > 0 ? totalLatency / validRecords : 0;
}

double HeartbeatManager::calculatePacketLossRate() const
{
    if (_recentRecords.isEmpty()) {
        return 0.0;
    }
    
    int totalRecords = _recentRecords.size();
    int failedRecords = 0;
    
    for (const HeartbeatRecord &record : _recentRecords) {
        if (!record.successful) {
            failedRecords++;
        }
    }
    
    return static_cast<double>(failedRecords) / totalRecords;
}

HeartbeatManager::ConnectionQuality HeartbeatManager::evaluateQuality(qint64 avgLatency, double lossRate) const
{
    if (avgLatency < 50 && lossRate < 0.001) {
        return Excellent;
    } else if (avgLatency < 100 && lossRate < 0.01) {
        return Good;
    } else if (avgLatency < 200 && lossRate < 0.05) {
        return Fair;
    } else if (avgLatency < 500 && lossRate < 0.1) {
        return Poor;
    } else {
        return Bad;
    }
}
