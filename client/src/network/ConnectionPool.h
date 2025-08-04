#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <QObject>
#include <QHash>
#include <QQueue>
#include <QTimer>
#include <QMutex>
#include <QLoggingCategory>
#include "NetworkClient.h"

Q_DECLARE_LOGGING_CATEGORY(connectionPool)

/**
 * @brief 连接池管理器
 * 
 * 管理多个网络连接实例，提供：
 * - 连接复用
 * - 负载均衡
 * - 故障转移
 * - 连接健康检查
 */
class ConnectionPool : public QObject
{
    Q_OBJECT
    
public:
    enum LoadBalanceStrategy {
        RoundRobin,     // 轮询
        LeastConnections, // 最少连接
        Random,         // 随机
        HealthBased     // 基于健康状态
    };
    Q_ENUM(LoadBalanceStrategy)
    
    enum ConnectionStatus {
        Available,      // 可用
        Busy,          // 忙碌
        Connecting,    // 连接中
        Disconnected,  // 已断开
        Error          // 错误状态
    };
    Q_ENUM(ConnectionStatus)
    
    struct ConnectionInfo {
        QString id;
        NetworkClient* client;
        ConnectionStatus status;
        QDateTime lastUsed;
        QDateTime created;
        int activeRequests;
        qint64 totalRequests;
        qint64 totalErrors;
        qint64 averageLatency;
        bool isHealthy;
    };
    
    explicit ConnectionPool(QObject *parent = nullptr);
    ~ConnectionPool();
    
    // 连接池管理
    bool initialize(const QString &host, int port, int poolSize = 5);
    void shutdown();
    bool isInitialized() const;
    
    // 连接获取和释放
    NetworkClient* acquireConnection();
    void releaseConnection(NetworkClient* client);
    void releaseConnection(const QString &connectionId);
    
    // 配置
    void setMaxPoolSize(int maxSize);
    void setMinPoolSize(int minSize);
    void setMaxIdleTime(int idleTimeMs);
    void setHealthCheckInterval(int intervalMs);
    void setLoadBalanceStrategy(LoadBalanceStrategy strategy);
    void setMaxRequestsPerConnection(int maxRequests);
    
    // 状态查询
    int getPoolSize() const;
    int getAvailableConnections() const;
    int getBusyConnections() const;
    int getHealthyConnections() const;
    QList<ConnectionInfo> getConnectionInfos() const;
    
    // 统计信息
    qint64 getTotalRequests() const;
    qint64 getTotalErrors() const;
    double getErrorRate() const;
    qint64 getAverageLatency() const;
    
signals:
    void connectionCreated(const QString &connectionId);
    void connectionDestroyed(const QString &connectionId);
    void connectionStatusChanged(const QString &connectionId, ConnectionStatus status);
    void poolSizeChanged(int newSize);
    void healthCheckCompleted(int healthyCount, int totalCount);
    void loadBalanceStrategyChanged(LoadBalanceStrategy strategy);
    
private slots:
    void onConnectionConnected();
    void onConnectionDisconnected();
    void onConnectionError(const QString &error);
    void onHealthCheckTimer();
    void onIdleCheckTimer();
    
private:
    QString generateConnectionId() const;
    NetworkClient* createConnection();
    void destroyConnection(const QString &connectionId);
    void updateConnectionStatus(const QString &connectionId, ConnectionStatus status);
    
    // 负载均衡算法
    NetworkClient* selectConnectionRoundRobin();
    NetworkClient* selectConnectionLeastConnections();
    NetworkClient* selectConnectionRandom();
    NetworkClient* selectConnectionHealthBased();
    
    // 健康检查
    void performHealthCheck();
    void checkConnectionHealth(const QString &connectionId);
    bool isConnectionHealthy(const ConnectionInfo &info) const;
    
    // 连接维护
    void maintainPoolSize();
    void removeIdleConnections();
    void balanceConnections();
    
    QHash<QString, ConnectionInfo> _connections;
    QQueue<QString> _availableConnections;
    mutable QMutex _poolMutex;
    
    QString _serverHost;
    int _serverPort;
    bool _initialized;
    
    // 配置参数
    int _maxPoolSize;
    int _minPoolSize;
    int _maxIdleTime;
    int _healthCheckInterval;
    LoadBalanceStrategy _loadBalanceStrategy;
    int _maxRequestsPerConnection;
    
    // 负载均衡状态
    int _roundRobinIndex;
    
    // 定时器
    QTimer *_healthCheckTimer;
    QTimer *_idleCheckTimer;
    
    // 统计信息
    qint64 _totalRequests;
    qint64 _totalErrors;
    
    // 默认配置
    static constexpr int DEFAULT_MAX_POOL_SIZE = 10;
    static constexpr int DEFAULT_MIN_POOL_SIZE = 2;
    static constexpr int DEFAULT_MAX_IDLE_TIME = 300000; // 5分钟
    static constexpr int DEFAULT_HEALTH_CHECK_INTERVAL = 60000; // 1分钟
    static constexpr int DEFAULT_MAX_REQUESTS_PER_CONNECTION = 1000;
    static constexpr int IDLE_CHECK_INTERVAL = 30000; // 30秒
};

#endif // CONNECTIONPOOL_H
