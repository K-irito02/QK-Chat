#include "ThreadSafetyEnhancements.h"
#include <QDebug>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDir>
#include <algorithm>

Q_LOGGING_CATEGORY(threadSafety, "qkchat.server.threadsafety")

// ============================================================================
// LockWaitMonitor 实现
// ============================================================================

LockWaitMonitor* LockWaitMonitor::s_instance = nullptr;
QMutex LockWaitMonitor::s_instanceMutex;

LockWaitMonitor* LockWaitMonitor::instance()
{
    if (!s_instance) {
        QMutexLocker locker(&s_instanceMutex);
        if (!s_instance) {
            s_instance = new LockWaitMonitor();
        }
    }
    return s_instance;
}

LockWaitMonitor::LockWaitMonitor(QObject *parent)
    : QObject(parent)
    , m_detectionTimer(new QTimer(this))
{
    connect(m_detectionTimer, &QTimer::timeout, this, &LockWaitMonitor::performDeadlockDetection);
    m_detectionTimer->start(5000); // 每5秒检查一次死锁
    
    qCInfo(threadSafety) << "LockWaitMonitor initialized";
}

void LockWaitMonitor::recordLockWait(const QString& lockName, QThread* thread, LockType type)
{
    QMutexLocker locker(&m_mutex);
    
    WaitInfo waitInfo;
    waitInfo.lockName = lockName;
    waitInfo.thread = thread;
    waitInfo.startTime = QDateTime::currentDateTime();
    waitInfo.lockType = type;
    
    QString key = QString("%1:%2").arg(lockName).arg(reinterpret_cast<quintptr>(thread));
    m_waitingThreads[key] = waitInfo;
    
    updateWaitGraph(lockName, thread);
    
    qCDebug(threadSafety) << "Lock wait recorded:" << lockName << "thread:" << thread << "type:" << static_cast<int>(type);
}

void LockWaitMonitor::recordLockAcquired(const QString& lockName, QThread* thread, LockType type)
{
    QMutexLocker locker(&m_mutex);
    
    QString key = QString("%1:%2").arg(lockName).arg(reinterpret_cast<quintptr>(thread));
    
    auto it = m_waitingThreads.find(key);
    if (it != m_waitingThreads.end()) {
        WaitInfo waitInfo = it.value();
        m_waitingThreads.erase(it);
        
        // 计算等待时间
        qint64 waitTime = waitInfo.startTime.msecsTo(QDateTime::currentDateTime());
        
        // 记录锁持有信息
        LockInfo lockInfo;
        lockInfo.lockName = lockName;
        lockInfo.thread = thread;
        lockInfo.lockType = type;
        lockInfo.acquiredTime = QDateTime::currentDateTime();
        
        m_heldLocks[key] = lockInfo;
        
        // 更新统计
        updateStatistics(lockName, waitTime);
        
        if (waitTime > m_maxWaitTime) {
            emit longWaitDetected(lockName, thread, waitTime);
            qCWarning(threadSafety) << "Long wait detected:" << lockName << "wait time:" << waitTime << "ms";
        }
        
        qCDebug(threadSafety) << "Lock acquired:" << lockName << "thread:" << thread << "wait time:" << waitTime << "ms";
    }
}

void LockWaitMonitor::recordLockReleased(const QString& lockName, QThread* thread)
{
    QMutexLocker locker(&m_mutex);
    
    QString key = QString("%1:%2").arg(lockName).arg(reinterpret_cast<quintptr>(thread));
    
    if (m_heldLocks.remove(key) > 0) {
        qCDebug(threadSafety) << "Lock released:" << lockName << "thread:" << thread;
    }
}

void LockWaitMonitor::setMaxWaitTime(int maxWaitTimeMs)
{
    m_maxWaitTime = maxWaitTimeMs;
    qCInfo(threadSafety) << "Max wait time set to:" << maxWaitTimeMs << "ms";
}

QJsonObject LockWaitMonitor::getStatistics() const
{
    QMutexLocker locker(&m_mutex);
    
    QJsonObject stats;
    
    QJsonObject lockStats;
    for (auto it = m_lockStats.begin(); it != m_lockStats.end(); ++it) {
        QJsonObject lockStat;
        lockStat["totalWaits"] = it.value().totalWaits.loadAcquire();
        lockStat["totalWaitTime"] = static_cast<qint64>(it.value().totalWaitTime.loadAcquire());
        lockStat["maxWaitTime"] = static_cast<qint64>(it.value().maxWaitTime.loadAcquire());
        lockStat["averageWaitTime"] = it.value().totalWaits.loadAcquire() > 0 ? 
                                      static_cast<qint64>(it.value().totalWaitTime.loadAcquire()) / it.value().totalWaits.loadAcquire() : 0;
        lockStats[it.key()] = lockStat;
    }
    
    stats["lockStatistics"] = lockStats;
    stats["currentWaitingThreads"] = m_waitingThreads.size();
    stats["currentHeldLocks"] = m_heldLocks.size();
    stats["deadlockDetections"] = static_cast<qint64>(m_deadlockCount.loadAcquire());
    
    return stats;
}

void LockWaitMonitor::performDeadlockDetection()
{
    QMutexLocker locker(&m_mutex);
    
    // 简化的死锁检测：检查循环等待
    QMap<QThread*, QStringList> threadWaits;
    
    // 构建等待图
    for (const WaitInfo& wait : m_waitingThreads) {
        threadWaits[wait.thread].append(wait.lockName);
    }
    
    // 检查是否有线程等待时间过长
    QDateTime now = QDateTime::currentDateTime();
    for (const WaitInfo& wait : m_waitingThreads) {
        qint64 waitTime = wait.startTime.msecsTo(now);
        if (waitTime > m_maxWaitTime * 2) { // 两倍最大等待时间认为是潜在死锁
            emit potentialDeadlockDetected(wait.lockName, wait.thread);
            m_deadlockCount.fetchAndAddOrdered(1);
            
            qCWarning(threadSafety) << "Potential deadlock detected:" << wait.lockName 
                                    << "thread:" << wait.thread << "wait time:" << waitTime << "ms";
        }
    }
}

void LockWaitMonitor::updateWaitGraph(const QString& lockName, QThread* thread)
{
    // 简化的等待图更新
    Q_UNUSED(lockName)
    Q_UNUSED(thread)
    // 这里可以实现更复杂的死锁检测算法
}

void LockWaitMonitor::updateStatistics(const QString& lockName, qint64 waitTime)
{
    if (!m_lockStats.contains(lockName)) {
        m_lockStats[lockName] = LockStatistics();
    }
    
    LockStatistics& stats = m_lockStats[lockName];
    stats.totalWaits.fetchAndAddOrdered(1);
    stats.totalWaitTime.fetchAndAddOrdered(waitTime);
    
    qint64 currentMax = stats.maxWaitTime.loadAcquire();
    while (waitTime > currentMax && !stats.maxWaitTime.testAndSetOrdered(currentMax, waitTime)) {
        currentMax = stats.maxWaitTime.loadAcquire();
    }
}

// ============================================================================
// SmartRWLock 实现
// ============================================================================

SmartRWLock::SmartRWLock(const QString& name, QObject *parent)
    : QObject(parent)
    , m_name(name)
    , m_monitor(LockWaitMonitor::instance())
{
    qCDebug(threadSafety) << "SmartRWLock created:" << m_name;
}

bool SmartRWLock::tryLockForRead(int timeout)
{
    QThread* currentThread = QThread::currentThread();
    m_monitor->recordLockWait(m_name, currentThread, LockWaitMonitor::LockType::ReadLock);
    
    QElapsedTimer timer;
    timer.start();
    
    bool acquired = m_lock.tryLockForRead(timeout);
    
    if (acquired) {
        m_monitor->recordLockAcquired(m_name, currentThread, LockWaitMonitor::LockType::ReadLock);
        m_stats.readLocks.fetchAndAddOrdered(1);
        
        qCDebug(threadSafety) << "Read lock acquired:" << m_name << "elapsed:" << timer.elapsed() << "ms";
    } else {
        m_stats.readWaits.fetchAndAddOrdered(1);
        m_stats.timeouts.fetchAndAddOrdered(1);
        
        qCWarning(threadSafety) << "Read lock timeout:" << m_name << "timeout:" << timeout << "ms";
    }
    
    return acquired;
}

bool SmartRWLock::tryLockForWrite(int timeout)
{
    QThread* currentThread = QThread::currentThread();
    m_monitor->recordLockWait(m_name, currentThread, LockWaitMonitor::LockType::WriteLock);
    
    QElapsedTimer timer;
    timer.start();
    
    bool acquired = m_lock.tryLockForWrite(timeout);
    
    if (acquired) {
        m_monitor->recordLockAcquired(m_name, currentThread, LockWaitMonitor::LockType::WriteLock);
        m_stats.writeLocks.fetchAndAddOrdered(1);
        
        qCDebug(threadSafety) << "Write lock acquired:" << m_name << "elapsed:" << timer.elapsed() << "ms";
    } else {
        m_stats.writeWaits.fetchAndAddOrdered(1);
        m_stats.timeouts.fetchAndAddOrdered(1);
        
        qCWarning(threadSafety) << "Write lock timeout:" << m_name << "timeout:" << timeout << "ms";
    }
    
    return acquired;
}

void SmartRWLock::lockForRead()
{
    QThread* currentThread = QThread::currentThread();
    m_monitor->recordLockWait(m_name, currentThread, LockWaitMonitor::LockType::ReadLock);
    
    QElapsedTimer timer;
    timer.start();
    
    m_lock.lockForRead();
    
    m_monitor->recordLockAcquired(m_name, currentThread, LockWaitMonitor::LockType::ReadLock);
    m_stats.readLocks.fetchAndAddOrdered(1);
    
    qCDebug(threadSafety) << "Read lock acquired (blocking):" << m_name << "elapsed:" << timer.elapsed() << "ms";
}

void SmartRWLock::lockForWrite()
{
    QThread* currentThread = QThread::currentThread();
    m_monitor->recordLockWait(m_name, currentThread, LockWaitMonitor::LockType::WriteLock);
    
    QElapsedTimer timer;
    timer.start();
    
    m_lock.lockForWrite();
    
    m_monitor->recordLockAcquired(m_name, currentThread, LockWaitMonitor::LockType::WriteLock);
    m_stats.writeLocks.fetchAndAddOrdered(1);
    
    qCDebug(threadSafety) << "Write lock acquired (blocking):" << m_name << "elapsed:" << timer.elapsed() << "ms";
}

void SmartRWLock::unlock()
{
    m_lock.unlock();
    m_monitor->recordLockReleased(m_name, QThread::currentThread());
    
    qCDebug(threadSafety) << "Lock released:" << m_name;
}

SmartRWLock::LockStats SmartRWLock::getStats() const
{
    return m_stats;
}

// ============================================================================
// ConnectionPoolEnhancer 实现
// ============================================================================

ConnectionPoolEnhancer::ConnectionPoolEnhancer(QObject *parent)
    : QObject(parent)
    , m_checkTimer(new QTimer(this))
    , m_circuitBreakerState(CircuitBreakerState::Closed)
{
    connect(m_checkTimer, &QTimer::timeout, this, &ConnectionPoolEnhancer::performHealthCheck);
    m_checkTimer->start(10000); // 每10秒检查一次
    
    qCInfo(threadSafety) << "ConnectionPoolEnhancer initialized";
}

void ConnectionPoolEnhancer::setConfig(const PoolConfig& config)
{
    m_config = config;
    qCInfo(threadSafety) << "Pool config updated - failure threshold:" << config.failureThreshold
                         << "timeout:" << config.timeout.count() << "ms";
}

bool ConnectionPoolEnhancer::acquireConnection(int timeoutMs)
{
    // 检查熔断器状态
    if (m_circuitBreakerState == CircuitBreakerState::Open) {
        auto now = std::chrono::steady_clock::now();
        if (now - m_lastFailureTime < m_config.timeout) {
            recordFailure();
            return false; // 熔断器开启，拒绝请求
        } else {
            m_circuitBreakerState = CircuitBreakerState::HalfOpen;
            emit circuitBreakerStateChanged(m_circuitBreakerState);
        }
    }
    
    // 检查背压
    if (m_queueSize.loadAcquire() >= m_config.backpressureThreshold) {
        m_backpressureDrops.fetchAndAddOrdered(1);
        emit backpressureActivated();
        return false;
    }
    
    m_queueSize.fetchAndAddOrdered(1);
    
    // 模拟连接获取
    QThread::msleep(10); // 模拟数据库连接延迟
    
    bool success = true; // 简化实现，假设总是成功
    
    if (success) {
        recordSuccess();
        m_queueSize.fetchAndSubOrdered(1);
        m_acquiredConnections.fetchAndAddOrdered(1);
        return true;
    } else {
        recordFailure();
        m_queueSize.fetchAndSubOrdered(1);
        return false;
    }
}

void ConnectionPoolEnhancer::releaseConnection()
{
    m_acquiredConnections.fetchAndSubOrdered(1);
    qCDebug(threadSafety) << "Connection released";
}

ConnectionPoolEnhancer::PoolStats ConnectionPoolEnhancer::getStats() const
{
    PoolStats stats;
    stats.totalRequests = m_totalRequests.loadAcquire();
    stats.successfulRequests = m_successfulRequests.loadAcquire();
    stats.failedRequests = m_failedRequests.loadAcquire();
    stats.timeouts = m_timeouts.loadAcquire();
    stats.backpressureDrops = m_backpressureDrops.loadAcquire();
    stats.currentQueueSize = m_queueSize.loadAcquire();
    stats.acquiredConnections = m_acquiredConnections.loadAcquire();
    stats.circuitBreakerState = m_circuitBreakerState;
    
    return stats;
}

void ConnectionPoolEnhancer::recordSuccess()
{
    m_totalRequests.fetchAndAddOrdered(1);
    m_successfulRequests.fetchAndAddOrdered(1);
    m_consecutiveFailures.storeRelease(0);
    
    if (m_circuitBreakerState == CircuitBreakerState::HalfOpen) {
        m_circuitBreakerState = CircuitBreakerState::Closed;
        emit circuitBreakerStateChanged(m_circuitBreakerState);
        qCInfo(threadSafety) << "Circuit breaker closed after successful operation";
    }
}

void ConnectionPoolEnhancer::recordFailure()
{
    m_totalRequests.fetchAndAddOrdered(1);
    m_failedRequests.fetchAndAddOrdered(1);
    
    int failures = m_consecutiveFailures.fetchAndAddOrdered(1) + 1;
    
    if (failures >= m_config.failureThreshold && m_circuitBreakerState == CircuitBreakerState::Closed) {
        m_circuitBreakerState = CircuitBreakerState::Open;
        m_lastFailureTime = std::chrono::steady_clock::now();
        emit circuitBreakerStateChanged(m_circuitBreakerState);
        
        qCWarning(threadSafety) << "Circuit breaker opened after" << failures << "consecutive failures";
    }
}

void ConnectionPoolEnhancer::performHealthCheck()
{
    // 检查连接池健康状态
    PoolStats stats = getStats();
    
    double failureRate = stats.totalRequests > 0 ? 
                        static_cast<double>(stats.failedRequests) / stats.totalRequests : 0.0;
    
    if (failureRate > 0.1) { // 失败率超过10%
        emit poolUnhealthy(failureRate);
        qCWarning(threadSafety) << "Pool unhealthy - failure rate:" << failureRate;
    }
}

// ============================================================================
// SSLSessionManager 实现
// ============================================================================

SSLSessionManager* SSLSessionManager::s_instance = nullptr;

SSLSessionManager* SSLSessionManager::instance()
{
    if (!s_instance) {
        s_instance = new SSLSessionManager();
    }
    return s_instance;
}

SSLSessionManager::SSLSessionManager(QObject *parent)
    : QObject(parent)
    , m_lock("SSLSessionManager")
    , m_cleanupTimer(new QTimer(this))
{
    connect(m_cleanupTimer, &QTimer::timeout, this, &SSLSessionManager::performCleanup);
    m_cleanupTimer->start(300000); // 每5分钟清理一次
    
    qCInfo(threadSafety) << "SSLSessionManager initialized";
}

void SSLSessionManager::setConfig(const SSLConfig& config)
{
    SmartRWLock::WriteLocker locker(&m_lock);
    m_config = config;
    
    qCInfo(threadSafety) << "SSL config updated - cache size:" << config.maxCacheSize
                         << "session timeout:" << config.sessionTimeout.count() << "ms";
}

bool SSLSessionManager::cacheSession(const QString& sessionId, const QByteArray& sessionData)
{
    SmartRWLock::WriteLocker locker(&m_lock);
    
    if (m_sessionCache.size() >= m_config.maxCacheSize) {
        // 移除最旧的会话
        auto oldest = std::min_element(m_sessionCache.begin(), m_sessionCache.end(),
                                     [](const SSLSession& a, const SSLSession& b) {
                                         return a.lastAccessed < b.lastAccessed;
                                     });
        if (oldest != m_sessionCache.end()) {
            m_sessionCache.erase(oldest);
        }
    }
    
    SSLSession session;
    session.sessionId = sessionId;
    session.sessionData = sessionData;
    session.createdTime = QDateTime::currentDateTime();
    session.lastAccessed = session.createdTime;
    session.accessCount = 0;
    
    m_sessionCache[sessionId] = session;
    m_totalSessions.fetchAndAddOrdered(1);
    
    qCDebug(threadSafety) << "SSL session cached:" << sessionId;
    return true;
}

QByteArray SSLSessionManager::retrieveSession(const QString& sessionId)
{
    SmartRWLock::ReadLocker locker(&m_lock);
    
    auto it = m_sessionCache.find(sessionId);
    if (it != m_sessionCache.end()) {
        SSLSession& session = it.value();
        
        // 检查会话是否过期
        if (session.createdTime.msecsTo(QDateTime::currentDateTime()) > m_config.sessionTimeout.count()) {
            // 会话过期，需要在写锁中移除
            locker.unlock();
            SmartRWLock::WriteLocker writeLocker(&m_lock);
            m_sessionCache.erase(it);
            m_expiredSessions.fetchAndAddOrdered(1);
            return QByteArray();
        }
        
        session.lastAccessed = QDateTime::currentDateTime();
        session.accessCount++;
        m_reusedSessions.fetchAndAddOrdered(1);
        
        qCDebug(threadSafety) << "SSL session retrieved:" << sessionId;
        return session.sessionData;
    }
    
    return QByteArray();
}

void SSLSessionManager::removeSession(const QString& sessionId)
{
    SmartRWLock::WriteLocker locker(&m_lock);
    
    if (m_sessionCache.remove(sessionId) > 0) {
        qCDebug(threadSafety) << "SSL session removed:" << sessionId;
    }
}

SSLSessionManager::SSLStats SSLSessionManager::getStats() const
{
    SmartRWLock::ReadLocker locker(&m_lock);
    
    SSLStats stats;
    stats.totalSessions = m_totalSessions.loadAcquire();
    stats.cachedSessions = m_sessionCache.size();
    stats.reusedSessions = m_reusedSessions.loadAcquire();
    stats.expiredSessions = m_expiredSessions.loadAcquire();
    stats.cacheHitRate = stats.totalSessions > 0 ? 
                         static_cast<double>(stats.reusedSessions) / stats.totalSessions : 0.0;
    
    return stats;
}

void SSLSessionManager::performCleanup()
{
    SmartRWLock::WriteLocker locker(&m_lock);
    
    QDateTime now = QDateTime::currentDateTime();
    int cleanedCount = 0;
    
    auto it = m_sessionCache.begin();
    while (it != m_sessionCache.end()) {
        if (it.value().createdTime.msecsTo(now) > m_config.sessionTimeout.count()) {
            it = m_sessionCache.erase(it);
            cleanedCount++;
        } else {
            ++it;
        }
    }
    
    if (cleanedCount > 0) {
        m_expiredSessions.fetchAndAddOrdered(cleanedCount);
        qCDebug(threadSafety) << "SSL session cleanup completed, removed" << cleanedCount << "expired sessions";
    }
}

// ============================================================================
// BackpressureController 实现
// ============================================================================

BackpressureController::BackpressureController(int maxQueueSize, QObject *parent)
    : QObject(parent)
    , m_maxQueueSize(maxQueueSize)
    , m_evaluationTimer(new QTimer(this))
{
    m_stats.maxSize.storeRelease(maxQueueSize);
    
    connect(m_evaluationTimer, &QTimer::timeout, this, &BackpressureController::evaluateBackpressure);
    m_evaluationTimer->start(1000); // 每秒评估一次
    
    qCInfo(threadSafety) << "BackpressureController initialized with max queue size:" << maxQueueSize;
}

bool BackpressureController::canEnqueue() const
{
    BackpressureLevel currentLevel = getCurrentLevel();
    
    switch (currentLevel) {
    case BackpressureLevel::Normal:
        return true;
    case BackpressureLevel::Warning:
        return QRandomGenerator::global()->bounded(100) < 90; // 90% 通过率
    case BackpressureLevel::Critical:
        return QRandomGenerator::global()->bounded(100) < 50; // 50% 通过率
    case BackpressureLevel::Emergency:
        return QRandomGenerator::global()->bounded(100) < 10; // 10% 通过率
    }
    
    return false;
}

void BackpressureController::onMessageEnqueued()
{
    m_stats.currentSize.fetchAndAddOrdered(1);
    updateArrivalRate();
    
    qCDebug(threadSafety) << "Message enqueued, current size:" << m_stats.currentSize.loadAcquire();
}

void BackpressureController::onMessageProcessed()
{
    m_stats.currentSize.fetchAndSubOrdered(1);
    updateProcessingRate();
    
    qCDebug(threadSafety) << "Message processed, current size:" << m_stats.currentSize.loadAcquire();
}

void BackpressureController::onMessageDropped()
{
    m_stats.droppedMessages.fetchAndAddOrdered(1);
    
    qCWarning(threadSafety) << "Message dropped due to backpressure";
}

BackpressureController::BackpressureLevel BackpressureController::getCurrentLevel() const
{
    int currentSize = m_stats.currentSize.loadAcquire();
    double utilization = static_cast<double>(currentSize) / m_maxQueueSize;
    
    if (utilization >= 0.9) return BackpressureLevel::Emergency;
    if (utilization >= 0.7) return BackpressureLevel::Critical;
    if (utilization >= 0.5) return BackpressureLevel::Warning;
    
    return BackpressureLevel::Normal;
}

BackpressureController::BackpressureStats BackpressureController::getStats() const
{
    return m_stats;
}

void BackpressureController::evaluateBackpressure()
{
    BackpressureLevel newLevel = getCurrentLevel();
    BackpressureLevel oldLevel = m_currentLevel.exchange(newLevel);
    
    if (newLevel != oldLevel) {
        emit backpressureLevelChanged(newLevel, oldLevel);
        
        qCInfo(threadSafety) << "Backpressure level changed from" << static_cast<int>(oldLevel)
                             << "to" << static_cast<int>(newLevel);
    }
    
    // 更新处理速率
    updateProcessingRate();
}

void BackpressureController::updateArrivalRate()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastArrivalUpdate);
    
    if (elapsed.count() >= 1000) { // 每秒更新一次
        m_arrivalCount.fetchAndAddOrdered(1);
        
        if (elapsed.count() > 0) {
            double rate = static_cast<double>(m_arrivalCount.loadAcquire()) * 1000.0 / elapsed.count();
            m_stats.arrivalRate.storeRelease(static_cast<int>(rate));
        }
        
        m_arrivalCount.storeRelease(0);
        m_lastArrivalUpdate = now;
    } else {
        m_arrivalCount.fetchAndAddOrdered(1);
    }
}

void BackpressureController::updateProcessingRate()
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastProcessingUpdate);
    
    if (elapsed.count() >= 1000) { // 每秒更新一次
        m_processingCount.fetchAndAddOrdered(1);
        
        if (elapsed.count() > 0) {
            double rate = static_cast<double>(m_processingCount.loadAcquire()) * 1000.0 / elapsed.count();
            m_stats.processingRate.storeRelease(static_cast<int>(rate));
        }
        
        m_processingCount.storeRelease(0);
        m_lastProcessingUpdate = now;
    } else {
        m_processingCount.fetchAndAddOrdered(1);
    }
}

// ============================================================================
// AtomicStatsCounter 实现
// ============================================================================

AtomicStatsCounter::AtomicStatsCounter()
{
    qCDebug(threadSafety) << "AtomicStatsCounter initialized";
}

void AtomicStatsCounter::incrementMessages()
{
    m_snapshot.totalMessages.fetchAndAddOrdered(1);
}

void AtomicStatsCounter::incrementProcessedMessages()
{
    m_snapshot.processedMessages.fetchAndAddOrdered(1);
}

void AtomicStatsCounter::incrementFailedMessages()
{
    m_snapshot.failedMessages.fetchAndAddOrdered(1);
}

void AtomicStatsCounter::incrementConnections()
{
    m_snapshot.totalConnections.fetchAndAddOrdered(1);
    m_snapshot.activeConnections.fetchAndAddOrdered(1);
}

void AtomicStatsCounter::decrementConnections()
{
    m_snapshot.activeConnections.fetchAndSubOrdered(1);
}

void AtomicStatsCounter::incrementAuthenticatedConnections()
{
    m_snapshot.authenticatedConnections.fetchAndAddOrdered(1);
}

void AtomicStatsCounter::decrementAuthenticatedConnections()
{
    m_snapshot.authenticatedConnections.fetchAndSubOrdered(1);
}

void AtomicStatsCounter::updateResponseTime(int responseTimeMs)
{
    m_snapshot.totalResponseTime.fetchAndAddOrdered(responseTimeMs);
    m_snapshot.responseCount.fetchAndAddOrdered(1);
    
    // 更新最大响应时间
    int currentMax = m_snapshot.maxResponseTime.loadAcquire();
    while (responseTimeMs > currentMax && 
           !m_snapshot.maxResponseTime.testAndSetOrdered(currentMax, responseTimeMs)) {
        currentMax = m_snapshot.maxResponseTime.loadAcquire();
    }
}

AtomicStatsCounter::StatsSnapshot AtomicStatsCounter::getSnapshot() const
{
    return m_snapshot;
}

void AtomicStatsCounter::reset()
{
    m_snapshot.totalMessages.storeRelease(0);
    m_snapshot.processedMessages.storeRelease(0);
    m_snapshot.failedMessages.storeRelease(0);
    m_snapshot.totalConnections.storeRelease(0);
    m_snapshot.activeConnections.storeRelease(0);
    m_snapshot.authenticatedConnections.storeRelease(0);
    m_snapshot.totalResponseTime.storeRelease(0);
    m_snapshot.responseCount.storeRelease(0);
    m_snapshot.maxResponseTime.storeRelease(0);
    
    qCDebug(threadSafety) << "AtomicStatsCounter reset";
}

// ============================================================================
// LockFreeClientManager 模板特化实现
// ============================================================================

template<typename KeyType, typename ValueType>
void LockFreeClientManager<KeyType, ValueType>::forEachClient(std::function<void(const KeyType&, ClientPtr)> func) const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    Node* current = m_head.load();
    while (current) {
        if (!current->deleted.load()) {
            func(current->key, current->client);
        }
        current = current->next.load();
    }
}

template<typename KeyType, typename ValueType>
size_t LockFreeClientManager<KeyType, ValueType>::size() const
{
    return m_size.load();
}

template<typename KeyType, typename ValueType>
bool LockFreeClientManager<KeyType, ValueType>::empty() const
{
    return m_size.load() == 0;
}

template<typename KeyType, typename ValueType>
void LockFreeClientManager<KeyType, ValueType>::cleanupDeletedNodes() const
{
    std::lock_guard<std::mutex> lock(m_deletedMutex);
    while (!m_deletedNodes.empty()) {
        m_deletedNodes.pop();
    }
}

#include "ThreadSafetyEnhancements.moc"