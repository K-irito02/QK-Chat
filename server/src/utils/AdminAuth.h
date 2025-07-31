#ifndef ADMINAUTH_H
#define ADMINAUTH_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QSettings>

/**
 * @brief 管理员认证工具类
 * 
 * 提供管理员身份验证功能，包括：
 * - 用户名密码验证
 * - 登录尝试限制
 * - 账户锁定机制
 */
class AdminAuth : public QObject
{
    Q_OBJECT
    
public:
    explicit AdminAuth(QObject *parent = nullptr);
    
    // 认证方法
    bool authenticate(const QString &username, const QString &password);
    
    // 锁定管理
    bool isAccountLocked() const;
    void lockAccount();
    void unlockAccount();
    int getRemainingLockoutTime() const;
    
    // 失败尝试管理
    int getFailedAttempts() const;
    void incrementFailedAttempts();
    void resetFailedAttempts();
    
    // 配置管理
    void loadConfig();
    QString getAdminUsername() const;
    
signals:
    void accountLocked();
    void accountUnlocked();
    void authenticationFailed(const QString &reason);
    
private:
    QString hashPassword(const QString &password) const;
    void saveState();
    void loadState();
    
    QSettings *_settings;
    
    QString _adminUsername;
    QString _adminPasswordHash;
    int _failedAttempts;
    QDateTime _lockoutTime;
    
    static const int MAX_FAILED_ATTEMPTS = 5;
    static const int LOCKOUT_DURATION_MINUTES = 30;
};

#endif // ADMINAUTH_H 