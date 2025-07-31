#ifndef DATABASE_H
#define DATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QMutex>

/**
 * @brief 数据库操作类
 * 
 * 提供MySQL数据库访问功能，包括：
 * - 用户管理
 * - 消息存储
 * - 会话管理
 * - 统计数据
 */
class Database : public QObject
{
    Q_OBJECT
    
public:
    explicit Database(QObject *parent = nullptr);
    ~Database();
    
    // 数据库连接管理
    bool initialize();
    bool isConnected() const;
    void close();
    bool reconnect();
    
    // 用户相关操作
    struct UserInfo {
        qint64 id;
        QString username;
        QString email;
        QString passwordHash;
        QString avatarUrl;
        QString displayName;
        QString status;
        QDateTime lastOnline;
        QDateTime createdAt;
        QDateTime updatedAt;
    };
    
    bool createUser(const QString &username, const QString &email, const QString &passwordHash, const QString &avatarUrl = "");
    UserInfo getUserById(qint64 userId);
    UserInfo getUserByUsername(const QString &username);
    UserInfo getUserByEmail(const QString &email);
    bool updateUser(qint64 userId, const QVariantMap &data);
    bool deleteUser(qint64 userId);
    bool isUsernameAvailable(const QString &username);
    bool isEmailAvailable(const QString &email);
    QList<UserInfo> getActiveUsers(int limit = 100);
    bool updateUserLastOnline(qint64 userId, const QDateTime &lastOnline = QDateTime::currentDateTime());
    int getTotalUserCount() const;
    
    // 用户认证
    UserInfo authenticateUser(const QString &usernameOrEmail, const QString &passwordHash);
    
    // 会话管理
    struct SessionInfo {
        qint64 id;
        qint64 userId;
        QString sessionToken;
        QString deviceInfo;
        QString ipAddress;
        QDateTime expiresAt;
        QDateTime createdAt;
        QDateTime lastActivity;
    };
    
    QString createUserSession(qint64 userId, const QString &deviceInfo, const QString &ipAddress, int expirationHours = 24);
    SessionInfo getSessionByToken(const QString &sessionToken);
    bool updateSessionActivity(const QString &sessionToken);
    bool deleteSession(const QString &sessionToken);
    bool deleteUserSessions(qint64 userId);
    void cleanExpiredSessions();
    
    // 消息相关操作
    struct MessageInfo {
        qint64 id;
        QString messageId;
        qint64 senderId;
        qint64 receiverId;
        QString messageType;
        QString content;
        QString fileUrl;
        qint64 fileSize;
        QString deliveryStatus;
        QDateTime createdAt;
        QDateTime deliveredAt;
        QDateTime readAt;
    };
    
    bool saveMessage(const QString &messageId, qint64 senderId, qint64 receiverId, 
                    const QString &messageType, const QString &content, 
                    const QString &fileUrl = "", qint64 fileSize = 0);
    MessageInfo getMessageById(const QString &messageId);
    QList<MessageInfo> getMessagesBetweenUsers(qint64 userId1, qint64 userId2, int limit = 50, int offset = 0);
    bool updateMessageStatus(const QString &messageId, const QString &status);
    QList<MessageInfo> getRecentMessages(int limit = 100);
    
    // 好友关系
    struct FriendshipInfo {
        qint64 id;
        qint64 userId;
        qint64 friendId;
        QString status;
        QDateTime requestedAt;
        QDateTime acceptedAt;
    };
    
    bool addFriendRequest(qint64 userId, qint64 friendId);
    bool acceptFriendRequest(qint64 userId, qint64 friendId);
    bool deleteFriendship(qint64 userId, qint64 friendId);
    QList<FriendshipInfo> getUserFriends(qint64 userId);
    QList<FriendshipInfo> getPendingFriendRequests(qint64 userId);
    
    // 统计数据
    struct ServerStats {
        QDate statDate;
        int onlineUsers;
        int newRegistrations;
        int messagesSent;
        int filesTransferred;
        int totalUsers;
        int activeUsers;
        QDateTime createdAt;
        QDateTime updatedAt;
    };
    
    bool updateDailyStats(const ServerStats &stats);
    ServerStats getTodayStats();
    QList<ServerStats> getStatsHistory(const QDate &startDate, const QDate &endDate);
    
    // 系统日志
    enum LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };
    
    bool logEvent(LogLevel level, const QString &module, const QString &message, 
                 qint64 userId = -1, const QString &ipAddress = "", 
                 const QString &userAgent = "", const QVariantMap &extraData = QVariantMap());
    QList<QVariantMap> getSystemLogs(LogLevel minLevel = Info, int limit = 1000, int offset = 0);
    
signals:
    void databaseError(const QString &error);
    void connectionLost();
    void connectionRestored();
    
private:
    bool executeQuery(QSqlQuery &query);
    QString getConnectionName() const;
    bool createTables();
    void setupDatabase();
    
    QSqlDatabase _database;
    QString _connectionName;
    bool _isConnected;
    mutable QMutex _mutex;
    
    // 连接参数
    QString _host;
    int _port;
    QString _databaseName;
    QString _username;
    QString _password;
    int _connectTimeout;
    int _readTimeout;
};

#endif // DATABASE_H 