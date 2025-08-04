#ifndef DATABASEPOOL_H
#define DATABASEPOOL_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <QDateTime>
#include <QElapsedTimer>
#include <memory>
#include <functional>
#include <future>
#include <chrono>
#include <mutex>

Q_DECLARE_LOGGING_CATEGORY(databasePool)

/**
 * @brief 数据库连接信息
 */
struct DatabaseConnection {
    QSqlDatabase database;
    QString connectionName;
    QDateTime lastUsed;
    QAtomicInt inUse{0};
    QAtomicInt queryCount{0};
    bool isValid;
    
    DatabaseConnection() : isValid(false) {}
    
    bool isInUse() const {
        return inUse.loadAcquire() > 0;
    }
    
    void markUsed() {
        inUse.storeRelease(1);
        lastUsed = QDateTime::currentDateTime();
        queryCount.fetchAndAddOrdered(1);
    }
    
    void markFree() {
        inUse.storeRelease(0);
    }
};

/**
 * @brief 数据库操作类型
 */
enum class DatabaseOperationType {
    Read = 0,
    Write = 1,
    Transaction = 2
};

/**
 * @brief 数据库操作结果
 */
template<typename T>
struct DatabaseResult {
    bool success;
    T data;
    QString error;
    int affectedRows;
    QDateTime timestamp;
    
    DatabaseResult() : success(false), affectedRows(0), timestamp(QDateTime::currentDateTime()) {}
    DatabaseResult(const T& result) : success(true), data(result), affectedRows(0), timestamp(QDateTime::currentDateTime()) {}
    DatabaseResult(const QString& errorMsg) : success(false), error(errorMsg), affectedRows(0), timestamp(QDateTime::currentDateTime()) {}
};

/**
 * @brief 高性能数据库连接池
 * 
 * 特性：
 * - 连接池管理和复用
 * - 读写分离支持
 * - 异步操作支持
 * - 连接健康检查
 * - 自动重连机制
 */
class DatabasePool : public QObject
{
    Q_OBJECT

public:
    struct PoolConfig {
        QString hostName;
        int port;
        QString databaseName;
        QString userName;
        QString password;
        QString driverName;
        
        // 连接池配置
        int minConnections;
        int maxConnections;
        int connectionTimeout;
        int queryTimeout;
        
        // 读写分离配置
        bool enableReadWriteSplit;
        QString readOnlyHostName;
        int readOnlyPort;
        
        PoolConfig() 
            : port(3306)
            , driverName("QMYSQL")
            , minConnections(2)
            , maxConnections(10)
            , connectionTimeout(30000)
            , queryTimeout(30000)
            , enableReadWriteSplit(false)
            , readOnlyPort(3306)
        {}
    };

    struct PoolStats {
        QAtomicInt totalConnections{0};
        QAtomicInt activeConnections{0};
        QAtomicInt idleConnections{0};
        QAtomicInt totalQueries{0};
        QAtomicInt successfulQueries{0};
        QAtomicInt failedQueries{0};
        QAtomicInt readQueries{0};
        QAtomicInt writeQueries{0};
        QAtomicInt transactionQueries{0};
        QAtomicInt connectionErrors{0};
        QAtomicInt reconnections{0};
        QDateTime lastActivity;
    };

    explicit DatabasePool(QObject *parent = nullptr);
    ~DatabasePool();

    // 初始化和配置
    bool initialize(const PoolConfig& config);
    void shutdown();
    bool isInitialized() const;
    
    // 同步操作接口
    DatabaseResult<QSqlQuery> executeQuery(const QString& sql, const QVariantList& params = QVariantList(), 
                                          DatabaseOperationType type = DatabaseOperationType::Read);
    DatabaseResult<QSqlQuery> executeQuery(const QString& sql, const QVariantMap& params, 
                                          DatabaseOperationType type = DatabaseOperationType::Read);
    
    // 异步操作接口
    std::future<DatabaseResult<QSqlQuery>> executeQueryAsync(const QString& sql, const QVariantList& params = QVariantList(), 
                                                            DatabaseOperationType type = DatabaseOperationType::Read);
    
    // 事务支持
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    
    // 批量操作
    DatabaseResult<QList<QSqlQuery>> executeBatch(const QStringList& sqlList, 
                                                  const QList<QVariantList>& paramsList,
                                                  DatabaseOperationType type = DatabaseOperationType::Write);
    
    // 连接管理
    std::shared_ptr<DatabaseConnection> acquireConnection(DatabaseOperationType type = DatabaseOperationType::Read);
    void releaseConnection(std::shared_ptr<DatabaseConnection> connection);
    
    // 健康检查
    bool testConnection(std::shared_ptr<DatabaseConnection> connection);
    void checkConnectionHealth();
    
    // 统计信息
    PoolStats getStats() const;
    void resetStats();
    int getActiveConnectionCount() const;
    int getTotalConnectionCount() const;
    
    // 配置管理
    void setMaxConnections(int maxConnections);
    void setConnectionTimeout(int timeoutMs);
    void setQueryTimeout(int timeoutMs);
    void enableReadWriteSplit(bool enabled);

signals:
    void connectionCreated(const QString& connectionName);
    void connectionDestroyed(const QString& connectionName);
    void connectionError(const QString& error);
    void queryExecuted(const QString& sql, int executionTime);
    void poolOverloaded();
    void healthCheckFailed(const QString& connectionName);

private slots:
    void performMaintenance();
    void checkHealth();

private:
    // 连接池
    QQueue<std::shared_ptr<DatabaseConnection>> m_readConnections;
    QQueue<std::shared_ptr<DatabaseConnection>> m_writeConnections;
    QList<std::shared_ptr<DatabaseConnection>> m_allConnections;
    
    // 同步原语
    QMutex m_readPoolMutex;
    QMutex m_writePoolMutex;
    QMutex m_allConnectionsMutex;
    QWaitCondition m_connectionAvailable;
    
    // 配置
    PoolConfig m_config;
    QAtomicInt m_initialized{0};
    
    // 统计信息
    PoolStats m_stats;
    
    // 定时器
    QTimer* m_maintenanceTimer;
    QTimer* m_healthTimer;
    
    // 内部方法
    std::shared_ptr<DatabaseConnection> createConnection(bool readOnly = false);
    void destroyConnection(std::shared_ptr<DatabaseConnection> connection);
    QString generateConnectionName() const;
    
    // 连接池管理
    void ensureMinConnections();
    void cleanupIdleConnections();
    std::shared_ptr<DatabaseConnection> getAvailableConnection(DatabaseOperationType type);
    
    // 查询执行
    DatabaseResult<QSqlQuery> executeQueryInternal(std::shared_ptr<DatabaseConnection> connection,
                                                   const QString& sql, const QVariantList& params);
    DatabaseResult<QSqlQuery> executeQueryInternal(std::shared_ptr<DatabaseConnection> connection,
                                                   const QString& sql, const QVariantMap& params);
    
    // 错误处理
    void handleConnectionError(std::shared_ptr<DatabaseConnection> connection, const QString& error);
    bool attemptReconnection(std::shared_ptr<DatabaseConnection> connection);
    
    // 统计更新
    void updateQueryStats(DatabaseOperationType type, bool success, int executionTime);
    void updateConnectionStats();
    
    // 工具方法
    bool isReadOperation(const QString& sql) const;
    DatabaseOperationType detectOperationType(const QString& sql) const;
    void logDatabaseEvent(const QString& event, const QString& details = QString()) const;

    // 事务管理
    QHash<QThread*, std::shared_ptr<DatabaseConnection>> m_transactionConnections;
    QMutex m_transactionMutex;
};

#endif // DATABASEPOOL_H
