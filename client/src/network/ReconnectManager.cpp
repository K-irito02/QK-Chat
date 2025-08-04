#include "ReconnectManager.h"
#include "../utils/LogManager.h"
#include <QNetworkInterface>
#include <QtMath>

Q_LOGGING_CATEGORY(reconnectManager, "qkchat.client.reconnect")

ReconnectManager::ReconnectManager(QObject *parent)
    : QObject(parent)
    , _isReconnecting(false)
    , _currentAttempt(0)
    , _maxAttempts(DEFAULT_MAX_ATTEMPTS)
    , _baseInterval(DEFAULT_BASE_INTERVAL)
    , _maxInterval(DEFAULT_MAX_INTERVAL)
    , _backoffMultiplier(DEFAULT_BACKOFF_MULTIPLIER)
    , _strategy(ExponentialBackoff)
    , _connectionTimeout(DEFAULT_CONNECTION_TIMEOUT)
    , _networkAvailable(true)
    , _totalAttempts(0)
    , _successfulAttempts(0)
{
    // 创建定时器
    _reconnectTimer = new QTimer(this);
    _reconnectTimer->setSingleShot(true);
    connect(_reconnectTimer, &QTimer::timeout, this, &ReconnectManager::onReconnectTimer);
    
    _connectionTimeoutTimer = new QTimer(this);
    _connectionTimeoutTimer->setSingleShot(true);
    connect(_connectionTimeoutTimer, &QTimer::timeout, this, &ReconnectManager::onConnectionTimeout);
    
    _networkStatusTimer = new QTimer(this);
    connect(_networkStatusTimer, &QTimer::timeout, this, &ReconnectManager::onNetworkStatusCheck);
    _networkStatusTimer->start(NETWORK_STATUS_CHECK_INTERVAL);
    
    // 初始网络状态检查
    checkNetworkStatus();
    
    qCInfo(reconnectManager) << "ReconnectManager initialized";
}

ReconnectManager::~ReconnectManager()
{
    stopReconnect();
}

void ReconnectManager::startReconnect(ReconnectTrigger trigger, const QString &reason)
{
    if (_isReconnecting) {
        qCWarning(reconnectManager) << "Reconnect already in progress";
        return;
    }
    
    if (!_networkAvailable) {
        qCWarning(reconnectManager) << "Network not available, delaying reconnect";
        // 等待网络可用
        return;
    }
    
    _isReconnecting = true;
    _currentAttempt = 0;
    _currentTrigger = trigger;
    _currentReason = reason;
    _reconnectStartTime = QDateTime::currentDateTime();
    
    qCInfo(reconnectManager) << "Starting reconnect due to:" << reason;
    LogManager::instance()->writeConnectionLog("RECONNECT_STARTED", 
        QString("Trigger: %1, Reason: %2").arg(static_cast<int>(trigger)).arg(reason));
    
    emit reconnectStarted(trigger, reason);
    
    // 立即尝试第一次重连
    onReconnectTimer();
}

void ReconnectManager::stopReconnect()
{
    if (!_isReconnecting) {
        return;
    }
    
    _isReconnecting = false;
    _reconnectTimer->stop();
    stopConnectionTimeout();
    
    qCInfo(reconnectManager) << "Reconnect stopped";
    LogManager::instance()->writeConnectionLog("RECONNECT_STOPPED", 
        QString("After %1 attempts").arg(_currentAttempt));
    
    emit reconnectStopped();
}

void ReconnectManager::resetReconnectState()
{
    stopReconnect();
    _currentAttempt = 0;
    _attemptHistory.clear();
    
    qCInfo(reconnectManager) << "Reconnect state reset";
}

bool ReconnectManager::isReconnecting() const
{
    return _isReconnecting;
}

void ReconnectManager::setMaxAttempts(int maxAttempts)
{
    _maxAttempts = maxAttempts;
}

void ReconnectManager::setBaseInterval(int intervalMs)
{
    _baseInterval = intervalMs;
}

void ReconnectManager::setMaxInterval(int maxIntervalMs)
{
    _maxInterval = maxIntervalMs;
}

void ReconnectManager::setBackoffMultiplier(double multiplier)
{
    _backoffMultiplier = multiplier;
}

void ReconnectManager::setStrategy(ReconnectStrategy strategy)
{
    _strategy = strategy;
}

void ReconnectManager::setConnectionTimeout(int timeoutMs)
{
    _connectionTimeout = timeoutMs;
}

int ReconnectManager::getCurrentAttempt() const
{
    return _currentAttempt;
}

int ReconnectManager::getMaxAttempts() const
{
    return _maxAttempts;
}

int ReconnectManager::getNextInterval() const
{
    return calculateNextInterval();
}

QDateTime ReconnectManager::getLastAttemptTime() const
{
    return _lastAttemptTime;
}

QList<ReconnectManager::ReconnectAttempt> ReconnectManager::getAttemptHistory() const
{
    return _attemptHistory;
}

void ReconnectManager::setNetworkAvailable(bool available)
{
    if (_networkAvailable != available) {
        _networkAvailable = available;
        
        qCInfo(reconnectManager) << "Network status changed:" << (available ? "Available" : "Unavailable");
        LogManager::instance()->writeConnectionLog("NETWORK_STATUS_CHANGED", 
            available ? "Available" : "Unavailable");
        
        emit networkStatusChanged(available);
        
        // 如果网络恢复且需要重连，立即尝试
        if (available && _isReconnecting && !_reconnectTimer->isActive()) {
            onReconnectTimer();
        }
    }
}

bool ReconnectManager::isNetworkAvailable() const
{
    return _networkAvailable;
}

int ReconnectManager::getTotalAttempts() const
{
    return _totalAttempts;
}

int ReconnectManager::getSuccessfulAttempts() const
{
    return _successfulAttempts;
}

double ReconnectManager::getSuccessRate() const
{
    if (_totalAttempts == 0) {
        return 0.0;
    }
    return static_cast<double>(_successfulAttempts) / _totalAttempts * 100.0;
}

qint64 ReconnectManager::getTotalReconnectTime() const
{
    if (!_reconnectStartTime.isValid()) {
        return 0;
    }
    
    QDateTime endTime = _isReconnecting ? QDateTime::currentDateTime() : _lastAttemptTime;
    return _reconnectStartTime.msecsTo(endTime);
}

void ReconnectManager::onReconnectTimer()
{
    if (!_isReconnecting) {
        return;
    }
    
    if (!_networkAvailable) {
        qCWarning(reconnectManager) << "Network not available, postponing reconnect";
        // 等待网络状态检查器重新触发
        return;
    }
    
    _currentAttempt++;
    _totalAttempts++;
    _lastAttemptTime = QDateTime::currentDateTime();
    
    if (_currentAttempt > _maxAttempts) {
        qCWarning(reconnectManager) << "Maximum reconnect attempts reached:" << _maxAttempts;
        LogManager::instance()->writeConnectionLog("MAX_RECONNECT_ATTEMPTS", 
            QString("Reached maximum of %1 attempts").arg(_maxAttempts));
        
        emit maxAttemptsReached();
        stopReconnect();
        return;
    }
    
    int nextInterval = calculateNextInterval();
    
    qCInfo(reconnectManager) << "Reconnect attempt" << _currentAttempt << "of" << _maxAttempts;
    LogManager::instance()->writeConnectionLog("RECONNECT_ATTEMPT", 
        QString("Attempt %1/%2, Next interval: %3ms").arg(_currentAttempt).arg(_maxAttempts).arg(nextInterval));
    
    emit reconnectAttempt(_currentAttempt, _maxAttempts, nextInterval);
    
    // 记录尝试
    ReconnectAttempt attempt;
    attempt.attemptNumber = _currentAttempt;
    attempt.timestamp = _lastAttemptTime;
    attempt.trigger = _currentTrigger;
    attempt.reason = _currentReason;
    attempt.delayMs = nextInterval;
    attempt.successful = false; // 将在连接成功时更新
    
    addAttemptToHistory(attempt);
    
    // 启动连接超时定时器
    startConnectionTimeout();
    
    // 如果不是最后一次尝试，设置下次重连定时器
    if (_currentAttempt < _maxAttempts) {
        _reconnectTimer->start(nextInterval);
    }
}

void ReconnectManager::onConnectionTimeout()
{
    qCWarning(reconnectManager) << "Connection timeout during reconnect attempt" << _currentAttempt;
    LogManager::instance()->writeConnectionLog("RECONNECT_TIMEOUT", 
        QString("Attempt %1 timed out").arg(_currentAttempt));
    
    emit reconnectFailed(_currentAttempt, "Connection timeout");
    
    // 更新历史记录
    if (!_attemptHistory.isEmpty()) {
        _attemptHistory.last().successful = false;
    }
}

void ReconnectManager::onNetworkStatusCheck()
{
    checkNetworkStatus();
}

int ReconnectManager::calculateNextInterval() const
{
    int interval;
    
    switch (_strategy) {
    case FixedInterval:
        interval = _baseInterval;
        break;
        
    case ExponentialBackoff:
        interval = calculateExponentialBackoff();
        break;
        
    case LinearBackoff:
        interval = calculateLinearBackoff();
        break;
        
    case AdaptiveBackoff:
        interval = calculateAdaptiveBackoff();
        break;
        
    default:
        interval = _baseInterval;
        break;
    }
    
    // 限制最大间隔
    return qMin(interval, _maxInterval);
}

int ReconnectManager::calculateExponentialBackoff() const
{
    if (_currentAttempt <= 1) {
        return _baseInterval;
    }
    
    double interval = _baseInterval * qPow(_backoffMultiplier, _currentAttempt - 1);
    return static_cast<int>(qMin(interval, static_cast<double>(_maxInterval)));
}

int ReconnectManager::calculateLinearBackoff() const
{
    return _baseInterval + (_currentAttempt - 1) * 1000; // 每次增加1秒
}

int ReconnectManager::calculateAdaptiveBackoff() const
{
    // 基于成功率的自适应退避
    double successRate = getSuccessRate();
    double multiplier = successRate > 50.0 ? 1.0 : 2.0; // 成功率低时增加退避
    
    double interval = _baseInterval * multiplier * qPow(_backoffMultiplier, _currentAttempt - 1);
    return static_cast<int>(qMin(interval, static_cast<double>(_maxInterval)));
}

void ReconnectManager::addAttemptToHistory(const ReconnectAttempt &attempt)
{
    _attemptHistory.append(attempt);
    
    // 限制历史记录大小
    while (_attemptHistory.size() > MAX_ATTEMPT_HISTORY) {
        _attemptHistory.removeFirst();
    }
}

void ReconnectManager::checkNetworkStatus()
{
    bool hasActiveInterface = false;
    
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &interface : interfaces) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            interface.flags().testFlag(QNetworkInterface::IsRunning) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            hasActiveInterface = true;
            break;
        }
    }
    
    setNetworkAvailable(hasActiveInterface);
}

void ReconnectManager::startConnectionTimeout()
{
    _connectionTimeoutTimer->start(_connectionTimeout);
}

void ReconnectManager::stopConnectionTimeout()
{
    _connectionTimeoutTimer->stop();
}
