#include "AdminManager.h"
#include "../database/Database.h"
#include "../config/ServerConfig.h"
#include <QCryptographicHash>
#include <QUuid>
#include <QRegularExpression>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QRandomGenerator>
#include <QSqlQuery>
#include <algorithm>

Q_LOGGING_CATEGORY(adminManager, "qkchat.server.adminmanager")

AdminManager::AdminManager(Database *database, QObject *parent)
    : QObject(parent)
    , _database(database)
    , _maxFailedAttempts(DEFAULT_MAX_FAILED_ATTEMPTS)
    , _lockoutDuration(DEFAULT_LOCKOUT_DURATION)
    , _passwordMinLength(DEFAULT_PASSWORD_MIN_LENGTH)
    , _requireSpecialChars(true)
    , _requireNumbers(true)
    , _requireUppercase(true)
    , _requireLowercase(true)
{
    loadSecurityConfig();
    
    // 设置清理定时器
    QTimer *cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, this, &AdminManager::cleanupExpiredLocks);
    cleanupTimer->start(60000); // 每分钟清理一次
    
    qCInfo(adminManager) << "AdminManager initialized";
}

AdminManager::~AdminManager()
{
    saveSecurityConfig();
    qCInfo(adminManager) << "AdminManager destroyed";
}

bool AdminManager::createAdminAccount(const QString &username, const QString &password, 
                                    const QString &displayName)
{
    QMutexLocker locker(&_mutex);
    
    if (!_database) {
        qCCritical(adminManager) << "Database not available";
        return false;
    }
    
    // 验证输入参数
    if (username.isEmpty() || password.isEmpty()) {
        qCWarning(adminManager) << "Invalid input parameters";
        return false;
    }
    
    // 检查用户名是否已存在
    if (isAdminAccount(username)) {
        qCWarning(adminManager) << "Admin account already exists:" << username;
        return false;
    }
    
    // 验证密码强度
    if (!enforcePasswordPolicy(password)) {
        qCWarning(adminManager) << "Password does not meet security requirements";
        return false;
    }
    
    // 生成安全的盐值
    QString salt = generateSalt();
    
    // 哈希密码
    QString passwordHash = hashPassword(password, salt);
    
    // 准备邮箱和显示名称

    QString adminDisplayName = displayName.isEmpty() ? QString("管理员-%1").arg(username) : displayName;
    
    // 创建管理员账号
    QSqlQuery query(_database->getDatabase());
    query.prepare(R"(
        INSERT INTO users (username, password_hash, salt, display_name, status, created_at)
        VALUES (?, ?, ?, ?, 'active', NOW())
    )");
    
    query.addBindValue(username);
    query.addBindValue(passwordHash);
    query.addBindValue(salt);
    query.addBindValue(adminDisplayName);
    
    if (!_database->executeQuery(query)) {
        qCCritical(adminManager) << "Failed to create admin account:" << query.lastError().text();
        return false;
    }
    
    qint64 adminId = query.lastInsertId().toLongLong();
    
    // 记录审计日志
    logAdminAction(adminId, "admin_account_created", QString("Created admin account: %1").arg(username));
    
    // 发送信号
    emit adminAccountCreated(adminId, username);
    
    qCInfo(adminManager) << "Admin account created successfully:" << username << "ID:" << adminId;
    return true;
}

bool AdminManager::updateAdminPassword(qint64 adminId, const QString &newPassword)
{
    QMutexLocker locker(&_mutex);
    
    if (!_database) {
        qCCritical(adminManager) << "Database not available";
        return false;
    }
    
    // 验证管理员账号
    if (!isAdminAccount(adminId)) {
        qCWarning(adminManager) << "User is not an admin account:" << adminId;
        return false;
    }
    
    // 验证密码强度
    if (!enforcePasswordPolicy(newPassword)) {
        qCWarning(adminManager) << "New password does not meet security requirements";
        return false;
    }
    
    // 生成新的盐值和哈希
    QString newSalt = generateSalt();
    QString newPasswordHash = hashPassword(newPassword, newSalt);
    
    // 更新密码
    QSqlQuery query(_database->getDatabase());
    query.prepare("UPDATE users SET password_hash = ?, salt = ?, updated_at = NOW() WHERE id = ?");
    query.addBindValue(newPasswordHash);
    query.addBindValue(newSalt);
    query.addBindValue(adminId);
    
    if (!_database->executeQuery(query)) {
        qCCritical(adminManager) << "Failed to update admin password:" << query.lastError().text();
        return false;
    }
    
    // 重置失败尝试次数
    resetFailedAttempts(adminId);
    
    // 记录审计日志
    logAdminAction(adminId, "admin_password_updated", "Admin password updated");
    
    // 发送信号
    emit adminAccountUpdated(adminId);
    
    qCInfo(adminManager) << "Admin password updated successfully for ID:" << adminId;
    return true;
}

bool AdminManager::deleteAdminAccount(qint64 adminId)
{
    QMutexLocker locker(&_mutex);
    
    if (!_database) {
        qCCritical(adminManager) << "Database not available";
        return false;
    }
    
    // 验证管理员账号
    if (!isAdminAccount(adminId)) {
        qCWarning(adminManager) << "User is not an admin account:" << adminId;
        return false;
    }
    
    // 获取管理员信息用于日志
    Database::UserInfo userInfo = _database->getUserById(adminId);
    
    // 删除管理员账号
    QSqlQuery query(_database->getDatabase());
    query.prepare("DELETE FROM users WHERE id = ?");
    query.addBindValue(adminId);
    
    if (!_database->executeQuery(query)) {
        qCCritical(adminManager) << "Failed to delete admin account:" << query.lastError().text();
        return false;
    }
    
    // 清理锁定状态
    _lockedAccounts.remove(adminId);
    _failedAttempts.remove(adminId);
    
    // 记录审计日志
    logAdminAction(-1, "admin_account_deleted", QString("Deleted admin account: %1 (ID: %2)").arg(userInfo.username).arg(adminId));
    
    // 发送信号
    emit adminAccountDeleted(adminId);
    
    qCInfo(adminManager) << "Admin account deleted successfully:" << userInfo.username << "ID:" << adminId;
    return true;
}

bool AdminManager::isAdminAccount(qint64 userId) const
{
    QMutexLocker locker(&_mutex);
    
    if (!_database) {
        return false;
    }
    
    Database::UserInfo userInfo = _database->getUserById(userId);
    return userInfo.id > 0 && (userInfo.username == "admin" || userInfo.username.startsWith("admin_"));
}

bool AdminManager::isAdminAccount(const QString &username) const
{
    QMutexLocker locker(&_mutex);
    
    if (!_database) {
        return false;
    }
    
    Database::UserInfo userInfo = _database->getUserByUsername(username);
    return userInfo.id > 0 && (userInfo.username == "admin" || userInfo.username.startsWith("admin_"));
}

bool AdminManager::authenticateAdmin(const QString &username, const QString &password)
{
    QMutexLocker locker(&_mutex);
    
    if (!_database) {
        qCCritical(adminManager) << "Database not available";
        return false;
    }
    
    // 检查账号是否存在
    Database::UserInfo userInfo = _database->getUserByUsername(username);
    if (userInfo.id <= 0) {
        emit adminLoginFailed(username, "Account not found");
        return false;
    }
    
    // 检查是否为管理员账号
    if (!isAdminAccount(userInfo.id)) {
        emit adminLoginFailed(username, "Not an admin account");
        return false;
    }
    
    // 检查账号是否被锁定
    if (isAccountLocked(userInfo.id)) {
        int remainingTime = getRemainingLockoutTime(userInfo.id);
        emit adminLoginFailed(username, QString("Account locked for %1 more minutes").arg(remainingTime));
        return false;
    }
    
    // 验证密码
    if (!verifyPassword(password, userInfo.passwordHash, userInfo.salt)) {
        incrementFailedAttempts(userInfo.id);
        
        int failedAttempts = getFailedAttempts(userInfo.id);
        if (failedAttempts >= _maxFailedAttempts) {
            lockAccount(userInfo.id, _lockoutDuration);
            emit adminLoginFailed(username, "Account locked due to too many failed attempts");
        } else {
            emit adminLoginFailed(username, QString("Invalid password (%1/%2 attempts remaining)").arg(_maxFailedAttempts - failedAttempts).arg(_maxFailedAttempts));
        }
        return false;
    }
    
    // 登录成功
    resetFailedAttempts(userInfo.id);
    unlockAccount(userInfo.id);
    
    // 记录审计日志
    logAdminAction(userInfo.id, "admin_login_success", QString("Admin login from IP: %1").arg("unknown"));
    
    // 发送信号
    emit adminLoginSuccess(userInfo.id, username);
    
    qCInfo(adminManager) << "Admin login successful:" << username;
    return true;
}

bool AdminManager::validateAdminCredentials(const QString &username, const QString &password)
{
    QMutexLocker locker(&_mutex);
    
    if (!_database) {
        return false;
    }
    
    Database::UserInfo userInfo = _database->getUserByUsername(username);
    if (userInfo.id <= 0) {
        return false;
    }
    
    if (!isAdminAccount(userInfo.id)) {
        return false;
    }
    
    return verifyPassword(password, userInfo.passwordHash, userInfo.salt);
}

bool AdminManager::enforcePasswordPolicy(const QString &password)
{
    if (password.length() < _passwordMinLength) {
        return false;
    }
    
    if (_requireUppercase && !password.contains(QRegularExpression("[A-Z]"))) {
        return false;
    }
    
    if (_requireLowercase && !password.contains(QRegularExpression("[a-z]"))) {
        return false;
    }
    
    if (_requireNumbers && !password.contains(QRegularExpression("[0-9]"))) {
        return false;
    }
    
    if (_requireSpecialChars && !password.contains(QRegularExpression("[!@#$%^&*()_+\\-=\\[\\]{};':\"\\\\|,.<>\\/?]"))) {
        return false;
    }
    
    return true;
}

bool AdminManager::checkPasswordStrength(const QString &password)
{
    int score = 0;
    
    // 长度分数
    if (password.length() >= 8) score += 1;
    if (password.length() >= 12) score += 1;
    if (password.length() >= 16) score += 1;
    
    // 字符类型分数
    if (password.contains(QRegularExpression("[a-z]"))) score += 1;
    if (password.contains(QRegularExpression("[A-Z]"))) score += 1;
    if (password.contains(QRegularExpression("[0-9]"))) score += 1;
    if (password.contains(QRegularExpression("[!@#$%^&*()_+\\-=\\[\\]{};':\"\\\\|,.<>\\/?]"))) score += 1;
    
    // 复杂度分数
    if (password.contains(QRegularExpression("(.)\\1{2,}"))) score -= 1; // 重复字符
    if (password.contains(QRegularExpression("(abc|bcd|cde|def|efg|fgh|ghi|hij|ijk|jkl|klm|lmn|mno|nop|opq|pqr|qrs|rst|stu|tuv|uvw|vwx|wxy|xyz)"))) score -= 1; // 连续字符
    
    return score >= 4; // 至少4分才算强密码
}

QString AdminManager::generateSecurePassword()
{
    const QString chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+-=[]{}|;:,.<>?";
    QString password;
    
    // 确保包含所有必需字符类型
    password += QChar('a' + QRandomGenerator::global()->bounded(26)); // 小写字母
    password += QChar('A' + QRandomGenerator::global()->bounded(26)); // 大写字母
    password += QChar('0' + QRandomGenerator::global()->bounded(10)); // 数字
    password += "!@#$%^&*"[QRandomGenerator::global()->bounded(8)]; // 特殊字符
    
    // 添加随机字符
    for (int i = 4; i < 12; ++i) {
        password += chars[QRandomGenerator::global()->bounded(chars.length())];
    }
    
    // 打乱密码
    std::shuffle(password.begin(), password.end(), *QRandomGenerator::global());
    
    return password;
}

bool AdminManager::isAccountLocked(qint64 userId) const
{
    QMutexLocker locker(&_mutex);
    
    if (!_lockedAccounts.contains(userId)) {
        return false;
    }
    
    QDateTime lockTime = _lockedAccounts[userId];
    QDateTime now = QDateTime::currentDateTime();
    
    if (now > lockTime.addSecs(_lockoutDuration * 60)) {
        // 锁定时间已过期，清理锁定状态
        const_cast<AdminManager*>(this)->_lockedAccounts.remove(userId);
        return false;
    }
    
    return true;
}

bool AdminManager::isAccountLocked(const QString &username) const
{
    QMutexLocker locker(&_mutex);
    
    if (!_database) {
        return false;
    }
    
    Database::UserInfo userInfo = _database->getUserByUsername(username);
    if (userInfo.id <= 0) {
        return false;
    }
    
    return isAccountLocked(userInfo.id);
}

void AdminManager::lockAccount(qint64 userId, int durationMinutes)
{
    QMutexLocker locker(&_mutex);
    
    QDateTime lockTime = QDateTime::currentDateTime();
    _lockedAccounts[userId] = lockTime;
    
    // 记录审计日志
    logAdminAction(-1, "account_locked", QString("Account locked for %1 minutes").arg(durationMinutes));
    
    // 发送信号
    emit accountLocked(userId, durationMinutes);
    
    qCWarning(adminManager) << "Account locked:" << userId << "for" << durationMinutes << "minutes";
}

void AdminManager::unlockAccount(qint64 userId)
{
    QMutexLocker locker(&_mutex);
    
    _lockedAccounts.remove(userId);
    _failedAttempts.remove(userId);
    
    // 记录审计日志
    logAdminAction(-1, "account_unlocked", "Account unlocked");
    
    // 发送信号
    emit accountUnlocked(userId);
    
    qCInfo(adminManager) << "Account unlocked:" << userId;
}

int AdminManager::getRemainingLockoutTime(qint64 userId) const
{
    QMutexLocker locker(&_mutex);
    
    if (!_lockedAccounts.contains(userId)) {
        return 0;
    }
    
    QDateTime lockTime = _lockedAccounts[userId];
    QDateTime now = QDateTime::currentDateTime();
    QDateTime unlockTime = lockTime.addSecs(_lockoutDuration * 60);
    
    if (now >= unlockTime) {
        return 0;
    }
    
    return now.secsTo(unlockTime) / 60;
}

void AdminManager::logAdminAction(qint64 adminId, const QString &action, const QString &details)
{
    if (!_database) {
        return;
    }
    
    QVariantMap extraData;
    extraData["action"] = action;
    extraData["details"] = details;
    extraData["admin_id"] = adminId;
    
    _database->logEvent(Database::Info, "AdminManager", 
                       QString("Admin action: %1 - %2").arg(action).arg(details),
                       adminId, "", "", extraData);
}

QList<QVariantMap> AdminManager::getAdminAuditLogs(qint64 adminId, int limit)
{
    Q_UNUSED(adminId) // TODO: 实现按管理员ID过滤日志
    if (!_database) {
        return QList<QVariantMap>();
    }
    
    return _database->getSystemLogs(Database::Info, limit, 0);
}

int AdminManager::getAdminCount() const
{
    QMutexLocker locker(&_mutex);
    
    if (!_database) {
        return 0;
    }
    
    // 简化实现，实际应该查询管理员账号
    return _database->getTotalUserCount();
}

QList<QVariantMap> AdminManager::getAdminList() const
{
    QMutexLocker locker(&_mutex);
    
    QList<QVariantMap> adminList;
    
    if (!_database) {
        return adminList;
    }
    
    // 获取所有用户并过滤管理员
    QList<Database::UserInfo> allUsers = _database->getActiveUsers(1000);
    
    for (const Database::UserInfo &user : allUsers) {
        if (isAdminAccount(user.id)) {
            QVariantMap adminInfo;
            adminInfo["id"] = user.id;
            adminInfo["username"] = user.username;
    
            adminInfo["display_name"] = user.displayName;
            adminInfo["last_online"] = user.lastOnline;
            adminInfo["status"] = user.status;
            adminInfo["is_locked"] = isAccountLocked(user.id);
            adminInfo["failed_attempts"] = getFailedAttempts(user.id);
            
            adminList.append(adminInfo);
        }
    }
    
    return adminList;
}

void AdminManager::cleanupExpiredLocks()
{
    QMutexLocker locker(&_mutex);
    
    QDateTime now = QDateTime::currentDateTime();
    QList<qint64> expiredLocks;
    
    for (auto it = _lockedAccounts.begin(); it != _lockedAccounts.end(); ++it) {
        if (now > it.value().addSecs(_lockoutDuration * 60)) {
            expiredLocks.append(it.key());
        }
    }
    
    for (qint64 userId : expiredLocks) {
        _lockedAccounts.remove(userId);
        _failedAttempts.remove(userId);
        qCDebug(adminManager) << "Cleaned up expired lock for user:" << userId;
    }
}

QString AdminManager::hashPassword(const QString &password, const QString &salt)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData((password + salt).toUtf8());
    return QString(hash.result().toHex());
}

QString AdminManager::generateSalt()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool AdminManager::verifyPassword(const QString &password, const QString &hash, const QString &salt)
{
    QString computedHash = hashPassword(password, salt);
    return computedHash == hash;
}

void AdminManager::incrementFailedAttempts(qint64 userId)
{
    QMutexLocker locker(&_mutex);
    
    int currentAttempts = _failedAttempts.value(userId, 0);
    _failedAttempts[userId] = currentAttempts + 1;
    
    qCWarning(adminManager) << "Failed login attempt for user:" << userId << "Total attempts:" << _failedAttempts[userId];
}

void AdminManager::resetFailedAttempts(qint64 userId)
{
    QMutexLocker locker(&_mutex);
    
    _failedAttempts.remove(userId);
    
    qCDebug(adminManager) << "Reset failed attempts for user:" << userId;
}

int AdminManager::getFailedAttempts(qint64 userId) const
{
    QMutexLocker locker(&_mutex);
    
    return _failedAttempts.value(userId, 0);
}

bool AdminManager::hasAdminPermission(qint64 userId, const QString &permission) const
{
    Q_UNUSED(permission) // TODO: 实现具体权限检查
    // 简化实现，实际应该检查具体的权限
    return isAdminAccount(userId);
}

void AdminManager::loadSecurityConfig()
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/QK Team/QK Chat Server/admin_security.ini", QSettings::IniFormat);
    
    _maxFailedAttempts = settings.value("Security/max_failed_attempts", DEFAULT_MAX_FAILED_ATTEMPTS).toInt();
    _lockoutDuration = settings.value("Security/lockout_duration", DEFAULT_LOCKOUT_DURATION).toInt();
    _passwordMinLength = settings.value("Security/password_min_length", DEFAULT_PASSWORD_MIN_LENGTH).toInt();
    _requireSpecialChars = settings.value("Security/require_special_chars", true).toBool();
    _requireNumbers = settings.value("Security/require_numbers", true).toBool();
    _requireUppercase = settings.value("Security/require_uppercase", true).toBool();
    _requireLowercase = settings.value("Security/require_lowercase", true).toBool();
}

void AdminManager::saveSecurityConfig()
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/QK Team/QK Chat Server/admin_security.ini", QSettings::IniFormat);
    
    settings.setValue("Security/max_failed_attempts", _maxFailedAttempts);
    settings.setValue("Security/lockout_duration", _lockoutDuration);
    settings.setValue("Security/password_min_length", _passwordMinLength);
    settings.setValue("Security/require_special_chars", _requireSpecialChars);
    settings.setValue("Security/require_numbers", _requireNumbers);
    settings.setValue("Security/require_uppercase", _requireUppercase);
    settings.setValue("Security/require_lowercase", _requireLowercase);
} 