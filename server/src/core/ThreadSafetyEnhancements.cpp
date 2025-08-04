#include "ThreadSafetyEnhancements.h"
#include <QDebug>
#include <QCoreApplication>
#include <QThread>
#include <algorithm>
#include <unordered_set>

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
    m_deadlockTimer->start(1000); // 每秒检查一次
    
    qCInfo(threadSafety) << "LockWaitMonitor initialized";
}

void LockWaitMonitor::registerLockAcquire(const QString& lockName, QThread* thread)
{
    std::unique_lock<std::shared_mutex> lock(m_lockInfoMutex);
    
    LockInfo& info = m_lockInfo[lockName];
    info.lockName = lockName;
    info.owner = thread;
    info.acquiredTime = QDateTime::currentDateTime();
    
    // 移除等待记录
    auto& threadLocks = m_threadLocks[thread];
    threadLocks.removeAll(lockName);
    threadLocks.append(lockName);
}

void LockWaitMonitor::registerLockRelease(const QString& lockName, QThread* thread)
{
    std::unique_lock<std::shared_mutex> lock(m_lockInfoMutex);
    
    auto it = m_lockInfo.find(lockName);
    if (it != m_lockInfo.end() && it->second.owner == thread) {
        m_lockInfo.erase(it);
    }
    
    auto& threadLocks = m_threadLocks[thread];
    threadLocks.removeAll(lockName);
    if (threadLocks.isEmpty()) {
        m_threadLocks.erase(thread);
    }
}

void LockWaitMonitor::registerLockWait(const QString& lockName, QThread* thread)
{
    std::shared_lock<std::shared_mutex> lock(m_lockInfoMutex);
    
    auto it = m_lockInfo.find(lockName);
    if (it != m_lockInfo.end()) {
        QElapsedTimer timer;
        timer.start();
        
        // 记录等待开始时间
        LockInfo waitInfo;
        waitInfo.lockName = lockName + "_wait";
        waitInfo.owner = thread;
        waitInfo.waitTimer = timer;
        waitInfo.acquiredTime = QDateTime::currentDateTime();
        
        lock.unlock();
        std::unique_lock<std::shared_mutex> writeLock(m_lockInfoMutex);
        m_lockInfo[waitInfo.lockName] = waitInfo;
    }
}

void LockWaitMonitor::performDeadlockCheck()
{
    std::shared_lock<std::shared_mutex> lock(m_lockInfoMutex);
    
    // 构建等待图
    std::unordered_map<QThread*, QThread*> waitGraph;
    
    for (const auto& [lockName, info] : m_lockInfo) {
        if (lockName.endsWith("_wait")) {
            QString originalLock = lockName.left(lockName.length() - 5);
            auto ownerIt = m_lockInfo.find(originalLock);
            if (ownerIt != m_lockInfo.end()) {
                waitGraph[info.owner] = ownerIt->second.owner;
            }
            
            // 检查长时间等待
            if (info.waitTimer.elapsed() > m_maxWaitTime) {
                emit longWaitDetected(originalLock, info.waitTimer.elapsed());
            }
        }
    }
    
    // 检测死锁环
    std::unordered_set<QThread*> visited;
    std::unordered_set<QThread*> recursionStack;
    
    for (const auto& [thread, _] : waitGraph) {
        if (visited.find(thread) == visited.end()) {
            if (detectDeadlockCycle(thread, waitGraph, visited, recursionStack)) {
                QStringList involvedThreads;
                for (QThread* t : recursionStack) {
                    involvedThreads << QString("Thread_%1").arg(reinterpret_cast<quintptr>(t));
                }
                emit deadlockDetected(involvedThreads);
                break;
            }
        }
    }
}

bool LockWaitMonitor::detectDeadlockCycle(QThread* startThread, 
                                        const std::unordered_map<QThread*, QThread*>& waitGraph,
                                        std::unordered_set<QThread*>& visited,
                                        std::unordered_set<QThread*>& recursionStack)
{
    visited.insert(startThread);
    recursionStack.insert(startThread);
    
    auto it = waitGraph.find(startThread);
    if (it != waitGraph.end()) {
        QThread* nextThread = it->second;
        
        if (recursionStack.find(nextThread) != recursionStack.end()) {
            return true; // 找到环
        }
        
        if (visited.find(nextThread) == visited.end()) {
            if (detectDeadlockCycle(nextThread, waitGraph, visited, recursionStack)) {
                return true;
            }
        }
    }
    
    recursionStack.erase(startThread);
    return false;
}

// ============================================================================
// SmartRWLock 实现
// ============================================================================

SmartRWLock::SmartRWLock(const QString& name)
    : m_name(name)
    , m_monitor(LockWaitMonitor::instance())
{
}

SmartRWLock::~SmartRWLock()
{
    m_monitor->registerLockRelease(m_name, QThread::currentThread());
}

void SmartRWLock::lockForRead()
{
    m_monitor->registerLockWait(m_name, QThread::currentThread());
    m_stats.readWaits.fetchAndAddOrdered(1);
    
    m_lock.lockForRead();
    
    m_monitor->registerLockAcquire(m_name + "_read", QThread::currentThread());
    m_stats.readLocks.fetchAndAddOrdered(1);
}

void SmartRWLock::lockForWrite()
{
    m_monitor->registerLockWait(m_name, QThread::currentThread());
    m_stats.writeWaits.fetchAndAddOrdered(1);
    
    m_lock.lockForWrite();
    
    m_monitor->registerLockAcquire(m_name + "_write", QThread::currentThread());
    m_stats.writeLocks.fetchAndAddOrdered(1);
}

bool SmartRWLock::tryLockForRead(int timeout)
{
    bool success = m_lock.tryLockForRead(timeout);
    if (success) {
        m_monitor->registerLockAcquire(m_name + "_read", QThread::currentThread());
        m_stats.readLocks.fetchAndAddOrdered(1);
    } else {
        m_stats.timeouts.fetchAndAddOrdered(1);
    }
    return success;
}

bool SmartRWLock::tryLockForWrite(int timeout)
{
    bool success = m_lock.tryLockForWrite(timeout);
    if (success) {
        m_monitor->registerLockAcquire(m_name + "_write", QThread::currentThread());
        m_stats.writeLocks.fetchAndAddOrdered(1);
    } else {
        m_stats.timeouts.fetchAndAddOrdered(1);
    }
    return success;
}

void SmartRWLock::unlock()
{
    QThread* currentThread = QThread::currentThread();
    m_monitor->registerLockRelease(m_name + "_read", currentThread);
    m_monitor->registerLockRelease(m_name + "_write", currentThread);
    m_lock.unlock();
}

// ============================================================================
// ConnectionPoolEnhancer 实现
// ============================================================================

ConnectionPoolEnhancer::ConnectionPoolEnhancer(QObject *parent)
    : QObject(parent)
    , m_circuitTimer(new QTimer(this))
{
    connect(m_circuitTimer, &QTimer::timeout, this, &ConnectionPoolEnhancer::checkCircuitBreaker);
    m_circuitTimer->start(5000); // 每5秒检查一次
}

bool ConnectionPoolEnhancer::shouldAllocateConnection() const
{
    if (m_circuitState.load() == CircuitState::Open) {
        return false;
    }
    
    int active = m_activeConnections.loadAcquire();
    return active < m_config.maxConnections;
}

bool ConnectionPoolEnhancer::shouldRejectRequest() const
{
    if (m_circuitState.load() == CircuitState::Open) {
        return true;
    }
    
    if (m_config.enableBackpressure) {
        double load = getCurrentLoad();
        return load > m_config.backpressureThreshold;
    }
    
    return false;
}

void ConnectionPoolEnhancer::recordConnectionSuccess()
{
    m_consecutiveFailures.storeRelease(0);
    
    if (m_circuitState.load() == CircuitState::HalfOpen) {
        m_circuitState.store(CircuitState::Closed);
        emit circuitBreakerClosed();
        qCInfo(threadSafety) << "Circuit breaker closed - connection successful";
    }
}

void ConnectionPoolEnhancer::recordConnectionFailure()
{
    int failures = m_consecutiveFailures.fetchAndAddOrdered(1) + 1;
    
    if (failures >= m_config.circuitBreakerThreshold && 
        m_circuitState.load() == CircuitState::Closed) {
        m_circuitState.store(CircuitState::Open);
        m_circuitOpenTime = QDateTime::currentDateTime();
        emit circuitBreakerOpened();
        qCWarning(threadSafety) << "Circuit breaker opened - too many failures:" << failures;
    }
}

void ConnectionPoolEnhancer::recordConnectionTimeout()
{
    recordConnectionFailure(); // 超时也视为失败
}

double ConnectionPoolEnhancer::getCurrentLoad() const
{
    int active = m_activeConnections.loadAcquire();
    int pending = m_pendingRequests.loadAcquire();
    return static_cast<double>(active + pending) / m_config.maxConnections;
}

void ConnectionPoolEnhancer::checkCircuitBreaker()
{
    if (m_circuitState.load() == CircuitState::Open) {
        QDateTime now = QDateTime::currentDateTime();
        if (m_circuitOpenTime.msecsTo(now) >= m_config.circuitBreakerTimeout) {
            m_circuitState.store(CircuitState::HalfOpen);
            qCInfo(threadSafety) << "Circuit breaker moved to half-open state";
        }
    }
    
    // 检查系统负载
    double load = getCurrentLoad();
    if (load > m_config.backpressureThreshold) {
        emit backpressureActivated();
    }
    
    if (load > 0.95) {
        emit poolOverloaded();
    }
}

// ============================================================================
// SSLSessionManager 实现
// ============================================================================

SSLSessionManager* SSLSessionManager::s_instance = nullptr;
QMutex SSLSessionManager::s_instanceMutex;

SSLSessionManager* SSLSessionManager::instance()
{
    if (!s_instance) {
        QMutexLocker locker(&s_instanceMutex);
        if (!s_instance) {
            s_instance = new SSLSessionManager();
        }
    }
    return s_instance;
}

SSLSessionManager::SSLSessionManager(QObject *parent)
    : QObject(parent)
    , m_sessionsLock("SSLSessions")
    , m_cleanupTimer(new QTimer(this))
{
    connect(m_cleanupTimer, &QTimer::timeout, this, &SSLSessionManager::performCleanup);
    m_cleanupTimer->start(300000); // 每5分钟清理一次
    
    qCInfo(threadSafety) << "SSLSessionManager initialized";
}

bool SSLSessionManager::storeSession(const QByteArray& sessionId, const QByteArray& sessionData)
{
    m_sessionsLock.lockForWrite();
    
    // 检查是否超过最大会话数
    if (m_sessions.size() >= m_maxSessions) {
        // 移除最旧的会话
        auto oldest = std::min_element(m_sessions.begin(), m_sessions.end(),
            [](const SessionInfo& a, const SessionInfo& b) {
                return a.lastUsed < b.lastUsed;
            });
        if (oldest != m_sessions.end()) {
            m_sessions.erase(oldest);
        }
    }
    
    SessionInfo info;
    info.sessionId = sessionId;
    info.sessionData = sessionData;
    info.createdTime = QDateTime::currentDateTime();
    info.lastUsed = info.createdTime;
    
    m_sessions[sessionId] = info;
    m_sessionsLock.unlock();
    
    emit sessionStored(sessionId);
    return true;
}

QByteArray SSLSessionManager::retrieveSession(const QByteArray& sessionId)
{
    m_sessionsLock.lockForRead();
    
    auto it = m_sessions.find(sessionId);
    if (it != m_sessions.end()) {
        SessionInfo& info = it.value();
        
        // 检查是否过期
        QDateTime now = QDateTime::currentDateTime();
        if (info.createdTime.secsTo(now) < m_sessionTimeout) {
            info.lastUsed = now;
            info.useCount.fetchAndAddOrdered(1);
            
            QByteArray data = info.sessionData;
            m_sessionsLock.unlock();
            
            emit sessionReused(sessionId);
            return data;
        }
    }
    
    m_sessionsLock.unlock();
    return QByteArray();
}

void SSLSessionManager::removeSession(const QByteArray& sessionId)
{
    m_sessionsLock.lockForWrite();
    m_sessions.remove(sessionId);
    m_sessionsLock.unlock();
}

void SSLSessionManager::performCleanup()
{
    m_sessionsLock.lockForWrite();
    
    QDateTime now = QDateTime::currentDateTime();
    auto it = m_sessions.begin();
    
    while (it != m_sessions.end()) {
        if (it->createdTime.secsTo(now) >= m_sessionTimeout) {
            it = m_sessions.erase(it);
        } else {
            ++it;
        }
    }
    
    m_sessionsLock.unlock();
    
    qCDebug(threadSafety) << "SSL session cleanup completed, remaining sessions:" << m_sessions.size();
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
    m_rateTimer->start(1000); // 每秒更新速率
    
    qCInfo(threadSafety) << "BackpressureController initialized with max queue size:" << maxQueueSize;
}

bool BackpressureController::canEnqueue() const
{
    BackpressureLevel level = getCurrentLevel();
    
    switch (level) {
    case BackpressureLevel::Normal:
    case BackpressureLevel::Warning:
        return true;
    case BackpressureLevel::Critical:
        return QRandomGenerator::global()->bounded(100) < 50; // 50%概率拒绝
    case BackpressureLevel::Emergency:
        return false;
    }
    
    return true;
}

void BackpressureController::onMessageEnqueued()
{
    m_stats.currentSize.fetchAndAddOrdered(1);
    m_messagesLastSecond.fetchAndAddOrdered(1);
    
    if (m_stats.currentSize.loadAcquire() > m_maxQueueSize) {
        emit queueOverflow();
    }
}

void BackpressureController::onMessageProcessed()
{
    m_stats.currentSize.fetchAndSubOrdered(1);
    m_processedLastSecond.fetchAndAddOrdered(1);
}

void BackpressureController::onMessageDropped()
{
    m_stats.droppedMessages.fetchAndAddOrdered(1);
    emit messageDropped();
}

BackpressureController::BackpressureLevel BackpressureController::getCurrentLevel() const
{
    return calculateLevel();
}

void BackpressureController::setThresholds(double warningThreshold, double criticalThreshold, double emergencyThreshold)
{
    m_warningThreshold = warningThreshold;
    m_criticalThreshold = criticalThreshold;
    m_emergencyThreshold = emergencyThreshold;
}

void BackpressureController::updateRates()
{
    // 更新处理速率
    int arrival = m_messagesLastSecond.fetchAndStoreOrdered(0);
    int processed = m_processedLastSecond.fetchAndStoreOrdered(0);
    
    m_stats.arrivalRate.storeRelease(arrival);
    m_stats.processingRate.storeRelease(processed);
    
    // 检查背压级别变化
    BackpressureLevel newLevel = calculateLevel();
    static BackpressureLevel lastLevel = BackpressureLevel::Normal;
    
    if (newLevel != lastLevel) {
        emit backpressureLevelChanged(newLevel);
        lastLevel = newLevel;
    }
}

BackpressureController::BackpressureLevel BackpressureController::calculateLevel() const
{
    double utilization = static_cast<double>(m_stats.currentSize.loadAcquire()) / m_maxQueueSize;
    
    if (utilization >= m_emergencyThreshold) {
        return BackpressureLevel::Emergency;
    } else if (utilization >= m_criticalThreshold) {
        return BackpressureLevel::Critical;
    } else if (utilization >= m_warningThreshold) {
        return BackpressureLevel::Warning;
    } else {
        return BackpressureLevel::Normal;
    }
}

// ============================================================================
// AtomicStatsCounter 实现
// ============================================================================

void AtomicStatsCounter::updateResponseTime(int responseTime)
{
    m_stats.totalResponseTime.fetch_add(responseTime, std::memory_order_relaxed);
    m_stats.responseCount.fetch_add(1, std::memory_order_relaxed);
    
    // 更新最大响应时间
    int currentMax = m_stats.maxResponseTime.load(std::memory_order_relaxed);
    while (responseTime > currentMax) {
        if (m_stats.maxResponseTime.compare_exchange_weak(currentMax, responseTime, std::memory_order_relaxed)) {
            break;
        }
    }
}

AtomicStatsCounter::Stats AtomicStatsCounter::getSnapshot() const
{
    Stats snapshot;
    snapshot.totalMessages.store(m_stats.totalMessages.load(std::memory_order_acquire));
    snapshot.processedMessages.store(m_stats.processedMessages.load(std::memory_order_acquire));
    snapshot.failedMessages.store(m_stats.failedMessages.load(std::memory_order_acquire));
    snapshot.totalConnections.store(m_stats.totalConnections.load(std::memory_order_acquire));
    snapshot.activeConnections.store(m_stats.activeConnections.load(std::memory_order_acquire));
    snapshot.authenticatedConnections.store(m_stats.authenticatedConnections.load(std::memory_order_acquire));
    snapshot.totalResponseTime.store(m_stats.totalResponseTime.load(std::memory_order_acquire));
    snapshot.responseCount.store(m_stats.responseCount.load(std::memory_order_acquire));
    snapshot.maxResponseTime.store(m_stats.maxResponseTime.load(std::memory_order_acquire));
    
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