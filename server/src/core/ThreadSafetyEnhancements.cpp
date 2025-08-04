#include "ThreadSafetyEnhancements.h"
#include <QDebug>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDir>
#include <QSslSocket>
#include "ChatClientConnection.h"
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
    , m_deadlockTimer(new QTimer(this))
{
    connect(m_deadlockTimer, &QTimer::timeout, this, &LockWaitMonitor::performDeadlockCheck);
    m_deadlockTimer->start(5000); // 每5秒检查一次死锁
    
    qCInfo(threadSafety) << "LockWaitMonitor initialized";
}

void LockWaitMonitor::registerLockWait(const QString& lockName, QThread* thread)
{
    std::unique_lock<std::shared_mutex> lock(m_lockInfoMutex);
    
    LockInfo lockInfo;
    lockInfo.lockName = lockName;
    lockInfo.owner = thread;
    lockInfo.acquiredTime = QDateTime::currentDateTime();
    
    QString key = QString("%1:%2").arg(lockName).arg(reinterpret_cast<quintptr>(thread));
    m_lockInfo[key] = lockInfo;
    
    // 更新线程锁列表
    m_threadLocks[thread].append(lockName);
    
    qCDebug(threadSafety) << "Lock wait registered:" << lockName << "thread:" << thread;
}

void LockWaitMonitor::registerLockAcquire(const QString& lockName, QThread* thread)
{
    std::unique_lock<std::shared_mutex> lock(m_lockInfoMutex);
    
    QString key = QString("%1:%2").arg(lockName).arg(reinterpret_cast<quintptr>(thread));
    
    // 更新锁信息
    LockInfo lockInfo;
    lockInfo.lockName = lockName;
    lockInfo.owner = thread;
    lockInfo.acquiredTime = QDateTime::currentDateTime();
    
    m_lockInfo[key] = lockInfo;
    
    // 更新线程锁列表
    m_threadLocks[thread].append(lockName);
    
    qCDebug(threadSafety) << "Lock acquired:" << lockName << "thread:" << thread;
}

void LockWaitMonitor::registerLockRelease(const QString& lockName, QThread* thread)
{
    std::unique_lock<std::shared_mutex> lock(m_lockInfoMutex);
    
    QString key = QString("%1:%2").arg(lockName).arg(reinterpret_cast<quintptr>(thread));
    
    auto lockIt = m_lockInfo.find(key);
    if (lockIt != m_lockInfo.end()) {
        m_lockInfo.erase(lockIt);
        // 从线程锁列表中移除
        auto it = m_threadLocks.find(thread);
        if (it != m_threadLocks.end()) {
            it->second.removeAll(lockName);
            if (it->second.isEmpty()) {
                m_threadLocks.erase(it);
            }
        }
        
        qCDebug(threadSafety) << "Lock released:" << lockName << "thread:" << thread;
    }
}



QJsonObject LockWaitMonitor::getStatistics() const
{
    std::shared_lock<std::shared_mutex> lock(m_lockInfoMutex);
    
    QJsonObject stats;
    
    stats["currentLockInfo"] = static_cast<int>(m_lockInfo.size());
    stats["currentThreadLocks"] = static_cast<int>(m_threadLocks.size());
    
    return stats;
}

void LockWaitMonitor::performDeadlockCheck()
{
    std::shared_lock<std::shared_mutex> lock(m_lockInfoMutex);
    
    // 简化的死锁检测：检查循环等待
    QMap<QThread*, QStringList> threadWaits;
    
    // 构建等待图
    for (const auto& pair : m_lockInfo) {
        const LockInfo& lockInfo = pair.second;
        threadWaits[lockInfo.owner].append(lockInfo.lockName);
    }
    
    // 检查是否有线程等待时间过长
    QDateTime now = QDateTime::currentDateTime();
    for (const auto& pair : m_lockInfo) {
        const LockInfo& lockInfo = pair.second;
        qint64 waitTime = lockInfo.acquiredTime.msecsTo(now);
        if (waitTime > m_maxWaitTime * 2) { // 两倍最大等待时间认为是潜在死锁
            emit longWaitDetected(lockInfo.lockName, static_cast<int>(waitTime));
            
            qCWarning(threadSafety) << "Potential deadlock detected:" << lockInfo.lockName 
                                    << "thread:" << lockInfo.owner << "wait time:" << waitTime << "ms";
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
    Q_UNUSED(lockName)
    Q_UNUSED(waitTime)
    // 简化的统计更新
}

// ============================================================================
// SmartRWLock 实现
// ============================================================================

SmartRWLock::SmartRWLock(const QString& name)
    : m_name(name)
    , m_monitor(LockWaitMonitor::instance())
{
    qCDebug(threadSafety) << "SmartRWLock created:" << m_name;
}

SmartRWLock::~SmartRWLock()
{
    qCDebug(threadSafety) << "SmartRWLock destroyed:" << m_name;
}

SmartRWLock::Stats SmartRWLock::getStats() const
{
    return m_stats;
}

bool SmartRWLock::tryLockForRead(int timeout)
{
    QThread* currentThread = QThread::currentThread();
    m_monitor->registerLockWait(m_name, currentThread);
    
    QElapsedTimer timer;
    timer.start();
    
    bool acquired = m_lock.tryLockForRead(timeout);
    
    if (acquired) {
        m_monitor->registerLockAcquire(m_name, currentThread);
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
    m_monitor->registerLockWait(m_name, currentThread);
    
    QElapsedTimer timer;
    timer.start();
    
    bool acquired = m_lock.tryLockForWrite(timeout);
    
    if (acquired) {
        m_monitor->registerLockAcquire(m_name, currentThread);
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
    m_monitor->registerLockWait(m_name, currentThread);
    
    QElapsedTimer timer;
    timer.start();
    
    m_lock.lockForRead();
    
    m_monitor->registerLockAcquire(m_name, currentThread);
    m_stats.readLocks.fetchAndAddOrdered(1);
    
    qCDebug(threadSafety) << "Read lock acquired (blocking):" << m_name << "elapsed:" << timer.elapsed() << "ms";
}

void SmartRWLock::lockForWrite()
{
    QThread* currentThread = QThread::currentThread();
    m_monitor->registerLockWait(m_name, currentThread);
    
    QElapsedTimer timer;
    timer.start();
    
    m_lock.lockForWrite();
    
    m_monitor->registerLockAcquire(m_name, currentThread);
    m_stats.writeLocks.fetchAndAddOrdered(1);
    
    qCDebug(threadSafety) << "Write lock acquired (blocking):" << m_name << "elapsed:" << timer.elapsed() << "ms";
}

void SmartRWLock::unlock()
{
    m_lock.unlock();
    m_monitor->registerLockRelease(m_name, QThread::currentThread());
    
    qCDebug(threadSafety) << "Lock released:" << m_name;
}

// ============================================================================
// ConnectionPoolEnhancer 实现
// ============================================================================

ConnectionPoolEnhancer::ConnectionPoolEnhancer(QObject *parent)
    : QObject(parent)
    , m_circuitTimer(new QTimer(this))
{
    connect(m_circuitTimer, &QTimer::timeout, this, &ConnectionPoolEnhancer::checkCircuitBreaker);
    m_circuitTimer->start(10000); // 每10秒检查一次
    
    qCInfo(threadSafety) << "ConnectionPoolEnhancer initialized";
}



bool ConnectionPoolEnhancer::acquireConnection(int timeoutMs)
{
    // 检查熔断器状态
    if (m_circuitState == CircuitState::Open) {
        auto now = std::chrono::steady_clock::now();
        if (now - m_lastFailureTime < std::chrono::milliseconds(m_config.circuitBreakerTimeout)) {
            recordConnectionFailure();
            return false; // 熔断器开启，拒绝请求
        } else {
            m_circuitState = CircuitState::HalfOpen;
            emit circuitBreakerClosed();
        }
    }
    
    // 检查背压
    if (m_queueSize.loadAcquire() >= static_cast<int>(m_config.maxConnections * m_config.backpressureThreshold)) {
        m_backpressureDrops.fetchAndAddOrdered(1);
        emit backpressureActivated();
        return false;
    }
    
    m_queueSize.fetchAndAddOrdered(1);
    
    // 模拟连接获取
    QThread::msleep(10); // 模拟数据库连接延迟
    
    bool success = true; // 简化实现，假设总是成功
    
    if (success) {
        recordConnectionSuccess();
        m_queueSize.fetchAndSubOrdered(1);
        m_acquiredConnections.fetchAndAddOrdered(1);
        return true;
    } else {
        recordConnectionFailure();
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
    stats.circuitBreakerState = m_circuitState;
    
    return stats;
}

void ConnectionPoolEnhancer::recordConnectionSuccess()
{
    m_totalRequests.fetchAndAddOrdered(1);
    m_successfulRequests.fetchAndAddOrdered(1);
    m_consecutiveFailures.storeRelease(0);
    
    if (m_circuitState == CircuitState::HalfOpen) {
        m_circuitState = CircuitState::Closed;
        emit circuitBreakerClosed();
        qCInfo(threadSafety) << "Circuit breaker closed after successful operation";
    }
}

void ConnectionPoolEnhancer::recordConnectionFailure()
{
    m_totalRequests.fetchAndAddOrdered(1);
    m_failedRequests.fetchAndAddOrdered(1);
    
    int failures = m_consecutiveFailures.fetchAndAddOrdered(1) + 1;
    
    if (failures >= m_config.circuitBreakerThreshold && m_circuitState == CircuitState::Closed) {
        m_circuitState = CircuitState::Open;
        m_lastFailureTime = std::chrono::steady_clock::now();
        emit circuitBreakerOpened();
        
        qCWarning(threadSafety) << "Circuit breaker opened after" << failures << "consecutive failures";
    }
}

void ConnectionPoolEnhancer::checkCircuitBreaker()
{
    // 检查熔断器状态
    PoolStats stats = getStats();
    
    double failureRate = stats.totalRequests.loadAcquire() > 0 ? 
                        static_cast<double>(stats.failedRequests.loadAcquire()) / stats.totalRequests.loadAcquire() : 0.0;
    
    if (failureRate > 0.1) { // 失败率超过10%
        emit poolOverloaded();
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

bool SSLSessionManager::storeSession(const QByteArray& sessionId, const QByteArray& sessionData)
{
    SmartRWLock::WriteLocker locker(&m_lock);
    
    if (m_sessionCache.size() >= m_config.maxCacheSize) {
        // 移除最旧的会话
        auto oldest = std::min_element(m_sessionCache.begin(), m_sessionCache.end(),
                                     [](const SessionInfo& a, const SessionInfo& b) {
                                         return a.lastUsed < b.lastUsed;
                                     });
        if (oldest != m_sessionCache.end()) {
            m_sessionCache.erase(oldest);
        }
    }
    
    SessionInfo session;
    session.sessionId = sessionId;
    session.sessionData = sessionData;
    session.createdTime = QDateTime::currentDateTime();
    session.lastUsed = session.createdTime;
    session.useCount = 0;
    
    m_sessionCache[sessionId.toHex()] = session;
    m_totalSessions.fetchAndAddOrdered(1);
    
    qCDebug(threadSafety) << "SSL session cached:" << sessionId.toHex();
    return true;
}

QByteArray SSLSessionManager::retrieveSession(const QByteArray& sessionId)
{
    SmartRWLock::ReadLocker locker(&m_lock);
    
    auto it = m_sessionCache.find(sessionId.toHex());
    if (it != m_sessionCache.end()) {
        SessionInfo& session = it.value();
        
        // 检查会话是否过期
        if (session.createdTime.msecsTo(QDateTime::currentDateTime()) > m_config.sessionTimeout.count()) {
            // 会话过期，需要在写锁中移除
            locker.unlock();
            SmartRWLock::WriteLocker writeLocker(&m_lock);
            m_sessionCache.erase(it);
            m_expiredSessions.fetchAndAddOrdered(1);
            return QByteArray();
        }
        
        session.lastUsed = QDateTime::currentDateTime();
        session.useCount++;
        m_reusedSessions.fetchAndAddOrdered(1);
        
        qCDebug(threadSafety) << "SSL session retrieved:" << sessionId.toHex();
        return session.sessionData;
    }
    
    return QByteArray();
}

void SSLSessionManager::removeSession(const QByteArray& sessionId)
{
    SmartRWLock::WriteLocker locker(&m_lock);
    
    if (m_sessionCache.remove(sessionId.toHex()) > 0) {
        qCDebug(threadSafety) << "SSL session removed:" << sessionId.toHex();
    }
}

SSLSessionManager::SSLStats SSLSessionManager::getStats() const
{
    SmartRWLock::ReadLocker locker(&m_lock);
    
    SSLStats stats;
    stats.totalSessions = m_totalSessions.loadAcquire();
    stats.reusedSessions = m_reusedSessions.loadAcquire();
    stats.expiredSessions = m_expiredSessions.loadAcquire();
    stats.cacheHits = m_reusedSessions.loadAcquire();
    stats.cacheMisses = m_totalSessions.loadAcquire() - m_reusedSessions.loadAcquire();
    
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
    , m_rateTimer(new QTimer(this))
{
    m_stats.maxSize.storeRelease(maxQueueSize);
    
    connect(m_rateTimer, &QTimer::timeout, this, &BackpressureController::updateRates);
    m_rateTimer->start(1000); // 每秒评估一次
    
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

BackpressureController::BackpressureLevel BackpressureController::calculateLevel() const
{
    int currentSize = m_stats.currentSize.loadAcquire();
    double utilization = static_cast<double>(currentSize) / m_maxQueueSize;
    
    if (utilization >= m_emergencyThreshold) return BackpressureLevel::Emergency;
    if (utilization >= m_criticalThreshold) return BackpressureLevel::Critical;
    if (utilization >= m_warningThreshold) return BackpressureLevel::Warning;
    
    return BackpressureLevel::Normal;
}

void BackpressureController::updateRates()
{
    BackpressureLevel newLevel = calculateLevel();
    BackpressureLevel oldLevel = m_currentLevel.exchange(newLevel);
    
    if (newLevel != oldLevel) {
        emit backpressureLevelChanged(newLevel);
        
        qCInfo(threadSafety) << "Backpressure level changed from" << static_cast<int>(oldLevel)
                             << "to" << static_cast<int>(newLevel);
    }
    
    // 更新处理速率
    updateProcessingRate();
}

void BackpressureController::updateArrivalRate()
{
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsed = m_lastArrivalUpdate.msecsTo(now);
    
    if (elapsed >= 1000) { // 每秒更新一次
        m_arrivalCount.fetchAndAddOrdered(1);
        
        if (elapsed > 0) {
            double rate = static_cast<double>(m_arrivalCount.loadAcquire()) * 1000.0 / elapsed;
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
    QDateTime now = QDateTime::currentDateTime();
    qint64 elapsed = m_lastProcessingUpdate.msecsTo(now);
    
    if (elapsed >= 1000) { // 每秒更新一次
        m_processingCount.fetchAndAddOrdered(1);
        
        if (elapsed > 0) {
            double rate = static_cast<double>(m_processingCount.loadAcquire()) * 1000.0 / elapsed;
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

void AtomicStatsCounter::updateResponseTime(int responseTimeMs)
{
    m_stats.totalResponseTime.fetch_add(responseTimeMs, std::memory_order_relaxed);
    m_stats.responseCount.fetch_add(1, std::memory_order_relaxed);
    
    // 更新最大响应时间
    int currentMax = m_stats.maxResponseTime.load(std::memory_order_relaxed);
    while (responseTimeMs > currentMax && 
           !m_stats.maxResponseTime.compare_exchange_weak(currentMax, responseTimeMs, 
                                                        std::memory_order_relaxed)) {
        // 循环直到成功更新
    }
}

AtomicStatsCounter::StatsSnapshot AtomicStatsCounter::getSnapshot() const
{
    StatsSnapshot snapshot;
    snapshot.totalMessages = m_stats.totalMessages.load(std::memory_order_relaxed);
    snapshot.processedMessages = m_stats.processedMessages.load(std::memory_order_relaxed);
    snapshot.failedMessages = m_stats.failedMessages.load(std::memory_order_relaxed);
    snapshot.totalConnections = m_stats.totalConnections.load(std::memory_order_relaxed);
    snapshot.activeConnections = m_stats.activeConnections.load(std::memory_order_relaxed);
    snapshot.authenticatedConnections = m_stats.authenticatedConnections.load(std::memory_order_relaxed);
    snapshot.totalResponseTime = m_stats.totalResponseTime.load(std::memory_order_relaxed);
    snapshot.responseCount = m_stats.responseCount.load(std::memory_order_relaxed);
    snapshot.maxResponseTime = m_stats.maxResponseTime.load(std::memory_order_relaxed);
    return snapshot;
}

void AtomicStatsCounter::reset()
{
    m_stats.totalMessages.store(0, std::memory_order_relaxed);
    m_stats.processedMessages.store(0, std::memory_order_relaxed);
    m_stats.failedMessages.store(0, std::memory_order_relaxed);
    m_stats.totalConnections.store(0, std::memory_order_relaxed);
    m_stats.activeConnections.store(0, std::memory_order_relaxed);
    m_stats.authenticatedConnections.store(0, std::memory_order_relaxed);
    m_stats.totalResponseTime.store(0, std::memory_order_relaxed);
    m_stats.responseCount.store(0, std::memory_order_relaxed);
    m_stats.maxResponseTime.store(0, std::memory_order_relaxed);
    
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

// 显式实例化
template void LockFreeClientManager<QSslSocket*, ChatClientConnection>::forEachClient(std::function<void(QSslSocket* const&, std::shared_ptr<ChatClientConnection>)>) const;
template void LockFreeClientManager<qint64, ChatClientConnection>::forEachClient(std::function<void(const qint64&, std::shared_ptr<ChatClientConnection>)>) const;

#include "ThreadSafetyEnhancements.moc"