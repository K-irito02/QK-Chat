#include "ConnectionPool.h"
#include "../utils/LogManager.h"
#include <QUuid>
#include <QRandomGenerator>
#include <QMutexLocker>

Q_LOGGING_CATEGORY(connectionPool, "qkchat.client.connectionpool")

ConnectionPool::ConnectionPool(QObject *parent)
    : QObject(parent)
    , _initialized(false)
    , _serverPort(0)
    , _maxPoolSize(DEFAULT_MAX_POOL_SIZE)
    , _minPoolSize(DEFAULT_MIN_POOL_SIZE)
    , _maxIdleTime(DEFAULT_MAX_IDLE_TIME)
    , _healthCheckInterval(DEFAULT_HEALTH_CHECK_INTERVAL)
    , _loadBalanceStrategy(RoundRobin)
    , _maxRequestsPerConnection(DEFAULT_MAX_REQUESTS_PER_CONNECTION)
    , _roundRobinIndex(0)
    , _totalRequests(0)
    , _totalErrors(0)
{
    // 创建定时器
    _healthCheckTimer = new QTimer(this);
    connect(_healthCheckTimer, &QTimer::timeout, this, &ConnectionPool::onHealthCheckTimer);
    
    _idleCheckTimer = new QTimer(this);
    connect(_idleCheckTimer, &QTimer::timeout, this, &ConnectionPool::onIdleCheckTimer);
    _idleCheckTimer->start(IDLE_CHECK_INTERVAL);
    
    qCInfo(connectionPool) << "ConnectionPool created";
}

ConnectionPool::~ConnectionPool()
{
    shutdown();
}

bool ConnectionPool::initialize(const QString &host, int port, int poolSize)
{
    QMutexLocker locker(&_poolMutex);
    
    if (_initialized) {
        qCWarning(connectionPool) << "ConnectionPool already initialized";
        return false;
    }
    
    _serverHost = host;
    _serverPort = port;
    _maxPoolSize = qMax(poolSize, _minPoolSize);
    
    // 创建初始连接
    for (int i = 0; i < _minPoolSize; ++i) {
        NetworkClient* client = createConnection();
        if (!client) {
            qCWarning(connectionPool) << "Failed to create initial connection" << i;
            shutdown();
            return false;
        }
    }
    
    _initialized = true;
    _healthCheckTimer->start(_healthCheckInterval);
    
    qCInfo(connectionPool) << "ConnectionPool initialized with" << _connections.size() << "connections";
    LogManager::instance()->writeConnectionLog("POOL_INITIALIZED", 
        QString("Host: %1, Port: %2, Size: %3").arg(host).arg(port).arg(_connections.size()));
    
    emit poolSizeChanged(_connections.size());
    
    return true;
}

void ConnectionPool::shutdown()
{
    QMutexLocker locker(&_poolMutex);
    
    if (!_initialized) {
        return;
    }
    
    _healthCheckTimer->stop();
    _idleCheckTimer->stop();
    
    // 销毁所有连接
    QStringList connectionIds = _connections.keys();
    for (const QString &id : connectionIds) {
        destroyConnection(id);
    }
    
    _connections.clear();
    _availableConnections.clear();
    _initialized = false;
    
    qCInfo(connectionPool) << "ConnectionPool shutdown";
    LogManager::instance()->writeConnectionLog("POOL_SHUTDOWN", "All connections destroyed");
    
    emit poolSizeChanged(0);
}

bool ConnectionPool::isInitialized() const
{
    QMutexLocker locker(&_poolMutex);
    return _initialized;
}

NetworkClient* ConnectionPool::acquireConnection()
{
    QMutexLocker locker(&_poolMutex);
    
    if (!_initialized) {
        qCWarning(connectionPool) << "ConnectionPool not initialized";
        return nullptr;
    }
    
    NetworkClient* client = nullptr;
    
    // 根据负载均衡策略选择连接
    switch (_loadBalanceStrategy) {
    case RoundRobin:
        client = selectConnectionRoundRobin();
        break;
    case LeastConnections:
        client = selectConnectionLeastConnections();
        break;
    case Random:
        client = selectConnectionRandom();
        break;
    case HealthBased:
        client = selectConnectionHealthBased();
        break;
    }
    
    if (client) {
        // 更新连接状态
        for (auto it = _connections.begin(); it != _connections.end(); ++it) {
            if (it->client == client) {
                it->status = Busy;
                it->lastUsed = QDateTime::currentDateTime();
                it->activeRequests++;
                it->totalRequests++;
                
                _totalRequests++;
                
                qCDebug(connectionPool) << "Connection acquired:" << it->id;
                LogManager::instance()->writeConnectionLog("CONNECTION_ACQUIRED", 
                    QString("ID: %1, Active: %2").arg(it->id).arg(it->activeRequests));
                
                break;
            }
        }
    } else {
        // 尝试创建新连接
        if (_connections.size() < _maxPoolSize) {
            client = createConnection();
            if (client) {
                return acquireConnection(); // 递归获取新创建的连接
            }
        }
        
        qCWarning(connectionPool) << "No available connections in pool";
        LogManager::instance()->writeConnectionLog("NO_AVAILABLE_CONNECTION", 
            QString("Pool size: %1, Busy: %2").arg(_connections.size()).arg(getBusyConnections()));
    }
    
    return client;
}

void ConnectionPool::releaseConnection(NetworkClient* client)
{
    if (!client) {
        return;
    }
    
    QMutexLocker locker(&_poolMutex);
    
    // 查找并释放连接
    for (auto it = _connections.begin(); it != _connections.end(); ++it) {
        if (it->client == client) {
            it->status = Available;
            it->activeRequests = qMax(0, it->activeRequests - 1);
            
            // 如果连接健康且请求数未超限，加入可用队列
            if (it->isHealthy && it->totalRequests < _maxRequestsPerConnection) {
                if (!_availableConnections.contains(it->id)) {
                    _availableConnections.enqueue(it->id);
                }
            } else {
                // 连接不健康或请求数超限，销毁连接
                QString connectionId = it->id;
                locker.unlock();
                destroyConnection(connectionId);
                locker.relock();
            }
            
            qCDebug(connectionPool) << "Connection released:" << it->id;
            LogManager::instance()->writeConnectionLog("CONNECTION_RELEASED", 
                QString("ID: %1, Active: %2").arg(it->id).arg(it->activeRequests));
            
            break;
        }
    }
    
    // 维护连接池大小
    maintainPoolSize();
}

void ConnectionPool::releaseConnection(const QString &connectionId)
{
    QMutexLocker locker(&_poolMutex);
    
    if (_connections.contains(connectionId)) {
        NetworkClient* client = _connections[connectionId].client;
        locker.unlock();
        releaseConnection(client);
    }
}

void ConnectionPool::setMaxPoolSize(int maxSize)
{
    QMutexLocker locker(&_poolMutex);
    
    _maxPoolSize = qMax(maxSize, _minPoolSize);
    
    qCInfo(connectionPool) << "Max pool size set to:" << _maxPoolSize;
    
    // 如果当前连接数超过新的最大值，销毁多余连接
    while (_connections.size() > _maxPoolSize) {
        // 优先销毁空闲连接
        if (!_availableConnections.isEmpty()) {
            QString connectionId = _availableConnections.dequeue();
            destroyConnection(connectionId);
        } else {
            break;
        }
    }
}

void ConnectionPool::setMinPoolSize(int minSize)
{
    QMutexLocker locker(&_poolMutex);
    
    _minPoolSize = qMax(1, minSize);
    _maxPoolSize = qMax(_maxPoolSize, _minPoolSize);
    
    qCInfo(connectionPool) << "Min pool size set to:" << _minPoolSize;
    
    // 如果当前连接数少于新的最小值，创建新连接
    maintainPoolSize();
}

void ConnectionPool::setMaxIdleTime(int idleTimeMs)
{
    _maxIdleTime = idleTimeMs;
    qCInfo(connectionPool) << "Max idle time set to:" << _maxIdleTime << "ms";
}

void ConnectionPool::setHealthCheckInterval(int intervalMs)
{
    _healthCheckInterval = intervalMs;
    
    if (_healthCheckTimer->isActive()) {
        _healthCheckTimer->setInterval(_healthCheckInterval);
    }
    
    qCInfo(connectionPool) << "Health check interval set to:" << _healthCheckInterval << "ms";
}

void ConnectionPool::setLoadBalanceStrategy(LoadBalanceStrategy strategy)
{
    QMutexLocker locker(&_poolMutex);
    
    _loadBalanceStrategy = strategy;
    _roundRobinIndex = 0; // 重置轮询索引
    
    qCInfo(connectionPool) << "Load balance strategy set to:" << static_cast<int>(strategy);
    LogManager::instance()->writeConnectionLog("LOAD_BALANCE_STRATEGY_CHANGED", 
        QString("Strategy: %1").arg(static_cast<int>(strategy)));
    
    emit loadBalanceStrategyChanged(strategy);
}

void ConnectionPool::setMaxRequestsPerConnection(int maxRequests)
{
    _maxRequestsPerConnection = maxRequests;
    qCInfo(connectionPool) << "Max requests per connection set to:" << _maxRequestsPerConnection;
}

int ConnectionPool::getPoolSize() const
{
    QMutexLocker locker(&_poolMutex);
    return _connections.size();
}

int ConnectionPool::getAvailableConnections() const
{
    QMutexLocker locker(&_poolMutex);
    
    int count = 0;
    for (const ConnectionInfo &info : _connections) {
        if (info.status == Available && info.isHealthy) {
            count++;
        }
    }
    return count;
}

int ConnectionPool::getBusyConnections() const
{
    QMutexLocker locker(&_poolMutex);
    
    int count = 0;
    for (const ConnectionInfo &info : _connections) {
        if (info.status == Busy) {
            count++;
        }
    }
    return count;
}

int ConnectionPool::getHealthyConnections() const
{
    QMutexLocker locker(&_poolMutex);
    
    int count = 0;
    for (const ConnectionInfo &info : _connections) {
        if (info.isHealthy) {
            count++;
        }
    }
    return count;
}

QList<ConnectionPool::ConnectionInfo> ConnectionPool::getConnectionInfos() const
{
    QMutexLocker locker(&_poolMutex);
    return _connections.values();
}

qint64 ConnectionPool::getTotalRequests() const
{
    QMutexLocker locker(&_poolMutex);
    return _totalRequests;
}

qint64 ConnectionPool::getTotalErrors() const
{
    QMutexLocker locker(&_poolMutex);
    return _totalErrors;
}

double ConnectionPool::getErrorRate() const
{
    QMutexLocker locker(&_poolMutex);
    
    if (_totalRequests == 0) {
        return 0.0;
    }
    
    return static_cast<double>(_totalErrors) / _totalRequests * 100.0;
}

qint64 ConnectionPool::getAverageLatency() const
{
    QMutexLocker locker(&_poolMutex);
    
    if (_connections.isEmpty()) {
        return 0;
    }
    
    qint64 totalLatency = 0;
    int count = 0;
    
    for (const ConnectionInfo &info : _connections) {
        if (info.averageLatency > 0) {
            totalLatency += info.averageLatency;
            count++;
        }
    }
    
    return count > 0 ? totalLatency / count : 0;
}

void ConnectionPool::onConnectionConnected()
{
    NetworkClient* client = qobject_cast<NetworkClient*>(sender());
    if (!client) {
        return;
    }

    QMutexLocker locker(&_poolMutex);

    // 更新连接状态
    for (auto it = _connections.begin(); it != _connections.end(); ++it) {
        if (it->client == client) {
            updateConnectionStatus(it->id, Available);
            it->isHealthy = true;

            if (!_availableConnections.contains(it->id)) {
                _availableConnections.enqueue(it->id);
            }

            qCInfo(connectionPool) << "Connection connected:" << it->id;
            LogManager::instance()->writeConnectionLog("POOL_CONNECTION_CONNECTED", it->id);

            break;
        }
    }
}

void ConnectionPool::onConnectionDisconnected()
{
    NetworkClient* client = qobject_cast<NetworkClient*>(sender());
    if (!client) {
        return;
    }

    QMutexLocker locker(&_poolMutex);

    // 更新连接状态
    for (auto it = _connections.begin(); it != _connections.end(); ++it) {
        if (it->client == client) {
            updateConnectionStatus(it->id, Disconnected);
            it->isHealthy = false;

            // 从可用队列中移除
            _availableConnections.removeAll(it->id);

            qCWarning(connectionPool) << "Connection disconnected:" << it->id;
            LogManager::instance()->writeConnectionLog("POOL_CONNECTION_DISCONNECTED", it->id);

            break;
        }
    }

    // 维护连接池大小
    maintainPoolSize();
}

void ConnectionPool::onConnectionError(const QString &error)
{
    NetworkClient* client = qobject_cast<NetworkClient*>(sender());
    if (!client) {
        return;
    }

    QMutexLocker locker(&_poolMutex);

    // 更新连接状态和错误统计
    for (auto it = _connections.begin(); it != _connections.end(); ++it) {
        if (it->client == client) {
            updateConnectionStatus(it->id, Error);
            it->isHealthy = false;
            it->totalErrors++;
            _totalErrors++;

            // 从可用队列中移除
            _availableConnections.removeAll(it->id);

            qCWarning(connectionPool) << "Connection error:" << it->id << error;
            LogManager::instance()->writeConnectionLog("POOL_CONNECTION_ERROR",
                QString("ID: %1, Error: %2").arg(it->id, error));

            break;
        }
    }
}

void ConnectionPool::onHealthCheckTimer()
{
    performHealthCheck();
}

void ConnectionPool::onIdleCheckTimer()
{
    removeIdleConnections();
    maintainPoolSize();
}

QString ConnectionPool::generateConnectionId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

NetworkClient* ConnectionPool::createConnection()
{
    QString connectionId = generateConnectionId();

    NetworkClient* client = new NetworkClient(this);

    // 连接信号
    connect(client, &NetworkClient::connected, this, &ConnectionPool::onConnectionConnected);
    connect(client, &NetworkClient::disconnected, this, &ConnectionPool::onConnectionDisconnected);
    connect(client, &NetworkClient::connectionError, this, &ConnectionPool::onConnectionError);

    // 创建连接信息
    ConnectionInfo info;
    info.id = connectionId;
    info.client = client;
    info.status = Connecting;
    info.lastUsed = QDateTime::currentDateTime();
    info.created = QDateTime::currentDateTime();
    info.activeRequests = 0;
    info.totalRequests = 0;
    info.totalErrors = 0;
    info.averageLatency = 0;
    info.isHealthy = false;

    _connections[connectionId] = info;

    // 开始连接
    if (!client->connectToServer(_serverHost, _serverPort)) {
        qCWarning(connectionPool) << "Failed to start connection for:" << connectionId;
        _connections.remove(connectionId);
        client->deleteLater();
        return nullptr;
    }

    qCInfo(connectionPool) << "Connection created:" << connectionId;
    LogManager::instance()->writeConnectionLog("POOL_CONNECTION_CREATED",
        QString("ID: %1, Host: %2, Port: %3").arg(connectionId, _serverHost).arg(_serverPort));

    emit connectionCreated(connectionId);

    return client;
}

void ConnectionPool::destroyConnection(const QString &connectionId)
{
    QMutexLocker locker(&_poolMutex);

    if (!_connections.contains(connectionId)) {
        return;
    }

    ConnectionInfo info = _connections[connectionId];
    _connections.remove(connectionId);
    _availableConnections.removeAll(connectionId);

    // 断开连接并删除客户端
    if (info.client) {
        info.client->disconnect();
        info.client->deleteLater();
    }

    qCInfo(connectionPool) << "Connection destroyed:" << connectionId;
    LogManager::instance()->writeConnectionLog("POOL_CONNECTION_DESTROYED", connectionId);

    emit connectionDestroyed(connectionId);
    emit poolSizeChanged(_connections.size());
}

void ConnectionPool::updateConnectionStatus(const QString &connectionId, ConnectionStatus status)
{
    if (_connections.contains(connectionId)) {
        ConnectionStatus oldStatus = _connections[connectionId].status;
        _connections[connectionId].status = status;

        if (oldStatus != status) {
            emit connectionStatusChanged(connectionId, status);
        }
    }
}

NetworkClient* ConnectionPool::selectConnectionRoundRobin()
{
    if (_availableConnections.isEmpty()) {
        return nullptr;
    }

    // 轮询选择
    if (_roundRobinIndex >= _availableConnections.size()) {
        _roundRobinIndex = 0;
    }

    QString connectionId = _availableConnections.at(_roundRobinIndex);
    _roundRobinIndex++;

    if (_connections.contains(connectionId)) {
        _availableConnections.removeAll(connectionId);
        return _connections[connectionId].client;
    }

    return nullptr;
}

NetworkClient* ConnectionPool::selectConnectionLeastConnections()
{
    if (_availableConnections.isEmpty()) {
        return nullptr;
    }

    QString selectedId;
    int minConnections = INT_MAX;

    for (const QString &connectionId : _availableConnections) {
        if (_connections.contains(connectionId)) {
            const ConnectionInfo &info = _connections[connectionId];
            if (info.activeRequests < minConnections) {
                minConnections = info.activeRequests;
                selectedId = connectionId;
            }
        }
    }

    if (!selectedId.isEmpty()) {
        _availableConnections.removeAll(selectedId);
        return _connections[selectedId].client;
    }

    return nullptr;
}

NetworkClient* ConnectionPool::selectConnectionRandom()
{
    if (_availableConnections.isEmpty()) {
        return nullptr;
    }

    int randomIndex = QRandomGenerator::global()->bounded(_availableConnections.size());
    QString connectionId = _availableConnections.at(randomIndex);

    if (_connections.contains(connectionId)) {
        _availableConnections.removeAll(connectionId);
        return _connections[connectionId].client;
    }

    return nullptr;
}

NetworkClient* ConnectionPool::selectConnectionHealthBased()
{
    if (_availableConnections.isEmpty()) {
        return nullptr;
    }

    // 优先选择健康且延迟低的连接
    QString selectedId;
    qint64 bestLatency = LLONG_MAX;

    for (const QString &connectionId : _availableConnections) {
        if (_connections.contains(connectionId)) {
            const ConnectionInfo &info = _connections[connectionId];
            if (info.isHealthy && info.averageLatency < bestLatency) {
                bestLatency = info.averageLatency;
                selectedId = connectionId;
            }
        }
    }

    // 如果没有找到健康连接，使用轮询
    if (selectedId.isEmpty()) {
        return selectConnectionRoundRobin();
    }

    _availableConnections.removeAll(selectedId);
    return _connections[selectedId].client;
}

void ConnectionPool::performHealthCheck()
{
    QMutexLocker locker(&_poolMutex);

    int healthyCount = 0;
    int totalCount = _connections.size();

    for (auto it = _connections.begin(); it != _connections.end(); ++it) {
        checkConnectionHealth(it->id);
        if (it->isHealthy) {
            healthyCount++;
        }
    }

    qCDebug(connectionPool) << "Health check completed:" << healthyCount << "/" << totalCount << "healthy";
    LogManager::instance()->writeConnectionLog("HEALTH_CHECK_COMPLETED",
        QString("Healthy: %1/%2").arg(healthyCount).arg(totalCount));

    emit healthCheckCompleted(healthyCount, totalCount);
}

void ConnectionPool::checkConnectionHealth(const QString &connectionId)
{
    if (!_connections.contains(connectionId)) {
        return;
    }

    ConnectionInfo &info = _connections[connectionId];

    // 检查连接是否健康
    bool wasHealthy = info.isHealthy;
    info.isHealthy = isConnectionHealthy(info);

    if (wasHealthy != info.isHealthy) {
        qCDebug(connectionPool) << "Connection health changed:" << connectionId << info.isHealthy;
        LogManager::instance()->writeConnectionLog("CONNECTION_HEALTH_CHANGED",
            QString("ID: %1, Healthy: %2").arg(connectionId).arg(info.isHealthy));
    }
}

bool ConnectionPool::isConnectionHealthy(const ConnectionInfo &info) const
{
    // 检查连接是否处于良好状态
    if (!info.client || !info.client->isConnected()) {
        return false;
    }

    // 检查错误率
    if (info.totalRequests > 0) {
        double errorRate = static_cast<double>(info.totalErrors) / info.totalRequests;
        if (errorRate > 0.1) { // 错误率超过10%
            return false;
        }
    }

    // 检查延迟
    if (info.averageLatency > 5000) { // 延迟超过5秒
        return false;
    }

    return true;
}

void ConnectionPool::maintainPoolSize()
{
    // 确保连接池大小在合理范围内
    while (_connections.size() < _minPoolSize) {
        NetworkClient* client = createConnection();
        if (!client) {
            break;
        }
    }

    // 移除超过最大数量的连接
    while (_connections.size() > _maxPoolSize) {
        if (!_availableConnections.isEmpty()) {
            QString connectionId = _availableConnections.dequeue();
            destroyConnection(connectionId);
        } else {
            break;
        }
    }
}

void ConnectionPool::removeIdleConnections()
{
    QMutexLocker locker(&_poolMutex);

    QDateTime now = QDateTime::currentDateTime();
    QStringList toRemove;

    for (auto it = _connections.begin(); it != _connections.end(); ++it) {
        if (it->status == Available &&
            it->lastUsed.msecsTo(now) > _maxIdleTime &&
            _connections.size() > _minPoolSize) {
            toRemove.append(it->id);
        }
    }

    for (const QString &connectionId : toRemove) {
        qCDebug(connectionPool) << "Removing idle connection:" << connectionId;
        LogManager::instance()->writeConnectionLog("IDLE_CONNECTION_REMOVED", connectionId);
        destroyConnection(connectionId);
    }
}

void ConnectionPool::balanceConnections()
{
    // 这里可以实现更复杂的负载均衡逻辑
    // 例如根据服务器负载动态调整连接数
}
