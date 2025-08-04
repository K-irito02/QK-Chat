#ifndef THREAD_SAFETY_ENHANCEMENTS_H
#define THREAD_SAFETY_ENHANCEMENTS_H

#include <QObject>
#include <QMutex>
#include <QReadWriteLock>
#include <QHash>
#include <QQueue>
#include <QTimer>
#include <QAtomicInt>
#include <QAtomicPointer>
#include <QLoggingCategory>
#include <QElapsedTimer>
#include <QThread>
#include <QWaitCondition>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QRandomGenerator>
#include <memory>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <vector>
#include <functional>

Q_DECLARE_LOGGING_CATEGORY(threadSafety)

/**
 * @brief 锁等待监控器 - 检测锁等待时间和死锁
 */
class LockWaitMonitor : public QObject
{
    Q_OBJECT

public:
    struct LockInfo {
        QString lockName;
        QThread* owner;
        QDateTime acquiredTime;
        QElapsedTimer waitTimer;
        std::stack<QString> callStack;
    };

    static LockWaitMonitor* instance();
    
    void registerLockAcquire(const QString& lockName, QThread* thread);
    void registerLockRelease(const QString& lockName, QThread* thread);
    void registerLockWait(const QString& lockName, QThread* thread);
    void checkDeadlock();
    void setMaxWaitTime(int milliseconds) { m_maxWaitTime = milliseconds; }
    
    // 统计信息
    QJsonObject getStatistics() const;

signals:
    void deadlockDetected(const QStringList& involvedThreads);
    void longWaitDetected(const QString& lockName, int waitTime);

private slots:
    void performDeadlockCheck();

private:
    explicit LockWaitMonitor(QObject *parent = nullptr);
    
    static LockWaitMonitor* s_instance;
    static QMutex s_instanceMutex;
    
    mutable std::shared_mutex m_lockInfoMutex;
    std::unordered_map<QString, LockInfo> m_lockInfo;
    std::unordered_map<QThread*, QStringList> m_threadLocks;
    
    QTimer* m_deadlockTimer;
    int m_maxWaitTime{5000}; // 5秒
    
    bool detectDeadlockCycle(QThread* startThread, 
                           const std::unordered_map<QThread*, QThread*>& waitGraph,
                           std::unordered_set<QThread*>& visited,
                           std::unordered_set<QThread*>& recursionStack);
    void updateWaitGraph(const QString& lockName, QThread* thread);
    void updateStatistics(const QString& lockName, qint64 waitTime);
};

/**
 * @brief 智能读写锁 - 带监控和死锁检测的读写锁
 */
class SmartRWLock
{
public:
    explicit SmartRWLock(const QString& name);
    ~SmartRWLock();
    
    void lockForRead();
    void lockForWrite();
    bool tryLockForRead(int timeout = 0);
    bool tryLockForWrite(int timeout = 0);
    void unlock();
    
    // 统计信息
    struct Stats {
        QAtomicInt readLocks{0};
        QAtomicInt writeLocks{0};
        QAtomicInt readWaits{0};
        QAtomicInt writeWaits{0};
        QAtomicInt timeouts{0};
    };
    Stats getStats() const;
    
    // 锁包装器类
    class ReadLocker {
    public:
        explicit ReadLocker(SmartRWLock* lock) : m_lock(lock) {
            if (m_lock) m_lock->lockForRead();
        }
        ~ReadLocker() {
            if (m_lock) m_lock->unlock();
        }
        void unlock() {
            if (m_lock) {
                m_lock->unlock();
                m_lock = nullptr;
            }
        }
    private:
        SmartRWLock* m_lock;
    };
    
    class WriteLocker {
    public:
        explicit WriteLocker(SmartRWLock* lock) : m_lock(lock) {
            if (m_lock) m_lock->lockForWrite();
        }
        ~WriteLocker() {
            if (m_lock) m_lock->unlock();
        }
        void unlock() {
            if (m_lock) {
                m_lock->unlock();
                m_lock = nullptr;
            }
        }
    private:
        SmartRWLock* m_lock;
    };

private:
    QString m_name;
    QReadWriteLock m_lock;
    mutable Stats m_stats;
    LockWaitMonitor* m_monitor;
};

/**
 * @brief 无锁客户端连接管理器
 */
template<typename KeyType, typename ValueType>
class LockFreeClientManager
{
public:
    using ClientPtr = std::shared_ptr<ValueType>;
    
    LockFreeClientManager() = default;
    ~LockFreeClientManager() = default;
    
    // 添加客户端
    bool addClient(const KeyType& key, ClientPtr client);
    
    // 移除客户端
    bool removeClient(const KeyType& key);
    
    // 获取客户端
    ClientPtr getClient(const KeyType& key) const;
    
    // 获取所有客户端（快照）
    std::vector<ClientPtr> getAllClients() const;
    
    // 批量操作
    void forEachClient(std::function<void(const KeyType&, ClientPtr)> func) const;
    
    // 统计信息
    size_t size() const;
    bool empty() const;

private:
    struct Node {
        KeyType key;
        ClientPtr client;
        std::atomic<Node*> next{nullptr};
        std::atomic<bool> deleted{false};
        
        Node(const KeyType& k, ClientPtr c) : key(k), client(c) {}
    };
    
    mutable std::shared_mutex m_mutex;
    std::atomic<Node*> m_head{nullptr};
    std::atomic<size_t> m_size{0};
    
    // 内存管理
    mutable std::queue<std::unique_ptr<Node>> m_deletedNodes;
    mutable std::mutex m_deletedMutex;
    
    void cleanupDeletedNodes() const;
};

/**
 * @brief 连接池增强器 - 解决数据库连接池阻塞问题
 */
class ConnectionPoolEnhancer : public QObject
{
    Q_OBJECT

public:
    struct PoolConfig {
        int minConnections{2};
        int maxConnections{20};
        int maxWaitTime{5000};           // 最大等待时间(ms)
        int warmupConnections{5};        // 预热连接数
        int circuitBreakerThreshold{10}; // 熔断器阈值
        int circuitBreakerTimeout{30000}; // 熔断器超时(ms)
        bool enableBackpressure{true};   // 背压机制
        double backpressureThreshold{0.8}; // 背压阈值
    };
    
    enum class CircuitState {
        Closed,    // 正常状态
        Open,      // 熔断状态
        HalfOpen   // 半开状态
    };

    explicit ConnectionPoolEnhancer(QObject *parent = nullptr);
    
    void setConfig(const PoolConfig& config) { m_config = config; }
    bool acquireConnection(int timeoutMs);
    void releaseConnection();
    bool shouldAllocateConnection() const;
    bool shouldRejectRequest() const;
    void recordConnectionSuccess();
    void recordConnectionFailure();
    void recordConnectionTimeout();
    
    CircuitState getCircuitState() const { return m_circuitState; }
    double getCurrentLoad() const;
    
    // 统计信息
    struct PoolStats {
        QAtomicInt totalRequests{0};
        QAtomicInt successfulRequests{0};
        QAtomicInt failedRequests{0};
        QAtomicInt timeouts{0};
        QAtomicInt backpressureDrops{0};
        QAtomicInt currentQueueSize{0};
        QAtomicInt acquiredConnections{0};
        CircuitState circuitBreakerState{CircuitState::Closed};
    };
    PoolStats getStats() const;

signals:
    void circuitBreakerOpened();
    void circuitBreakerClosed();
    void backpressureActivated();
    void poolOverloaded();

private slots:
    void checkCircuitBreaker();

private:
    PoolConfig m_config;
    std::atomic<CircuitState> m_circuitState{CircuitState::Closed};
    QAtomicInt m_consecutiveFailures{0};
    QAtomicInt m_activeConnections{0};
    QAtomicInt m_pendingRequests{0};
    QDateTime m_circuitOpenTime;
    QTimer* m_circuitTimer;
    
    // 统计变量
    QAtomicInt m_totalRequests{0};
    QAtomicInt m_successfulRequests{0};
    QAtomicInt m_failedRequests{0};
    QAtomicInt m_timeouts{0};
    QAtomicInt m_backpressureDrops{0};
    QAtomicInt m_queueSize{0};
    QAtomicInt m_acquiredConnections{0};
    std::chrono::steady_clock::time_point m_lastFailureTime;
};

/**
 * @brief SSL会话缓存管理器 - 优化SSL握手性能
 */
class SSLSessionManager : public QObject
{
    Q_OBJECT

public:
    struct SessionInfo {
        QByteArray sessionId;
        QByteArray sessionData;
        QDateTime createdTime;
        QDateTime lastUsed;
        QAtomicInt useCount{0};
    };

    static SSLSessionManager* instance();
    
    bool storeSession(const QByteArray& sessionId, const QByteArray& sessionData);
    QByteArray retrieveSession(const QByteArray& sessionId);
    void removeSession(const QByteArray& sessionId);
    void cleanupExpiredSessions();
    
    void setMaxSessions(int maxSessions) { m_maxSessions = maxSessions; }
    void setSessionTimeout(int timeoutSeconds) { m_sessionTimeout = timeoutSeconds; }
    
    // 配置结构
    struct SSLConfig {
        int maxCacheSize{1000};
        std::chrono::milliseconds sessionTimeout{std::chrono::hours(1)};
    };
    void setConfig(const SSLConfig& config);
    
    // 统计信息
    struct SSLStats {
        QAtomicInt totalSessions{0};
        QAtomicInt reusedSessions{0};
        QAtomicInt expiredSessions{0};
        QAtomicInt cacheHits{0};
        QAtomicInt cacheMisses{0};
    };
    SSLStats getStats() const;

signals:
    void sessionStored(const QByteArray& sessionId);
    void sessionReused(const QByteArray& sessionId);

private slots:
    void performCleanup();

private:
    explicit SSLSessionManager(QObject *parent = nullptr);
    
    static SSLSessionManager* s_instance;
    static QMutex s_instanceMutex;
    
    mutable SmartRWLock m_lock;
    QHash<QString, SessionInfo> m_sessionCache;
    QTimer* m_cleanupTimer;
    
    SSLConfig m_config;
    int m_maxSessions{1000};
    int m_sessionTimeout{3600}; // 1小时
    
    // 统计变量
    QAtomicInt m_totalSessions{0};
    QAtomicInt m_reusedSessions{0};
    QAtomicInt m_expiredSessions{0};
};

/**
 * @brief 消息队列背压控制器
 */
class BackpressureController : public QObject
{
    Q_OBJECT

public:
    enum class BackpressureLevel {
        Normal = 0,
        Warning = 1,
        Critical = 2,
        Emergency = 3
    };

    struct QueueStats {
        QAtomicInt currentSize{0};
        QAtomicInt maxSize{0};
        QAtomicInt droppedMessages{0};
        QAtomicInt processingRate{0}; // 每秒处理数
        QAtomicInt arrivalRate{0};    // 每秒到达数
    };

    explicit BackpressureController(int maxQueueSize, QObject *parent = nullptr);
    
    bool canEnqueue() const;
    void onMessageEnqueued();
    void onMessageProcessed();
    void onMessageDropped();
    
    BackpressureLevel getCurrentLevel() const;
    QueueStats getStats() const { return m_stats; }
    
    void setThresholds(double warningThreshold, double criticalThreshold, double emergencyThreshold);

signals:
    void backpressureLevelChanged(BackpressureLevel level);
    void queueOverflow();
    void messageDropped();

private slots:
    void updateRates();
    void updateArrivalRate();
    void updateProcessingRate();

private:
    int m_maxQueueSize;
    QueueStats m_stats;
    
    // 阈值配置
    double m_warningThreshold{0.7};
    double m_criticalThreshold{0.85};
    double m_emergencyThreshold{0.95};
    
    // 速率计算
    QTimer* m_rateTimer;
    QAtomicInt m_messagesLastSecond{0};
    QAtomicInt m_processedLastSecond{0};
    
    // 速率统计
    QAtomicInt m_arrivalCount{0};
    QAtomicInt m_processingCount{0};
    QDateTime m_lastArrivalUpdate;
    QDateTime m_lastProcessingUpdate;
    
    // 当前级别
    std::atomic<BackpressureLevel> m_currentLevel{BackpressureLevel::Normal};
    
    BackpressureLevel calculateLevel() const;
};

/**
 * @brief 原子统计计数器 - 解决统计信息竞态条件
 */
class AtomicStatsCounter
{
public:
    struct Stats {
        std::atomic<qint64> totalMessages{0};
        std::atomic<qint64> processedMessages{0};
        std::atomic<qint64> failedMessages{0};
        std::atomic<qint64> totalConnections{0};
        std::atomic<qint64> activeConnections{0};
        std::atomic<qint64> authenticatedConnections{0};
        
        // 性能统计
        std::atomic<qint64> totalResponseTime{0};
        std::atomic<qint64> responseCount{0};
        std::atomic<int> maxResponseTime{0};
    };
    
    struct StatsSnapshot {
        qint64 totalMessages{0};
        qint64 processedMessages{0};
        qint64 failedMessages{0};
        qint64 totalConnections{0};
        qint64 activeConnections{0};
        qint64 authenticatedConnections{0};
        
        // 性能统计
        qint64 totalResponseTime{0};
        qint64 responseCount{0};
        int maxResponseTime{0};
    };

    AtomicStatsCounter() = default;
    
    void incrementMessages() { m_stats.totalMessages.fetch_add(1, std::memory_order_relaxed); }
    void incrementProcessed() { m_stats.processedMessages.fetch_add(1, std::memory_order_relaxed); }
    void incrementFailed() { m_stats.failedMessages.fetch_add(1, std::memory_order_relaxed); }
    void incrementConnections() { m_stats.totalConnections.fetch_add(1, std::memory_order_relaxed); }
    void incrementActive() { m_stats.activeConnections.fetch_add(1, std::memory_order_relaxed); }
    void decrementActive() { m_stats.activeConnections.fetch_sub(1, std::memory_order_relaxed); }
    void incrementAuthenticated() { m_stats.authenticatedConnections.fetch_add(1, std::memory_order_relaxed); }
    void decrementAuthenticated() { m_stats.authenticatedConnections.fetch_sub(1, std::memory_order_relaxed); }
    
    void updateResponseTime(int responseTime);
    
    // 获取快照（原子读取）
    StatsSnapshot getSnapshot() const;
    void reset();

private:
    Stats m_stats;
};

// 模板实现
template<typename KeyType, typename ValueType>
bool LockFreeClientManager<KeyType, ValueType>::addClient(const KeyType& key, ClientPtr client)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    // 检查是否已存在
    Node* current = m_head.load();
    while (current) {
        if (!current->deleted.load() && current->key == key) {
            return false; // 已存在
        }
        current = current->next.load();
    }
    
    // 创建新节点
    auto newNode = std::make_unique<Node>(key, client);
    Node* rawNode = newNode.release();
    
    // 插入到链表头部
    rawNode->next.store(m_head.load());
    m_head.store(rawNode);
    m_size.fetch_add(1);
    
    return true;
}

template<typename KeyType, typename ValueType>
bool LockFreeClientManager<KeyType, ValueType>::removeClient(const KeyType& key)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    
    Node* current = m_head.load();
    while (current) {
        if (!current->deleted.load() && current->key == key) {
            current->deleted.store(true);
            m_size.fetch_sub(1);
            
            // 延迟删除节点
            {
                std::lock_guard<std::mutex> deletedLock(m_deletedMutex);
                m_deletedNodes.emplace(current);
            }
            
            return true;
        }
        current = current->next.load();
    }
    
    return false;
}

template<typename KeyType, typename ValueType>
auto LockFreeClientManager<KeyType, ValueType>::getClient(const KeyType& key) const -> ClientPtr
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    Node* current = m_head.load();
    while (current) {
        if (!current->deleted.load() && current->key == key) {
            return current->client;
        }
        current = current->next.load();
    }
    
    return nullptr;
}

template<typename KeyType, typename ValueType>
std::vector<typename LockFreeClientManager<KeyType, ValueType>::ClientPtr> 
LockFreeClientManager<KeyType, ValueType>::getAllClients() const
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    
    std::vector<ClientPtr> result;
    Node* current = m_head.load();
    
    while (current) {
        if (!current->deleted.load()) {
            result.push_back(current->client);
        }
        current = current->next.load();
    }
    
    cleanupDeletedNodes();
    return result;
}

#endif // THREAD_SAFETY_ENHANCEMENTS_H