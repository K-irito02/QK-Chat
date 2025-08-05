#include "DatabasePool.h"
#include <QSqlDriver>
#include <QThread>
#include <QUuid>
#include <QDebug>
#include <QCoreApplication>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(databasePool, "qkchat.server.databasepool")

DatabasePool* DatabasePool::instance()
{
    static DatabasePool* instance = nullptr;
    static QMutex mutex;
    
    if (!instance) {
        QMutexLocker locker(&mutex);
        if (!instance) {
            instance = new DatabasePool();
        }
    }
    
    return instance;
}

DatabasePool::DatabasePool(QObject *parent)
    : QObject(parent)
    , m_maintenanceTimer(new QTimer(this))
    , m_healthTimer(new QTimer(this))
{
    // 设置维护定时器 - 每5分钟执行一次
    m_maintenanceTimer->setInterval(300000);
    connect(m_maintenanceTimer, &QTimer::timeout, this, &DatabasePool::performMaintenance);
    
    // 设置健康检查定时器 - 每30秒检查一次
    m_healthTimer->setInterval(30000);
    connect(m_healthTimer, &QTimer::timeout, this, &DatabasePool::checkHealth);
    
    qCInfo(databasePool) << "DatabasePool created";
}

DatabasePool::~DatabasePool()
{
    shutdown();
    qCInfo(databasePool) << "DatabasePool destroyed";
}

bool DatabasePool::initialize(const PoolConfig& config)
{
    qCInfo(databasePool) << "Initializing DatabasePool...";
    
    if (m_initialized.loadAcquire()) {
        qCWarning(databasePool) << "DatabasePool already initialized";
        return true;
    }
    
    m_config = config;
    
    try {
        // 创建最小连接数
        ensureMinConnections();
        
        // 启动定时器
        m_maintenanceTimer->start();
        m_healthTimer->start();
        
        m_initialized.storeRelease(1);
        m_stats.lastActivity = QDateTime::currentDateTime();
        
        qCInfo(databasePool) << "DatabasePool initialized successfully with"
                            << m_config.minConnections << "connections";
        return true;
        
    } catch (const std::exception& e) {
        qCCritical(databasePool) << "Failed to initialize DatabasePool:" << e.what();
        return false;
    }
}

void DatabasePool::shutdown()
{
    if (!m_initialized.testAndSetOrdered(1, 0)) {
        return;
    }
    
    qCInfo(databasePool) << "Shutting down DatabasePool...";
    
    m_maintenanceTimer->stop();
    m_healthTimer->stop();
    
    // 等待所有连接释放
    QMutexLocker locker(&m_allConnectionsMutex);
    
    for (auto& connection : m_allConnections) {
        if (connection) {
            destroyConnection(connection);
        }
    }
    
    m_allConnections.clear();
    m_readConnections.clear();
    m_writeConnections.clear();
    
    qCInfo(databasePool) << "DatabasePool shutdown complete";
}

bool DatabasePool::isInitialized() const
{
    return m_initialized.loadAcquire() > 0;
}

DatabaseResult<QSqlQuery> DatabasePool::executeQuery(const QString& sql, const QVariantList& params, 
                                                     DatabaseOperationType type)
{
    if (!isInitialized()) {
        return DatabaseResult<QSqlQuery>("DatabasePool not initialized");
    }
    
    QDateTime startTime = QDateTime::currentDateTime();
    
    auto connection = acquireConnection(type);
    if (!connection) {
        return DatabaseResult<QSqlQuery>("Failed to acquire database connection");
    }
    
    auto result = executeQueryInternal(connection, sql, params);
    releaseConnection(connection);
    
    int executionTime = startTime.msecsTo(QDateTime::currentDateTime());
    updateQueryStats(type, result.success, executionTime);
    emit queryExecuted(sql, executionTime);
    
    return result;
}

DatabaseResult<QSqlQuery> DatabasePool::executeQuery(const QString& sql, const QVariantMap& params, 
                                                     DatabaseOperationType type)
{
    if (!isInitialized()) {
        return DatabaseResult<QSqlQuery>("DatabasePool not initialized");
    }
    
    QDateTime startTime = QDateTime::currentDateTime();
    
    auto connection = acquireConnection(type);
    if (!connection) {
        return DatabaseResult<QSqlQuery>("Failed to acquire database connection");
    }
    
    auto result = executeQueryInternal(connection, sql, params);
    releaseConnection(connection);
    
    int executionTime = startTime.msecsTo(QDateTime::currentDateTime());
    updateQueryStats(type, result.success, executionTime);
    emit queryExecuted(sql, executionTime);
    
    return result;
}

std::future<DatabaseResult<QSqlQuery>> DatabasePool::executeQueryAsync(const QString& sql, 
                                                                       const QVariantList& params, 
                                                                       DatabaseOperationType type)
{
    return std::async(std::launch::async, [this, sql, params, type]() {
        return executeQuery(sql, params, type);
    });
}

bool DatabasePool::beginTransaction()
{
    QThread* currentThread = QThread::currentThread();
    QMutexLocker locker(&m_transactionMutex);
    
    if (m_transactionConnections.contains(currentThread)) {
        qCWarning(databasePool) << "Transaction already active for current thread";
        return false;
    }
    
    auto connection = acquireConnection(DatabaseOperationType::Transaction);
    if (!connection || !connection->database.transaction()) {
        if (connection) {
            releaseConnection(connection);
        }
        return false;
    }
    
    m_transactionConnections[currentThread] = connection;
    qCDebug(databasePool) << "Transaction started for thread:" << currentThread;
    return true;
}

bool DatabasePool::commitTransaction()
{
    QThread* currentThread = QThread::currentThread();
    QMutexLocker locker(&m_transactionMutex);
    
    auto it = m_transactionConnections.find(currentThread);
    if (it == m_transactionConnections.end()) {
        qCWarning(databasePool) << "No active transaction for current thread";
        return false;
    }
    
    auto connection = it.value();
    bool success = connection->database.commit();
    
    releaseConnection(connection);
    m_transactionConnections.erase(it);
    
    qCDebug(databasePool) << "Transaction" << (success ? "committed" : "commit failed") 
                         << "for thread:" << currentThread;
    return success;
}

bool DatabasePool::rollbackTransaction()
{
    QThread* currentThread = QThread::currentThread();
    QMutexLocker locker(&m_transactionMutex);
    
    auto it = m_transactionConnections.find(currentThread);
    if (it == m_transactionConnections.end()) {
        qCWarning(databasePool) << "No active transaction for current thread";
        return false;
    }
    
    auto connection = it.value();
    bool success = connection->database.rollback();
    
    releaseConnection(connection);
    m_transactionConnections.erase(it);
    
    qCDebug(databasePool) << "Transaction" << (success ? "rolled back" : "rollback failed") 
                         << "for thread:" << currentThread;
    return success;
}

DatabaseResult<QList<QSqlQuery>> DatabasePool::executeBatch(const QStringList& sqlList, 
                                                           const QList<QVariantList>& paramsList,
                                                           DatabaseOperationType type)
{
    if (sqlList.size() != paramsList.size()) {
        return DatabaseResult<QList<QSqlQuery>>("SQL list and params list size mismatch");
    }
    
    auto connection = acquireConnection(type);
    if (!connection) {
        return DatabaseResult<QList<QSqlQuery>>("Failed to acquire database connection");
    }
    
    QList<QSqlQuery> results;
    bool allSuccess = true;
    QString lastError;
    
    // 开始事务
    if (!connection->database.transaction()) {
        releaseConnection(connection);
        return DatabaseResult<QList<QSqlQuery>>("Failed to start transaction");
    }
    
    for (int i = 0; i < sqlList.size(); ++i) {
        auto result = executeQueryInternal(connection, sqlList[i], paramsList[i]);
        results.append(result.data);
        
        if (!result.success) {
            allSuccess = false;
            lastError = result.error;
            break;
        }
    }
    
    // 提交或回滚事务
    if (allSuccess) {
        if (!connection->database.commit()) {
            allSuccess = false;
            lastError = "Failed to commit transaction";
        }
    } else {
        connection->database.rollback();
    }
    
    releaseConnection(connection);
    
    if (allSuccess) {
        return DatabaseResult<QList<QSqlQuery>>(results);
    } else {
        return DatabaseResult<QList<QSqlQuery>>(lastError);
    }
}

std::shared_ptr<DatabaseConnection> DatabasePool::acquireConnection(DatabaseOperationType type)
{
    if (!isInitialized()) {
        return nullptr;
    }
    
    // 检查是否有事务连接
    QThread* currentThread = QThread::currentThread();
    {
        QMutexLocker locker(&m_transactionMutex);
        auto it = m_transactionConnections.find(currentThread);
        if (it != m_transactionConnections.end()) {
            return it.value(); // 返回事务连接
        }
    }
    
    return getAvailableConnection(type);
}

void DatabasePool::releaseConnection(std::shared_ptr<DatabaseConnection> connection)
{
    if (!connection) {
        return;
    }
    
    // 检查是否是事务连接
    QThread* currentThread = QThread::currentThread();
    {
        QMutexLocker locker(&m_transactionMutex);
        if (m_transactionConnections.contains(currentThread)) {
            return; // 事务连接不释放
        }
    }
    
    connection->markFree();
    
    // 根据连接类型放回相应的队列
    if (m_config.enableReadWriteSplit) {
        if (connection->database.hostName() == m_config.readOnlyHostName) {
            QMutexLocker locker(&m_readPoolMutex);
            m_readConnections.enqueue(connection);
        } else {
            QMutexLocker locker(&m_writePoolMutex);
            m_writeConnections.enqueue(connection);
        }
    } else {
        QMutexLocker locker(&m_writePoolMutex);
        m_writeConnections.enqueue(connection);
    }
    
    m_connectionAvailable.wakeOne();
    qCDebug(databasePool) << "Connection released:" << connection->connectionName;
}

bool DatabasePool::testConnection(std::shared_ptr<DatabaseConnection> connection)
{
    if (!connection || !connection->isValid) {
        return false;
    }
    
    QSqlQuery query(connection->database);
    return query.exec("SELECT 1");
}

void DatabasePool::checkConnectionHealth()
{
    QMutexLocker locker(&m_allConnectionsMutex);
    
    for (auto& connection : m_allConnections) {
        if (connection && !connection->isInUse()) {
            if (!testConnection(connection)) {
                qCWarning(databasePool) << "Health check failed for connection:" 
                                       << connection->connectionName;
                emit healthCheckFailed(connection->connectionName);
                
                // 尝试重连
                if (!attemptReconnection(connection)) {
                    // 重连失败，标记为无效
                    connection->isValid = false;
                }
            }
        }
    }
}

DatabasePool::PoolStats DatabasePool::getStats() const
{
    return m_stats;
}

void DatabasePool::resetStats()
{
    m_stats.totalQueries.storeRelease(0);
    m_stats.successfulQueries.storeRelease(0);
    m_stats.failedQueries.storeRelease(0);
    m_stats.readQueries.storeRelease(0);
    m_stats.writeQueries.storeRelease(0);
    m_stats.transactionQueries.storeRelease(0);
    m_stats.connectionErrors.storeRelease(0);
    m_stats.reconnections.storeRelease(0);
    
    qCInfo(databasePool) << "Database pool stats reset";
}

int DatabasePool::getActiveConnectionCount() const
{
    return m_stats.activeConnections.loadAcquire();
}

int DatabasePool::getTotalConnectionCount() const
{
    return m_stats.totalConnections.loadAcquire();
}

void DatabasePool::setMaxConnections(int maxConnections)
{
    m_config.maxConnections = maxConnections;
    qCInfo(databasePool) << "Max connections set to" << maxConnections;
}

void DatabasePool::setConnectionTimeout(int timeoutMs)
{
    m_config.connectionTimeout = timeoutMs;
    qCInfo(databasePool) << "Connection timeout set to" << timeoutMs << "ms";
}

void DatabasePool::setQueryTimeout(int timeoutMs)
{
    m_config.queryTimeout = timeoutMs;
    qCInfo(databasePool) << "Query timeout set to" << timeoutMs << "ms";
}

void DatabasePool::enableReadWriteSplit(bool enabled)
{
    m_config.enableReadWriteSplit = enabled;
    qCInfo(databasePool) << "Read-write split" << (enabled ? "enabled" : "disabled");
}

void DatabasePool::performMaintenance()
{
    qCDebug(databasePool) << "Performing database pool maintenance...";
    
    cleanupIdleConnections();
    ensureMinConnections();
    updateConnectionStats();
    
    qCDebug(databasePool) << "Maintenance completed. Active connections:" 
                         << getActiveConnectionCount();
}

void DatabasePool::checkHealth()
{
    checkConnectionHealth();
}

std::shared_ptr<DatabaseConnection> DatabasePool::createConnection(bool readOnly)
{
    QString connectionName = generateConnectionName();
    
    auto connection = std::make_shared<DatabaseConnection>();
    connection->connectionName = connectionName;
    connection->database = QSqlDatabase::addDatabase(m_config.driverName, connectionName);
    
    if (readOnly && m_config.enableReadWriteSplit) {
        connection->database.setHostName(m_config.readOnlyHostName);
        connection->database.setPort(m_config.readOnlyPort);
    } else {
        connection->database.setHostName(m_config.hostName);
        connection->database.setPort(m_config.port);
    }
    
    connection->database.setDatabaseName(m_config.databaseName);
    connection->database.setUserName(m_config.userName);
    connection->database.setPassword(m_config.password);
    
    // 设置连接选项
    connection->database.setConnectOptions("MYSQL_OPT_RECONNECT=1");
    
    if (!connection->database.open()) {
        QString error = connection->database.lastError().text();
        qCCritical(databasePool) << "Failed to create connection:" << error;
        handleConnectionError(connection, error);
        return nullptr;
    }
    
    connection->isValid = true;
    connection->lastUsed = QDateTime::currentDateTime();
    
    {
        QMutexLocker locker(&m_allConnectionsMutex);
        m_allConnections.append(connection);
    }
    
    m_stats.totalConnections.fetchAndAddOrdered(1);
    emit connectionCreated(connectionName);
    
    qCInfo(databasePool) << "Database connection created:" << connectionName;
    return connection;
}

void DatabasePool::destroyConnection(std::shared_ptr<DatabaseConnection> connection)
{
    if (!connection) {
        return;
    }
    
    QString connectionName = connection->connectionName;
    
    if (connection->database.isOpen()) {
        connection->database.close();
    }
    
    QSqlDatabase::removeDatabase(connectionName);
    connection->isValid = false;
    
    m_stats.totalConnections.fetchAndSubOrdered(1);
    emit connectionDestroyed(connectionName);
    
    qCDebug(databasePool) << "Database connection destroyed:" << connectionName;
}

QString DatabasePool::generateConnectionName() const
{
    return QString("db_conn_%1_%2")
           .arg(reinterpret_cast<qulonglong>(QThread::currentThreadId()))
           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

void DatabasePool::ensureMinConnections()
{
    int currentConnections = getTotalConnectionCount();
    int minConnections = m_config.minConnections;
    
    if (currentConnections < minConnections) {
        int connectionsToCreate = minConnections - currentConnections;
        
        for (int i = 0; i < connectionsToCreate; ++i) {
            auto connection = createConnection(false); // 创建写连接
            if (connection) {
                QMutexLocker locker(&m_writePoolMutex);
                m_writeConnections.enqueue(connection);
            }
            
            // 如果启用读写分离，也创建读连接
            if (m_config.enableReadWriteSplit) {
                auto readConnection = createConnection(true);
                if (readConnection) {
                    QMutexLocker locker(&m_readPoolMutex);
                    m_readConnections.enqueue(readConnection);
                }
            }
        }
    }
}

void DatabasePool::cleanupIdleConnections()
{
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-300); // 5分钟
    QList<std::shared_ptr<DatabaseConnection>> connectionsToRemove;
    
    {
        QMutexLocker locker(&m_allConnectionsMutex);
        
        for (auto it = m_allConnections.begin(); it != m_allConnections.end();) {
            auto connection = *it;
            if (connection && !connection->isInUse() && 
                connection->lastUsed < cutoffTime &&
                getTotalConnectionCount() > m_config.minConnections) {
                
                connectionsToRemove.append(connection);
                it = m_allConnections.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    // 销毁空闲连接
    for (auto& connection : connectionsToRemove) {
        destroyConnection(connection);
    }
    
    if (!connectionsToRemove.isEmpty()) {
        qCDebug(databasePool) << "Cleaned up" << connectionsToRemove.size() << "idle connections";
    }
}

std::shared_ptr<DatabaseConnection> DatabasePool::getAvailableConnection(DatabaseOperationType type)
{
    if (!isInitialized()) {
        qCWarning(databasePool) << "DatabasePool not initialized";
        return nullptr;
    }

    QMutex* poolMutex;
    QQueue<std::shared_ptr<DatabaseConnection>>* pool;

    // 选择合适的连接池
    if (type == DatabaseOperationType::Read && m_config.enableReadWriteSplit) {
        poolMutex = &m_readPoolMutex;
        pool = &m_readConnections;
    } else {
        poolMutex = &m_writePoolMutex;
        pool = &m_writeConnections;
    }

    QMutexLocker locker(poolMutex);

    // 使用条件变量避免忙等待和惊群效应
    QElapsedTimer timer;
    timer.start();

    // 等待条件：有可用连接 OR 可以创建新连接 OR 系统正在关闭
    while (pool->isEmpty() &&
           getTotalConnectionCount() >= m_config.maxConnections &&
           isInitialized()) {

        int remainingTime = m_config.connectionTimeout - timer.elapsed();
        if (remainingTime <= 0) {
            qCWarning(databasePool) << "Connection acquisition timeout for type:" << static_cast<int>(type);
            return nullptr;
        }

        if (!m_connectionAvailable.wait(poolMutex, remainingTime)) {
            qCWarning(databasePool) << "Connection acquisition timeout for type:" << static_cast<int>(type);
            return nullptr;
        }
    }

    // 检查是否正在关闭
    if (!isInitialized()) {
        qCWarning(databasePool) << "DatabasePool is shutting down";
        return nullptr;
    }

    // 如果有可用连接，直接返回
    if (!pool->isEmpty()) {
        auto connection = pool->dequeue();
        if (connection && connection->isValid) {
            connection->markUsed();
            m_stats.activeConnections.fetchAndAddOrdered(1);
            return connection;
        }
    }

    // 需要创建新连接，先释放锁避免阻塞其他线程
    locker.unlock();

    auto newConnection = createConnection(type == DatabaseOperationType::Read);
    if (newConnection && newConnection->isValid) {
        newConnection->markUsed();
        m_stats.activeConnections.fetchAndAddOrdered(1);
        return newConnection;
    }

    qCWarning(databasePool) << "Failed to create new connection for type:" << static_cast<int>(type);
    return nullptr;
}

DatabaseResult<QSqlQuery> DatabasePool::executeQueryInternal(std::shared_ptr<DatabaseConnection> connection,
                                                            const QString& sql, const QVariantList& params)
{
    if (!connection || !connection->isValid) {
        return DatabaseResult<QSqlQuery>("Invalid database connection");
    }
    
    QSqlQuery query(connection->database);
    query.prepare(sql);
    
    // 绑定参数
    for (int i = 0; i < params.size(); ++i) {
        query.bindValue(i, params[i]);
    }
    
    if (!query.exec()) {
        QString error = query.lastError().text();
        qCWarning(databasePool) << "Query execution failed:" << error << "SQL:" << sql;
        return DatabaseResult<QSqlQuery>(error);
    }
    
    return DatabaseResult<QSqlQuery>(query);
}

DatabaseResult<QSqlQuery> DatabasePool::executeQueryInternal(std::shared_ptr<DatabaseConnection> connection,
                                                            const QString& sql, const QVariantMap& params)
{
    if (!connection || !connection->isValid) {
        return DatabaseResult<QSqlQuery>("Invalid database connection");
    }
    
    QSqlQuery query(connection->database);
    query.prepare(sql);
    
    // 绑定命名参数
    for (auto it = params.begin(); it != params.end(); ++it) {
        query.bindValue(it.key(), it.value());
    }
    
    if (!query.exec()) {
        QString error = query.lastError().text();
        qCWarning(databasePool) << "Query execution failed:" << error << "SQL:" << sql;
        return DatabaseResult<QSqlQuery>(error);
    }
    
    return DatabaseResult<QSqlQuery>(query);
}

void DatabasePool::handleConnectionError(std::shared_ptr<DatabaseConnection> connection, const QString& error)
{
    m_stats.connectionErrors.fetchAndAddOrdered(1);
    emit connectionError(error);
    
    if (connection) {
        connection->isValid = false;
        qCWarning(databasePool) << "Connection error for" << connection->connectionName << ":" << error;
    }
}

bool DatabasePool::attemptReconnection(std::shared_ptr<DatabaseConnection> connection)
{
    if (!connection) {
        return false;
    }
    
    qCInfo(databasePool) << "Attempting to reconnect:" << connection->connectionName;
    
    connection->database.close();
    
    if (connection->database.open()) {
        connection->isValid = true;
        connection->lastUsed = QDateTime::currentDateTime();
        m_stats.reconnections.fetchAndAddOrdered(1);
        qCInfo(databasePool) << "Reconnection successful:" << connection->connectionName;
        return true;
    } else {
        QString error = connection->database.lastError().text();
        qCWarning(databasePool) << "Reconnection failed:" << connection->connectionName << error;
        return false;
    }
}

void DatabasePool::updateQueryStats(DatabaseOperationType type, bool success, int executionTime)
{
    Q_UNUSED(executionTime)
    
    m_stats.totalQueries.fetchAndAddOrdered(1);
    
    if (success) {
        m_stats.successfulQueries.fetchAndAddOrdered(1);
    } else {
        m_stats.failedQueries.fetchAndAddOrdered(1);
    }
    
    switch (type) {
    case DatabaseOperationType::Read:
        m_stats.readQueries.fetchAndAddOrdered(1);
        break;
    case DatabaseOperationType::Write:
        m_stats.writeQueries.fetchAndAddOrdered(1);
        break;
    case DatabaseOperationType::Transaction:
        m_stats.transactionQueries.fetchAndAddOrdered(1);
        break;
    }
    
    m_stats.lastActivity = QDateTime::currentDateTime();
}

void DatabasePool::updateConnectionStats()
{
    int active = 0;
    int idle = 0;
    
    {
        QMutexLocker locker(&m_allConnectionsMutex);
        for (const auto& connection : m_allConnections) {
            if (connection && connection->isValid) {
                if (connection->isInUse()) {
                    active++;
                } else {
                    idle++;
                }
            }
        }
    }
    
    m_stats.activeConnections.storeRelease(active);
    m_stats.idleConnections.storeRelease(idle);
}

bool DatabasePool::isReadOperation(const QString& sql) const
{
    QString trimmedSql = sql.trimmed().toUpper();
    return trimmedSql.startsWith("SELECT") || 
           trimmedSql.startsWith("SHOW") || 
           trimmedSql.startsWith("DESCRIBE") ||
           trimmedSql.startsWith("EXPLAIN");
}

DatabaseOperationType DatabasePool::detectOperationType(const QString& sql) const
{
    if (isReadOperation(sql)) {
        return DatabaseOperationType::Read;
    } else {
        return DatabaseOperationType::Write;
    }
}

void DatabasePool::logDatabaseEvent(const QString& event, const QString& details) const
{
    if (details.isEmpty()) {
        qCDebug(databasePool) << event;
    } else {
        qCDebug(databasePool) << event << ":" << details;
    }
}
