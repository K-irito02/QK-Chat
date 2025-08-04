#ifndef ARCHITECTURE_OPTIMIZER_H
#define ARCHITECTURE_OPTIMIZER_H

#include <QObject>
#include <QMutex>
#include <QHash>
#include <QTimer>
#include <QAtomicInt>
#include <QDateTime>
#include <QLoggingCategory>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QHostInfo>
#include <memory>
#include <functional>
#include <atomic>
#include <random>

Q_DECLARE_LOGGING_CATEGORY(architecture)

/**
 * @brief 节点角色枚举
 */
enum class NodeRole {
    Master,      // 主节点
    Slave,       // 从节点
    Standby,     // 备用节点
    Worker       // 工作节点
};

/**
 * @brief 负载均衡策略
 */
enum class LoadBalanceStrategy {
    RoundRobin,      // 轮询
    WeightedRoundRobin, // 加权轮询
    LeastConnections, // 最少连接
    IPHash,          // IP哈希
    ConsistentHash,  // 一致性哈希
    Random           // 随机
};

/**
 * @brief 服务发现策略
 */
enum class ServiceDiscoveryStrategy {
    Static,          // 静态配置
    DNS,             // DNS发现
    Consul,          // Consul服务发现
    Etcd,            // Etcd服务发现
    Multicast        // 组播发现
};

/**
 * @brief 节点信息
 */
struct NodeInfo {
    QString nodeId;
    QString address;
    int port{0};
    NodeRole role{NodeRole::Worker};
    bool isHealthy{true};
    int weight{100};
    double cpuUsage{0.0};
    double memoryUsage{0.0};
    int connectionCount{0};
    QDateTime lastHeartbeat;
    QJsonObject metadata;
};

/**
 * @brief 分片信息
 */
struct ShardInfo {
    QString shardId;
    QString startKey;
    QString endKey;
    QStringList nodeIds;
    int replicationFactor{3};
    bool isAvailable{true};
    QDateTime lastUpdate;
};

/**
 * @brief 集群管理器
 */
class ClusterManager : public QObject
{
    Q_OBJECT

public:
    struct ClusterConfig {
        QString clusterId;
        ServiceDiscoveryStrategy discoveryStrategy{ServiceDiscoveryStrategy::Static};
        LoadBalanceStrategy loadBalanceStrategy{LoadBalanceStrategy::RoundRobin};
        int heartbeatInterval{5000};     // 心跳间隔(ms)
        int nodeTimeout{15000};          // 节点超时(ms)
        int maxRetries{3};               // 最大重试次数
        bool enableAutoFailover{true};   // 自动故障转移
        bool enableLoadRebalancing{true}; // 负载重新平衡
    };

    explicit ClusterManager(QObject *parent = nullptr);
    
    // 集群管理
    void setConfig(const ClusterConfig& config);
    bool initializeCluster();
    void shutdownCluster();
    
    // 节点管理
    void registerNode(const NodeInfo& node);
    void unregisterNode(const QString& nodeId);
    void updateNodeStatus(const QString& nodeId, const NodeInfo& status);
    NodeInfo getNode(const QString& nodeId) const;
    QList<NodeInfo> getAllNodes() const;
    QList<NodeInfo> getHealthyNodes() const;
    
    // 负载均衡
    QString selectNode(const QString& key = QString()) const;
    QStringList selectNodes(int count, const QString& key = QString()) const;
    void updateNodeWeight(const QString& nodeId, int weight);
    
    // 故障转移
    void markNodeFailed(const QString& nodeId);
    void markNodeRecovered(const QString& nodeId);
    bool isClusterHealthy() const;
    
    // 统计信息
    QJsonObject getClusterStatistics() const;
    QJsonObject getNodeStatistics(const QString& nodeId) const;

signals:
    void nodeJoined(const NodeInfo& node);
    void nodeLeft(const QString& nodeId);
    void nodeStatusChanged(const QString& nodeId, bool healthy);
    void masterElected(const QString& nodeId);
    void clusterStateChanged(bool healthy);

private slots:
    void performHeartbeatCheck();
    void performLoadRebalancing();
    void electMaster();

private:
    ClusterConfig m_config;
    QHash<QString, NodeInfo> m_nodes;
    QString m_currentMaster;
    QString m_localNodeId;
    
    mutable QMutex m_nodesMutex;
    QTimer* m_heartbeatTimer;
    QTimer* m_rebalancingTimer;
    
    // 负载均衡相关
    mutable std::atomic<int> m_roundRobinIndex{0};
    mutable std::mt19937 m_randomEngine;
    
    QString calculateNodeId() const;
    void handleNodeFailure(const QString& nodeId);
    void redistributeLoad();
    QString selectNodeByStrategy(const QString& key) const;
    uint32_t hashKey(const QString& key) const;
};

/**
 * @brief 数据分片管理器
 */
class ShardingManager : public QObject
{
    Q_OBJECT

public:
    struct ShardingConfig {
        int virtualNodes{160};           // 虚拟节点数
        int replicationFactor{3};        // 复制因子
        QString hashFunction{"crc32"};   // 哈希函数
        bool enableAutoRebalancing{true}; // 自动重新平衡
        int rebalanceThreshold{20};      // 重新平衡阈值(%)
    };

    explicit ShardingManager(QObject *parent = nullptr);
    
    // 分片管理
    void setConfig(const ShardingConfig& config);
    void initializeShards(const QStringList& nodeIds);
    void addNode(const QString& nodeId);
    void removeNode(const QString& nodeId);
    
    // 数据路由
    QString getShardForKey(const QString& key) const;
    QStringList getNodesForKey(const QString& key) const;
    QStringList getNodesForShard(const QString& shardId) const;
    
    // 分片操作
    ShardInfo getShard(const QString& shardId) const;
    QList<ShardInfo> getAllShards() const;
    QList<ShardInfo> getShardsForNode(const QString& nodeId) const;
    
    // 重新平衡
    void triggerRebalancing();
    bool isRebalancingNeeded() const;
    QJsonObject getRebalancingPlan() const;
    
    // 统计信息
    QJsonObject getShardingStatistics() const;
    double getLoadBalance() const;

signals:
    void shardCreated(const ShardInfo& shard);
    void shardMigrated(const QString& shardId, const QString& fromNode, const QString& toNode);
    void rebalancingStarted();
    void rebalancingCompleted();

private:
    ShardingConfig m_config;
    QHash<QString, ShardInfo> m_shards;
    QHash<uint32_t, QString> m_hashRing; // 一致性哈希环
    
    mutable QMutex m_shardsMutex;
    
    void buildHashRing();
    void redistributeShards();
    uint32_t calculateHash(const QString& key) const;
    QString findShardOnRing(uint32_t hash) const;
    void migrateShardData(const QString& shardId, const QString& fromNode, const QString& toNode);
};

/**
 * @brief 服务注册与发现
 */
class ServiceRegistry : public QObject
{
    Q_OBJECT

public:
    struct ServiceInfo {
        QString serviceId;
        QString serviceName;
        QString address;
        int port{0};
        QJsonObject metadata;
        QDateTime registrationTime;
        QDateTime lastHeartbeat;
        bool isHealthy{true};
        int weight{100};
    };

    explicit ServiceRegistry(QObject *parent = nullptr);
    
    // 服务注册
    void registerService(const ServiceInfo& service);
    void unregisterService(const QString& serviceId);
    void updateServiceHealth(const QString& serviceId, bool healthy);
    void updateServiceMetadata(const QString& serviceId, const QJsonObject& metadata);
    
    // 服务发现
    QList<ServiceInfo> discoverServices(const QString& serviceName) const;
    ServiceInfo getService(const QString& serviceId) const;
    QStringList getServiceNames() const;
    
    // 健康检查
    void performHealthCheck();
    void setHealthCheckInterval(int intervalMs);
    
    // 统计信息
    QJsonObject getRegistryStatistics() const;
    int getServiceCount(const QString& serviceName = QString()) const;

signals:
    void serviceRegistered(const ServiceInfo& service);
    void serviceUnregistered(const QString& serviceId);
    void serviceHealthChanged(const QString& serviceId, bool healthy);

private slots:
    void performPeriodicHealthCheck();
    void cleanupStaleServices();

private:
    QHash<QString, ServiceInfo> m_services;
    QHash<QString, QStringList> m_servicesByName;
    
    mutable QMutex m_servicesMutex;
    QTimer* m_healthCheckTimer;
    QTimer* m_cleanupTimer;
    
    int m_healthCheckInterval{30000}; // 30秒
    int m_serviceTimeout{90000};      // 90秒
    
    bool checkServiceHealth(const ServiceInfo& service) const;
    void removeStaleService(const QString& serviceId);
};

/**
 * @brief 异步日志管理器
 */
class AsyncLogManager : public QObject
{
    Q_OBJECT

public:
    enum class LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };

    struct LogEntry {
        QDateTime timestamp;
        LogLevel level;
        QString category;
        QString message;
        QString thread;
        QString file;
        int line{0};
        QJsonObject context;
    };

    struct LogConfig {
        QString logDirectory{"./logs"};
        QString filePattern{"server_%1.log"};  // %1 = date
        int maxFileSize{100 * 1024 * 1024};    // 100MB
        int maxFiles{30};                       // 保留30个文件
        int flushInterval{1000};                // 1秒刷新
        int bufferSize{10000};                  // 缓冲区大小
        bool enableCompression{true};           // 启用压缩
        LogLevel minLevel{LogLevel::Info};      // 最低级别
    };

    explicit AsyncLogManager(QObject *parent = nullptr);
    ~AsyncLogManager();
    
    // 配置管理
    void setConfig(const LogConfig& config);
    LogConfig getConfig() const { return m_config; }
    
    // 日志记录
    void log(LogLevel level, const QString& category, const QString& message,
             const QString& file = QString(), int line = 0,
             const QJsonObject& context = QJsonObject());
    
    void debug(const QString& category, const QString& message, const QJsonObject& context = QJsonObject());
    void info(const QString& category, const QString& message, const QJsonObject& context = QJsonObject());
    void warning(const QString& category, const QString& message, const QJsonObject& context = QJsonObject());
    void error(const QString& category, const QString& message, const QJsonObject& context = QJsonObject());
    void critical(const QString& category, const QString& message, const QJsonObject& context = QJsonObject());
    
    // 控制
    void start();
    void stop();
    void flush();
    
    // 统计信息
    QJsonObject getStatistics() const;
    qint64 getTotalLogsWritten() const;

signals:
    void logWritten(const LogEntry& entry);
    void logError(const QString& error);

private slots:
    void processLogQueue();
    void performMaintenance();

private:
    LogConfig m_config;
    QQueue<LogEntry> m_logQueue;
    QMutex m_queueMutex;
    QTimer* m_flushTimer;
    QTimer* m_maintenanceTimer;
    
    QString m_currentLogFile;
    QAtomicInt m_totalLogs{0};
    QAtomicInt m_droppedLogs{0};
    bool m_isRunning{false};
    
    void writeToFile(const QList<LogEntry>& entries);
    void rotateLogFile();
    void cleanupOldFiles();
    QString formatLogEntry(const LogEntry& entry) const;
    QString getLogFileName() const;
    void ensureLogDirectory() const;
};

/**
 * @brief 分布式锁管理器
 */
class DistributedLockManager : public QObject
{
    Q_OBJECT

public:
    struct LockInfo {
        QString lockId;
        QString nodeId;
        QDateTime acquiredTime;
        QDateTime expiryTime;
        QString resource;
        QJsonObject metadata;
    };

    explicit DistributedLockManager(QObject *parent = nullptr);
    
    // 锁操作
    bool acquireLock(const QString& resource, int timeoutMs = 5000, int ttlMs = 30000);
    bool tryLock(const QString& resource, int ttlMs = 30000);
    bool releaseLock(const QString& resource);
    bool renewLock(const QString& resource, int ttlMs = 30000);
    
    // 锁查询
    bool isLocked(const QString& resource) const;
    LockInfo getLockInfo(const QString& resource) const;
    QList<LockInfo> getAllLocks() const;
    QList<LockInfo> getNodeLocks(const QString& nodeId) const;
    
    // 管理操作
    void cleanupExpiredLocks();
    void forceReleaseLock(const QString& resource);
    
    // 统计信息
    QJsonObject getLockStatistics() const;

signals:
    void lockAcquired(const QString& resource, const QString& nodeId);
    void lockReleased(const QString& resource, const QString& nodeId);
    void lockExpired(const QString& resource, const QString& nodeId);

private slots:
    void performLockMaintenance();

private:
    QHash<QString, LockInfo> m_locks;
    QString m_nodeId;
    mutable QMutex m_locksMutex;
    QTimer* m_maintenanceTimer;
    
    QString generateLockId() const;
    bool isLockExpired(const LockInfo& lock) const;
};

/**
 * @brief 架构优化器主类
 */
class ArchitectureOptimizer : public QObject
{
    Q_OBJECT

public:
    struct OptimizationConfig {
        bool enableClustering{false};
        bool enableSharding{false};
        bool enableServiceDiscovery{false};
        bool enableAsyncLogging{true};
        bool enableDistributedLocks{false};
        QString nodeRole{"worker"};
        QStringList seedNodes;
    };

    explicit ArchitectureOptimizer(QObject *parent = nullptr);
    ~ArchitectureOptimizer();
    
    // 初始化
    void setConfig(const OptimizationConfig& config);
    bool initialize();
    void shutdown();
    
    // 组件访问
    ClusterManager* clusterManager() const { return m_clusterManager; }
    ShardingManager* shardingManager() const { return m_shardingManager; }
    ServiceRegistry* serviceRegistry() const { return m_serviceRegistry; }
    AsyncLogManager* logManager() const { return m_logManager; }
    DistributedLockManager* lockManager() const { return m_lockManager; }
    
    // 优化建议
    QJsonObject analyzeArchitecture() const;
    QStringList getOptimizationSuggestions() const;
    
    // 统计信息
    QJsonObject getArchitectureStatistics() const;

signals:
    void optimizationApplied(const QString& optimization);
    void architectureChanged(const QJsonObject& newArchitecture);

private:
    OptimizationConfig m_config;
    
    // 组件管理器
    ClusterManager* m_clusterManager;
    ShardingManager* m_shardingManager;
    ServiceRegistry* m_serviceRegistry;
    AsyncLogManager* m_logManager;
    DistributedLockManager* m_lockManager;
    
    void initializeComponents();
    void setupConnections();
    QStringList analyzeBottlenecks() const;
    QStringList analyzeScalability() const;
    QStringList analyzeReliability() const;
};

#endif // ARCHITECTURE_OPTIMIZER_H