#ifndef ADMINMANAGER_H
#define ADMINMANAGER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMutex>
#include <QLoggingCategory>

class Database;

/**
 * @brief 管理员账号管理类
 * 
 * 提供安全的管理员账号管理功能：
 * - 管理员账号创建和验证
 * - 密码安全策略
 * - 账号锁定机制
 * - 审计日志
 * - 权限管理
 */
class AdminManager : public QObject
{
    Q_OBJECT
    
public:
    explicit AdminManager(Database *database, QObject *parent = nullptr);
    ~AdminManager();
    
    // 管理员账号管理
    bool createAdminAccount(const QString &username, const QString &password, 
                          const QString &displayName = "");
    bool updateAdminPassword(qint64 adminId, const QString &newPassword);
    bool deleteAdminAccount(qint64 adminId);
    bool isAdminAccount(qint64 userId) const;
    bool isAdminAccount(const QString &username) const;
    
    // 管理员验证
    bool authenticateAdmin(const QString &username, const QString &password);
    bool validateAdminCredentials(const QString &username, const QString &password);
    
    // 安全策略
    bool enforcePasswordPolicy(const QString &password);
    bool checkPasswordStrength(const QString &password);
    QString generateSecurePassword();
    
    // 账号锁定
    bool isAccountLocked(qint64 userId) const;
    bool isAccountLocked(const QString &username) const;
    void lockAccount(qint64 userId, int durationMinutes = 30);
    void unlockAccount(qint64 userId);
    int getRemainingLockoutTime(qint64 userId) const;
    
    // 审计和日志
    void logAdminAction(qint64 adminId, const QString &action, const QString &details = "");
    QList<QVariantMap> getAdminAuditLogs(qint64 adminId = -1, int limit = 100);
    
    // 统计信息
    int getAdminCount() const;
    QList<QVariantMap> getAdminList() const;
    
signals:
    void adminAccountCreated(qint64 adminId, const QString &username);
    void adminAccountUpdated(qint64 adminId);
    void adminAccountDeleted(qint64 adminId);
    void adminLoginSuccess(qint64 adminId, const QString &username);
    void adminLoginFailed(const QString &username, const QString &reason);
    void accountLocked(qint64 userId, int durationMinutes);
    void accountUnlocked(qint64 userId);
    
private slots:
    void cleanupExpiredLocks();
    
private:
    // 密码安全
    QString hashPassword(const QString &password, const QString &salt);
    QString generateSalt();
    bool verifyPassword(const QString &password, const QString &hash, const QString &salt);
    
    // 锁定管理
    void incrementFailedAttempts(qint64 userId);
    void resetFailedAttempts(qint64 userId);
    int getFailedAttempts(qint64 userId) const;
    
    // 权限检查
    bool hasAdminPermission(qint64 userId, const QString &permission) const;
    
    // 配置管理
    void loadSecurityConfig();
    void saveSecurityConfig();
    
    Database *_database;
    mutable QMutex _mutex;
    
    // 安全配置
    int _maxFailedAttempts;
    int _lockoutDuration;
    int _passwordMinLength;
    bool _requireSpecialChars;
    bool _requireNumbers;
    bool _requireUppercase;
    bool _requireLowercase;
    
    // 锁定状态缓存
    QHash<qint64, QDateTime> _lockedAccounts;
    QHash<qint64, int> _failedAttempts;
    
    static const int DEFAULT_MAX_FAILED_ATTEMPTS = 5;
    static const int DEFAULT_LOCKOUT_DURATION = 30; // 分钟
    static const int DEFAULT_PASSWORD_MIN_LENGTH = 8;
};

#endif // ADMINMANAGER_H 