#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QObject>
#include <QSslSocket>

class ChatClientConnection;
#include "ChatClientConnection.h"
#include <QDateTime>
#include <QTimer>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QStringList>
#include <QHash>
#include <QMutex>
#include <QByteArray>
#include <memory>

// Windows系统监控
#ifdef Q_OS_WIN
#include <pdh.h>
#include <windows.h>
#pragma comment(lib, "pdh.lib")
#endif

#include "ThreadManager.h"
#include "ConnectionManager.h"
#include "MessageEngine.h"
#include "SessionManager.h"
#include "../network/NetworkEventHandler.h"
#include "../network/QSslServer.h"
#include "../network/ProtocolParser.h"
#include "../database/Database.h"
#include "../database/DatabasePool.h"
#include "../cache/CacheManagerV2.h"
#include "../config/ServerConfig.h"
#include "../utils/LogManager.h"
#include "../utils/ThreadPool.h"

Q_DECLARE_LOGGING_CATEGORY(chatServer)

// 常量定义
const int CLEANUP_INTERVAL = 300000; // 5分钟
const int HEARTBEAT_TIMEOUT = 30000; // 30秒

// 前向声明
class ChatClientConnection;

/**
 * @brief 重构后的高性能聊天服务器
 * 
 * 新架构特性：
 * - 线程池分离：网络、消息、数据库、文件、服务线程池
 * - 无锁数据结构：减少锁竞争，提高并发性能
 * - 异步处理：网络事件和消息处理完全异步化
 * - 连接池管理：数据库连接池，支持读写分离
 * - 智能缓存：多级缓存，自动淘汰策略
 * - 实时监控：性能指标收集和健康检查
 */
class ChatServer : public QObject
{
    Q_OBJECT

public:
    struct ServerStats {
        // 连接统计
        int totalConnections{0};
        int authenticatedConnections{0};
        int activeConnections{0};
        
        // 消息统计
        qint64 totalMessages{0};
        qint64 processedMessages{0};
        qint64 failedMessages{0};
        
        // 性能统计
        int averageResponseTime{0};
        int maxResponseTime{0};
        int throughputPerSecond{0};
        
        // 系统统计
        int cpuUsage{0};
        int memoryUsage{0};
        QString uptime{};
        
        // 线程池统计
        ThreadManager::SystemStats threadStats{};
        
        // 数据库统计
        DatabasePool::PoolStats databaseStats{};
        
        QDateTime lastUpdate{};
    };

    explicit ChatServer(QObject *parent = nullptr);
    ~ChatServer();

    // 服务器控制
    bool startServer();
    void stopServer();
    void restartServer();
    bool isRunning() const;
    
    // 配置管理
    bool loadConfiguration(const QString& configFile = QString());
    void setMaxConnections(int maxConnections);
    void setHeartbeatInterval(int seconds);
    void setMessageQueueSize(int maxSize);
    
    // 统计信息
    ServerStats getServerStats() const;
    void resetAllStats();
    void refreshAllCaches(); // 刷新所有缓存数据
    
    // 消息发送接口
    bool sendMessageToUser(qint64 userId, const QJsonObject& message);
    bool sendMessageToUsers(const QList<qint64>& userIds, const QJsonObject& message);
    void broadcastMessage(const QJsonObject& message);
    void broadcastToAuthenticated(const QJsonObject& message);
    
    // 用户管理
    QStringList getOnlineUsers() const;
    QStringList getConnectedUsers() const;
    int getOnlineUserCount() const;
    int getConnectionCount() const;
    int getTotalUserCount() const;
    int getMessagesCount() const;
    QString getUptime() const;
    int getCpuUsage() const;
    int getMemoryUsage() const;
    bool kickUser(qint64 userId, const QString& reason = QString());
    
    // 消息发送
    bool sendMessageToUser(qint64 userId, const QByteArray& message);
    void broadcastMessage(const QByteArray& message);
    
    // 健康检查
    bool isHealthy() const;
    QString getHealthReport() const;
    
    // 数据库连接
    bool initializeDatabase();
    
    // 客户端管理
    QString getClientAddress(const QString& clientId) const;
    void sendJsonMessage(const QString& clientId, const QJsonObject& message);
    void sendErrorResponse(const QString& clientId, const QString& error, const QString& details = QString());
    
    // SSL配置
    bool configureSsl();
    
    // 获取服务器状态
    QJsonObject getServerStatus() const;
    
    // 获取数据库实例
    Database* getDatabase() const;
    
    // 获取客户端地址


signals:
    void serverStarted();
    void serverStopped();
    void serverError(const QString& error);
    
    void clientConnected(const QString& clientId, const QString& address);
    void clientDisconnected(const QString& clientId, const QString& address);
    void clientAuthenticated(qint64 userId);
    void userOnline(qint64 userId);
    void userOffline(qint64 userId);
    
    void messageReceived(qint64 fromUserId, qint64 toUserId, const QJsonObject& message);
    void messageProcessed(const QString& messageId);
    void messageFailed(const QString& messageId, const QString& error);
    
    void performanceAlert(const QString& message);
    void systemOverloaded();
    void healthStatusChanged(bool healthy);
    
    void databaseError(const QString& error);
    void databaseConnected();

private slots:
    // 系统维护
    void performSystemMaintenance();
    void performCleanup();
    void updateSystemStats();
    void checkSystemHealth();
    
    // 组件事件处理
    void onConnectionManagerEvent();
    void onMessageEngineEvent();
    void onThreadManagerEvent();

    // 客户端连接处理
    void onClientConnected(QSslSocket* socket);
    void onClientDisconnected(const QString& clientId);
    void handleClientData(const QString& clientId);
    void handleClientDisconnected(const QString& clientId);
    void handleSocketError(const QString& clientId, QAbstractSocket::SocketError error);

private:
    // 核心组件
    CustomSslServer* _sslServer;
    Database* _database;
    SessionManager* _sessionManager;
    ProtocolParser* _protocolParser;
    ThreadPool* _threadPool;
    QTimer* _cleanupTimer;
    CacheManagerV2* _cacheManager;
    
    // 客户端连接管理
    struct ClientInfo {
        QSslSocket* socket;
        QString clientId;
        QString address;
        quint16 port;
        QDateTime connectedTime;
        QDateTime lastActivity;
        bool isAuthenticated;
        qint64 userId;
        QString username;
        QByteArray messageBuffer;  // 消息缓冲区
    };
    
    QHash<QString, ClientInfo> _clients;
    mutable QMutex _clientsMutex;
    
    // 配置和状态
    QString _host;
    int _port;
    bool _isRunning;
    QDateTime _startTime;
    QDateTime _lastSystemInfoUpdate;
    
    // 统计信息
    qint64 _totalMessages;
    int _cachedCpuUsage;
    int _cachedMemoryUsage;
    int _cachedOnlineUserCount;
    int _cachedTotalUserCount;
    
    // 统计信息互斥锁
    mutable QMutex _statsMutex;
    
    // 系统信息定时器
    QTimer* _systemInfoTimer;
    
    // PDH系统监控
    mutable PDH_HQUERY _cpuQuery;
    mutable PDH_HCOUNTER _cpuTotal;
    mutable PDH_HQUERY _memQuery;
    mutable PDH_HCOUNTER _memTotal;
    mutable bool _pdhInitialized;
    mutable QMutex _pdhMutex;
    
    // 初始化方法
    bool initializeComponents();
    bool initializeCache();
    bool initializeNetwork();
    bool initializeMessageHandlers();
    
    // 配置方法
    void setupTimers();
    void setupSignalConnections();
    void loadDefaultConfiguration();
    void setupSslServer();
    void setupCleanupTimer();
    void setupSystemInfoTimer();
    void initializeEmailService();
    
    // SSL配置
    bool setupSslConfiguration();
    
    // 消息处理器
    void registerMessageHandlers();
    
    // 统计更新
    void updateConnectionStats();
    void updateMessageStats();
    void updatePerformanceStats();
    void updateSystemResourceStats();
    void updateSystemInfo();
    
    // 健康检查
    bool checkComponentHealth() const;
    bool checkResourceHealth() const;
    bool checkPerformanceHealth() const;
    void checkDatabaseHealth();
    
    // 系统信息获取
    int getCpuUsageInternal() const;
    int getMemoryUsageInternal() const;
    int getCpuUsageViaProcess() const;
    int getMemoryUsageViaProcess() const;
    
    // PDH系统监控方法
#ifdef Q_OS_WIN
    bool initializePdhCounters() const;
    int getCpuUsageViaPdh() const;
    int getMemoryUsageViaPdh() const;
    int getCpuUsageViaRegistry() const;
    int getMemoryUsageViaRegistry() const;
#endif
    
    // 错误处理
    void handleComponentError(const QString& component, const QString& error);
    void handleSystemError(const QString& error);
    
    // 客户端管理
    void onSslErrors(const QList<QSslError>& errors);
    void onPeerVerifyError(const QSslError& error);
    void cleanupConnections();
    void removeClient(QSslSocket* socket);
    void processClientMessage(const QString& clientId, const QByteArray& messageData);
    
    // 消息处理
    void handleLogoutRequest(const QString& clientId);
    void handleMessageRequest(const QString& clientId, const QVariantMap& data);
    void handleHeartbeat(const QString& clientId);
    void handleValidationRequest(const QString& clientId, const QVariantMap& data);
    void handleRegisterRequest(const QString& clientId, const QVariantMap& data);
    void handleEmailVerificationRequest(const QString& clientId, const QVariantMap& data);
    void handleSendEmailVerificationRequest(const QString& clientId, const QVariantMap& data);
    void handleEmailCodeVerificationRequest(const QString& clientId, const QVariantMap& data);
    void handleResendVerificationRequest(const QString& clientId, const QVariantMap& data);
    void handleLoginRequest(const QString& clientId, const QJsonObject& request);
    
    // 工具方法
    QString formatUptime() const;
    void logServerEvent(const QString& event, const QString& details = QString()) const;
    
    // 客户端查询
    std::shared_ptr<ChatClientConnection> getClientBySocket(QSslSocket* socket);
    std::shared_ptr<ChatClientConnection> getClientByUserId(qint64 userId);
};

#endif // CHATSERVER_H
