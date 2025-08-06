#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
#include <QSslSocket>
#include <QDateTime>
#include <QTimer>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <memory>
#include "../utils/LockFreeStructures.h"

Q_DECLARE_LOGGING_CATEGORY(connectionManager)

/**
 * @brief 连接状态枚举
 */
enum class ConnectionState {
    Connecting = 0,
    Connected = 1,
    Authenticated = 2,
    Disconnecting = 3,
    Disconnected = 4,
    Error = 5
};

/**
 * @brief 客户端连接信息
 */
struct ClientConnection {
    QSslSocket* socket{nullptr};
    qint64 userId{0};
    QString sessionToken{};
    QString ipAddress{};
    QString userAgent{};
    QDateTime connectedAt{};
    QDateTime lastActivity{};
    QByteArray readBuffer{};
    
    // 使用原子状态机管理连接状态
    AtomicStateMachine<ConnectionState> state{ConnectionState::Connecting};
    
    // 统计信息
    QAtomicInt messagesSent{0};
    QAtomicInt messagesReceived{0};
    QAtomicInt bytesTransferred{0};
    
    explicit ClientConnection(QSslSocket* sock) 
        : socket(sock)
        , userId(0)
        , connectedAt(QDateTime::currentDateTime())
        , lastActivity(QDateTime::currentDateTime())
    {
        if (sock) {
            ipAddress = sock->peerAddress().toString();
        }
    }
    
    bool isAuthenticated() const {
        return state.currentState() == ConnectionState::Authenticated;
    }
    
    bool isConnected() const {
        return state.isOneOf({ConnectionState::Connected, ConnectionState::Authenticated});
    }
    
    void updateActivity() {
        lastActivity = QDateTime::currentDateTime();
    }
    
    void incrementMessagesSent() {
        messagesSent.fetchAndAddOrdered(1);
    }
    
    void incrementMessagesReceived() {
        messagesReceived.fetchAndAddOrdered(1);
    }
    
    void addBytesTransferred(int bytes) {
        bytesTransferred.fetchAndAddOrdered(bytes);
    }
    
    void setUserId(qint64 id) {
        userId = id;
    }
    
    void setSessionToken(const QString& token) {
        sessionToken = token;
    }
    
    qint64 getUserId() const {
        return userId;
    }
    
    QString getSessionToken() const {
        return sessionToken;
    }
    
    QSslSocket* getSocket() const {
        return socket;
    }
};

/**
 * @brief 高性能连接管理器
 * 
 * 特性：
 * - 无锁数据结构减少竞争
 * - 读写分离优化并发访问
 * - 原子操作保证线程安全
 * - 自动清理过期连接
 */
class ConnectionManager : public QObject
{
    Q_OBJECT

public:
    struct ConnectionStats {
        int totalConnections{0};
        int authenticatedConnections{0};
        int activeConnections{0};
        qint64 totalMessagesSent{0};
        qint64 totalMessagesReceived{0};
        qint64 totalBytesTransferred{0};
        QDateTime lastUpdate{};
    };

    explicit ConnectionManager(QObject *parent = nullptr);
    ~ConnectionManager();

    // 连接管理
    bool addConnection(QSslSocket* socket);
    bool removeConnection(QSslSocket* socket);
    bool removeConnection(qint64 userId);
    
    // 连接查询
    std::shared_ptr<ClientConnection> getConnection(QSslSocket* socket) const;
    std::shared_ptr<ClientConnection> getConnectionByUserId(qint64 userId) const;
    std::shared_ptr<ClientConnection> getConnectionBySessionToken(const QString& sessionToken) const;
    QList<std::shared_ptr<ClientConnection>> getAllConnections() const;
    QList<std::shared_ptr<ClientConnection>> getAuthenticatedConnections() const;
    
    // 连接状态管理
    bool authenticateConnection(QSslSocket* socket, qint64 userId, const QString& sessionToken);
    bool updateConnectionActivity(QSslSocket* socket);
    bool setConnectionState(QSslSocket* socket, ConnectionState state);
    
    // 批量操作
    void broadcastToAll(const QByteArray& data);
    void broadcastToAuthenticated(const QByteArray& data);
    void broadcastToUsers(const QList<qint64>& userIds, const QByteArray& data);
    
    // 统计信息
    ConnectionStats getStats() const;
    int getConnectionCount() const;
    int getAuthenticatedCount() const;
    QStringList getConnectedUserIds() const;
    
    // 清理和维护
    void cleanupInactiveConnections(int timeoutSeconds = 300);
    void cleanupDisconnectedSockets();
    
    // 配置
    void setHeartbeatInterval(int seconds);
    void setMaxConnections(int maxConnections);
    void setInactivityTimeout(int seconds);

signals:
    void connectionAdded(QSslSocket* socket);
    void connectionRemoved(QSslSocket* socket, qint64 userId);
    void connectionAuthenticated(qint64 userId, const QString& sessionToken);
    void connectionStateChanged(QSslSocket* socket, ConnectionState oldState, ConnectionState newState);
    void maxConnectionsReached();
    void inactiveConnectionDetected(QSslSocket* socket);

private slots:
    void performMaintenance();
    void checkHeartbeats();

private:
    // 使用读写分离的并发映射
    ConcurrentMap<QSslSocket*, std::shared_ptr<ClientConnection>> m_socketConnections;
    ConcurrentMap<qint64, std::shared_ptr<ClientConnection>> m_userConnections;
    
    // 原子计数器
    AtomicCounters m_counters;
    
    // 配置参数
    QAtomicInt m_maxConnections{1000};
    QAtomicInt m_heartbeatInterval{30};
    QAtomicInt m_inactivityTimeout{300};
    
    // 维护定时器
    QTimer* m_maintenanceTimer;
    QTimer* m_heartbeatTimer;
    
    // 内部方法
    void updateCounters();
    void removeConnectionInternal(std::shared_ptr<ClientConnection> connection);
    bool isSocketValid(QSslSocket* socket) const;
    void logConnectionEvent(const QString& event, QSslSocket* socket, qint64 userId = 0) const;

    // 统计更新
    void incrementCounter(const QString& key);
    void addToCounter(const QString& key, int value);
};

#endif // CONNECTIONMANAGER_H
