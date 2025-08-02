#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include "../network/QSslServer.h"
#include <QSslSocket>
#include <QTimer>
#include "../utils/ThreadPool.h"
#include <QMutex>
#include <QHash>
#include <QMutexLocker>
#include <QDateTime>
#include <QByteArray>
#include <QVariantMap>
#include <QList>
#include <QSslError>
#include <QString>
#include <QStringList>
#include <QHostAddress>

class Database;
class SessionManager;
class ProtocolParser;

/**
 * @brief 聊天服务器核心类
 * 
 * 提供核心服务器功能：
 * - SSL连接管理
 * - 消息路由
 * - 会话管理
 * - 协议解析
 */
class ChatServer : public QObject
{
    Q_OBJECT
    
public:
    explicit ChatServer(QObject *parent = nullptr);
    ~ChatServer();
    
    // 服务器控制
    bool startServer();
    void stopServer();
    void restartServer();
    bool isRunning() const;
    bool initializeDatabase();
    
    // 状态查询
    int getOnlineUserCount() const;
    int getConnectionCount() const;
    QStringList getConnectedUsers() const;
    
    // Dashboard统计方法
    int getTotalUserCount() const;
    int getMessagesCount() const { QMutexLocker locker(&_statsMutex); return _totalMessages; }
    QString getUptime() const;
    int getCpuUsage() const;
    int getMemoryUsage() const;
    
    // 消息发送
    bool sendMessageToUser(qint64 userId, const QByteArray &message);
    void broadcastMessage(const QByteArray &message);
    
signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString &error);
    
    void clientConnected(qint64 socketId);
    void clientDisconnected(qint64 socketId);
    
    void messageReceived(qint64 fromUserId, qint64 toUserId, const QString &message);
    void userOnline(qint64 userId);
    void userOffline(qint64 userId);
    
public:
    struct ClientConnection {
        QSslSocket *socket;
        qint64 userId;
        QString sessionToken;
        QDateTime lastActivity;
        QByteArray readBuffer;
    };
    
private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onClientDataReceived();
    void onSslErrors(const QList<QSslError> &errors);
    void cleanupConnections();
    
private:
    void setupSslServer();
    void setupCleanupTimer();
    void processClientMessage(ClientConnection *client, const QByteArray &data);
    void handleLoginRequest(ClientConnection *client, const QVariantMap &data);
    void handleLogoutRequest(ClientConnection *client);
    void handleMessageRequest(ClientConnection *client, const QVariantMap &data);
    void handleHeartbeat(ClientConnection *client);
    void handleRegisterRequest(ClientConnection *client, const QVariantMap &data);
    void handleEmailVerificationRequest(ClientConnection *client, const QVariantMap &data);
    void handleEmailCodeVerificationRequest(ClientConnection *client, const QVariantMap &data);
    void handleSendEmailVerificationRequest(ClientConnection *client, const QVariantMap &data);
    void handleResendVerificationRequest(ClientConnection *client, const QVariantMap &data);
    
    void removeClient(QSslSocket *socket);
    ClientConnection *getClientBySocket(QSslSocket *socket);
    ClientConnection *getClientByUserId(qint64 userId);
    
    CustomSslServer *_sslServer;
    Database *_database;
    SessionManager *_sessionManager;
    ProtocolParser *_protocolParser;
    ThreadPool *_threadPool;
    QTimer *_cleanupTimer;
    
    QHash<QSslSocket*, ClientConnection*> _clients;
    QHash<qint64, ClientConnection*> _userConnections;
    mutable QMutex _clientsMutex;
    
    QString _host;
    int _port;
    bool _isRunning;
    
    // 统计相关成员变量
    QDateTime _startTime;
    int _totalMessages;
    mutable QMutex _statsMutex;
    
    static const int HEARTBEAT_TIMEOUT = 90000; // 90秒
    static const int CLEANUP_INTERVAL = 60000;  // 60秒
};

#endif // CHATSERVER_H