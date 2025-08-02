#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QObject>
#include <QHash>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QTimer>

/**
 * @brief 会话管理器
 * 
 * 管理用户会话，包括：
 * - 会话创建和验证
 * - 会话过期处理
 * - 会话清理
 */
class SessionManager : public QObject
{
    Q_OBJECT
    
public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager();
    
    // 会话管理
    QString createSession(qint64 userId, const QString &ipAddress, int expirationHours = 24);
    QString createSession(qint64 userId, const QString &deviceInfo, const QString &ipAddress, int expirationHours = 24);
    bool validateSession(const QString &sessionToken, qint64 &userId);
    bool updateSessionLastActive(const QString &sessionToken);
    bool removeSession(const QString &sessionToken);
    bool removeUserSessions(qint64 userId);
    void cleanExpiredSessions();
    
    // 会话信息查询
    qint64 getUserIdBySession(const QString &sessionToken);
    QString getDeviceInfo(const QString &sessionToken);
    QString getIpAddress(const QString &sessionToken);
    QDateTime getSessionExpiry(const QString &sessionToken);
    
    // 统计信息
    int getActiveSessionCount() const;
    int getUserSessionCount(qint64 userId) const;
    
signals:
    void sessionCreated(qint64 userId, const QString &sessionToken);
    void sessionRemoved(const QString &sessionToken);
    void sessionExpired(const QString &sessionToken);
    
private:
    struct SessionInfo {
        qint64 userId;
        QString deviceInfo;
        QString ipAddress;
        QDateTime createdAt;
        QDateTime lastActive;
        QDateTime expiresAt;
        bool isValid;
    };
    
    void setupCleanupTimer();
    QString generateSessionToken() const;
    bool isSessionExpired(const SessionInfo &session) const;
    
    QHash<QString, SessionInfo> _sessions;
    QHash<qint64, QStringList> _userSessions; // userId -> sessionTokens
    mutable QMutex _mutex;
    
    QTimer *_cleanupTimer;
    static const int CLEANUP_INTERVAL = 300000; // 5分钟
};

#endif // SESSIONMANAGER_H