#include "ConnectionManager.h"
#include <QDebug>
#include <QHostAddress>

Q_LOGGING_CATEGORY(connectionManager, "qkchat.server.connectionmanager")

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject(parent)
    , m_maintenanceTimer(new QTimer(this))
    , m_heartbeatTimer(new QTimer(this))
{
    // 设置维护定时器 - 每5分钟执行一次清理
    m_maintenanceTimer->setInterval(300000);
    connect(m_maintenanceTimer, &QTimer::timeout, this, &ConnectionManager::performMaintenance);
    m_maintenanceTimer->start();
    
    // 设置心跳检查定时器 - 每30秒检查一次
    m_heartbeatTimer->setInterval(30000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ConnectionManager::checkHeartbeats);
    m_heartbeatTimer->start();
    
    qCInfo(connectionManager) << "ConnectionManager initialized";
}

ConnectionManager::~ConnectionManager()
{
    m_maintenanceTimer->stop();
    m_heartbeatTimer->stop();
    
    // 清理所有连接
    auto connections = getAllConnections();
    for (auto& conn : connections) {
        if (conn && conn->getSocket()) {
            conn->getSocket()->disconnect();
            conn->getSocket()->close();
        }
    }
    
    qCInfo(connectionManager) << "ConnectionManager destroyed";
}

bool ConnectionManager::addConnection(QSslSocket* socket)
{
    if (!socket) {
        qCWarning(connectionManager) << "Cannot add null socket";
        return false;
    }
    
    // 检查是否达到最大连接数
    if (getConnectionCount() >= m_maxConnections.loadAcquire()) {
        qCWarning(connectionManager) << "Maximum connections reached:" << m_maxConnections.loadAcquire();
        emit maxConnectionsReached();
        return false;
    }
    
    // 检查socket是否已存在
    if (m_socketConnections.contains(socket)) {
        qCWarning(connectionManager) << "Socket already exists in connection manager";
        return false;
    }
    
    auto connection = std::make_shared<ClientConnection>(socket);
    connection->state.transitionIf(ConnectionState::Connecting, ConnectionState::Connected);
    
    m_socketConnections.insert(socket, connection);
    
    incrementCounter("total_connections");
    incrementCounter("active_connections");
    
    logConnectionEvent("CONNECTION_ADDED", socket);
    emit connectionAdded(socket);
    
    qCInfo(connectionManager) << "Connection added for socket:" << socket 
                             << "Total connections:" << getConnectionCount();
    return true;
}

bool ConnectionManager::removeConnection(QSslSocket* socket)
{
    if (!socket) {
        return false;
    }
    
    auto connection = m_socketConnections.value(socket);
    if (!connection) {
        return false;
    }
    
    removeConnectionInternal(connection);
    return true;
}

bool ConnectionManager::removeConnection(qint64 userId)
{
    if (userId <= 0) {
        return false;
    }
    
    auto connection = m_userConnections.value(userId);
    if (!connection) {
        return false;
    }
    
    removeConnectionInternal(connection);
    return true;
}

std::shared_ptr<ClientConnection> ConnectionManager::getConnection(QSslSocket* socket) const
{
    return m_socketConnections.value(socket);
}

std::shared_ptr<ClientConnection> ConnectionManager::getConnectionByUserId(qint64 userId) const
{
    return m_userConnections.value(userId);
}

QList<std::shared_ptr<ClientConnection>> ConnectionManager::getAllConnections() const
{
    return m_socketConnections.values();
}

QList<std::shared_ptr<ClientConnection>> ConnectionManager::getAuthenticatedConnections() const
{
    QList<std::shared_ptr<ClientConnection>> result;
    result.reserve(m_socketConnections.size()); // 预分配内存

    // 使用快照避免遍历期间的修改问题
    auto snapshot = m_socketConnections.snapshot();
    for (auto it = snapshot.begin(); it != snapshot.end(); ++it) {
        auto conn = it.value();
        if (conn && conn->isAuthenticated()) {
            result.append(conn);
        }
    }

    return result;
}

bool ConnectionManager::authenticateConnection(QSslSocket* socket, qint64 userId, const QString& sessionToken)
{
    auto connection = m_socketConnections.value(socket);
    if (!connection) {
        qCWarning(connectionManager) << "Cannot authenticate non-existent connection";
        return false;
    }
    
    if (!connection->state.transitionIf(ConnectionState::Connected, ConnectionState::Authenticated)) {
        qCWarning(connectionManager) << "Cannot authenticate connection in state:" 
                                    << static_cast<int>(connection->state.currentState());
        return false;
    }
    
    // 如果用户已经有其他连接，先移除旧连接
    auto existingConnection = m_userConnections.value(userId);
    if (existingConnection && existingConnection != connection) {
        qCInfo(connectionManager) << "Removing existing connection for user:" << userId;
        removeConnectionInternal(existingConnection);
    }
    
    connection->setUserId(userId);
    connection->setSessionToken(sessionToken);
    connection->updateActivity();
    
    m_userConnections.insert(userId, connection);
    
    incrementCounter("authenticated_connections");
    logConnectionEvent("CONNECTION_AUTHENTICATED", socket, userId);
    emit connectionAuthenticated(userId, sessionToken);
    
    qCInfo(connectionManager) << "Connection authenticated for user:" << userId;
    return true;
}

bool ConnectionManager::updateConnectionActivity(QSslSocket* socket)
{
    auto connection = m_socketConnections.value(socket);
    if (!connection) {
        return false;
    }
    
    connection->updateActivity();
    return true;
}

bool ConnectionManager::setConnectionState(QSslSocket* socket, ConnectionState state)
{
    auto connection = m_socketConnections.value(socket);
    if (!connection) {
        return false;
    }
    
    ConnectionState oldState = connection->state.currentState();
    ConnectionState newState = connection->state.exchange(state);
    
    if (oldState != newState) {
        emit connectionStateChanged(socket, oldState, newState);
        logConnectionEvent("STATE_CHANGED", socket, connection->getUserId());
    }
    
    return true;
}

void ConnectionManager::broadcastToAll(const QByteArray& data)
{
    int sentCount = 0;
    
    m_socketConnections.forEach([&data, &sentCount](QSslSocket* socket, std::shared_ptr<ClientConnection> conn) {
        if (conn && conn->isConnected() && socket && socket->state() == QAbstractSocket::ConnectedState) {
            socket->write(data);
            conn->incrementMessagesSent();
            conn->addBytesTransferred(data.size());
            sentCount++;
        }
    });
    
    addToCounter("messages_broadcast", sentCount);
    qCDebug(connectionManager) << "Broadcast message to" << sentCount << "connections";
}

void ConnectionManager::broadcastToAuthenticated(const QByteArray& data)
{
    int sentCount = 0;
    
    m_userConnections.forEach([&data, &sentCount](qint64 userId, std::shared_ptr<ClientConnection> conn) {
        Q_UNUSED(userId)
        if (conn && conn->isAuthenticated() && conn->getSocket() && 
            conn->getSocket()->state() == QAbstractSocket::ConnectedState) {
            conn->getSocket()->write(data);
            conn->incrementMessagesSent();
            conn->addBytesTransferred(data.size());
            sentCount++;
        }
    });
    
    addToCounter("messages_broadcast_auth", sentCount);
    qCDebug(connectionManager) << "Broadcast message to" << sentCount << "authenticated connections";
}

void ConnectionManager::broadcastToUsers(const QList<qint64>& userIds, const QByteArray& data)
{
    int sentCount = 0;
    
    for (qint64 userId : userIds) {
        auto connection = m_userConnections.value(userId);
        if (connection && connection->isAuthenticated() && connection->getSocket() &&
            connection->getSocket()->state() == QAbstractSocket::ConnectedState) {
            connection->getSocket()->write(data);
            connection->incrementMessagesSent();
            connection->addBytesTransferred(data.size());
            sentCount++;
        }
    }
    
    addToCounter("messages_targeted", sentCount);
    qCDebug(connectionManager) << "Sent targeted message to" << sentCount << "users";
}

ConnectionManager::ConnectionStats ConnectionManager::getStats() const
{
    ConnectionStats stats;
    stats.totalConnections = getConnectionCount();
    stats.authenticatedConnections = getAuthenticatedCount();
    stats.activeConnections = m_counters.get("active_connections");
    stats.totalMessagesSent = m_counters.get("total_messages_sent");
    stats.totalMessagesReceived = m_counters.get("total_messages_received");
    stats.totalBytesTransferred = m_counters.get("total_bytes_transferred");
    stats.lastUpdate = QDateTime::currentDateTime();
    
    return stats;
}

int ConnectionManager::getConnectionCount() const
{
    return m_socketConnections.size();
}

int ConnectionManager::getAuthenticatedCount() const
{
    return m_userConnections.size();
}

QStringList ConnectionManager::getConnectedUserIds() const
{
    QStringList userIds;
    
    m_userConnections.forEach([&userIds](qint64 userId, std::shared_ptr<ClientConnection> conn) {
        Q_UNUSED(conn)
        userIds.append(QString::number(userId));
    });
    
    return userIds;
}

void ConnectionManager::cleanupInactiveConnections(int timeoutSeconds)
{
    QDateTime cutoffTime = QDateTime::currentDateTime().addSecs(-timeoutSeconds);
    QList<QSslSocket*> socketsToRemove;
    
    m_socketConnections.forEach([&cutoffTime, &socketsToRemove](QSslSocket* socket, std::shared_ptr<ClientConnection> conn) {
        if (conn && conn->lastActivity < cutoffTime) {
            socketsToRemove.append(socket);
        }
    });
    
    for (QSslSocket* socket : socketsToRemove) {
        auto connection = m_socketConnections.value(socket);
        if (connection) {
            qCInfo(connectionManager) << "Removing inactive connection for user:" << connection->getUserId();
            emit inactiveConnectionDetected(socket);
            removeConnectionInternal(connection);
        }
    }
    
    if (!socketsToRemove.isEmpty()) {
        qCInfo(connectionManager) << "Cleaned up" << socketsToRemove.size() << "inactive connections";
    }
}

void ConnectionManager::cleanupDisconnectedSockets()
{
    QList<QSslSocket*> socketsToRemove;
    
    m_socketConnections.forEach([&socketsToRemove](QSslSocket* socket, std::shared_ptr<ClientConnection> conn) {
        Q_UNUSED(conn)
        if (!socket || socket->state() == QAbstractSocket::UnconnectedState) {
            socketsToRemove.append(socket);
        }
    });
    
    for (QSslSocket* socket : socketsToRemove) {
        removeConnection(socket);
    }
    
    if (!socketsToRemove.isEmpty()) {
        qCInfo(connectionManager) << "Cleaned up" << socketsToRemove.size() << "disconnected sockets";
    }
}

void ConnectionManager::setHeartbeatInterval(int seconds)
{
    m_heartbeatInterval.storeRelease(seconds);
    m_heartbeatTimer->setInterval(seconds * 1000);
    qCInfo(connectionManager) << "Heartbeat interval set to" << seconds << "seconds";
}

void ConnectionManager::setMaxConnections(int maxConnections)
{
    m_maxConnections.storeRelease(maxConnections);
    qCInfo(connectionManager) << "Max connections set to" << maxConnections;
}

void ConnectionManager::setInactivityTimeout(int seconds)
{
    m_inactivityTimeout.storeRelease(seconds);
    qCInfo(connectionManager) << "Inactivity timeout set to" << seconds << "seconds";
}

void ConnectionManager::performMaintenance()
{
    qCDebug(connectionManager) << "Performing maintenance...";
    
    cleanupDisconnectedSockets();
    cleanupInactiveConnections(m_inactivityTimeout.loadAcquire());
    
    // 更新统计信息
    updateCounters();
    
    qCDebug(connectionManager) << "Maintenance completed. Active connections:" << getConnectionCount();
}

void ConnectionManager::checkHeartbeats()
{
    // 这里可以实现心跳检查逻辑
    // 例如向所有连接发送心跳包，检查响应等
    qCDebug(connectionManager) << "Heartbeat check completed";
}

void ConnectionManager::removeConnectionInternal(std::shared_ptr<ClientConnection> connection)
{
    if (!connection) {
        return;
    }
    
    QSslSocket* socket = connection->getSocket();
    qint64 userId = connection->getUserId();
    
    // 更新状态
    connection->state.exchange(ConnectionState::Disconnected);
    
    // 从映射中移除
    if (socket) {
        m_socketConnections.remove(socket);
    }
    
    if (userId > 0) {
        m_userConnections.remove(userId);
    }
    
    // 更新计数器
    incrementCounter("disconnected_connections");
    if (m_counters.get("active_connections") > 0) {
        addToCounter("active_connections", -1);
    }
    
    logConnectionEvent("CONNECTION_REMOVED", socket, userId);
    emit connectionRemoved(socket, userId);
    
    qCInfo(connectionManager) << "Connection removed for user:" << userId;
}

bool ConnectionManager::isSocketValid(QSslSocket* socket) const
{
    return socket && socket->state() != QAbstractSocket::UnconnectedState;
}

void ConnectionManager::logConnectionEvent(const QString& event, QSslSocket* socket, qint64 userId) const
{
    QString socketInfo = socket ? QString("Socket:%1").arg(reinterpret_cast<quintptr>(socket)) : "Socket:null";
    QString userInfo = userId > 0 ? QString("User:%1").arg(userId) : "User:unknown";
    
    qCDebug(connectionManager) << event << socketInfo << userInfo;
}

void ConnectionManager::incrementCounter(const QString& key)
{
    m_counters.increment(key);
}

void ConnectionManager::addToCounter(const QString& key, int value)
{
    m_counters.add(key, value);
}

void ConnectionManager::updateCounters()
{
    // 更新实时统计
    m_counters.reset("current_connections");
    m_counters.add("current_connections", getConnectionCount());
    
    m_counters.reset("current_authenticated");
    m_counters.add("current_authenticated", getAuthenticatedCount());
}
