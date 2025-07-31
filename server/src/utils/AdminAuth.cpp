#include "AdminAuth.h"
#include "../config/ServerConfig.h"
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(adminAuth, "qkchat.server.adminauth")

AdminAuth::AdminAuth(QObject *parent)
    : QObject(parent)
    , _failedAttempts(0)
{
    // 初始化设置
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    _settings = new QSettings(configPath + "/admin_auth.ini", QSettings::IniFormat, this);
    
    loadConfig();
    loadState();
    
    qCInfo(adminAuth) << "AdminAuth initialized";
}

bool AdminAuth::authenticate(const QString &username, const QString &password)
{
    // 检查账户是否被锁定
    if (isAccountLocked()) {
        emit authenticationFailed("账户已被锁定，请稍后再试");
        return false;
    }
    
    // 验证用户名
    if (username != _adminUsername) {
        incrementFailedAttempts();
        emit authenticationFailed("用户名或密码错误");
        qCWarning(adminAuth) << "Invalid username attempt:" << username;
        return false;
    }
    
    // 验证密码
    QString hashedPassword = hashPassword(password);
    if (hashedPassword != _adminPasswordHash) {
        incrementFailedAttempts();
        emit authenticationFailed("用户名或密码错误");
        qCWarning(adminAuth) << "Invalid password attempt for user:" << username;
        return false;
    }
    
    // 认证成功，重置失败计数
    resetFailedAttempts();
    qCInfo(adminAuth) << "Authentication successful for user:" << username;
    return true;
}

bool AdminAuth::isAccountLocked() const
{
    if (_lockoutTime.isNull()) {
        return false;
    }
    
    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime unlockTime = _lockoutTime.addSecs(LOCKOUT_DURATION_MINUTES * 60);
    
    return currentTime < unlockTime;
}

void AdminAuth::lockAccount()
{
    _lockoutTime = QDateTime::currentDateTime();
    saveState();
    emit accountLocked();
    qCWarning(adminAuth) << "Account locked due to too many failed attempts";
}

void AdminAuth::unlockAccount()
{
    _lockoutTime = QDateTime();
    _failedAttempts = 0;
    saveState();
    emit accountUnlocked();
    qCInfo(adminAuth) << "Account unlocked";
}

int AdminAuth::getRemainingLockoutTime() const
{
    if (!isAccountLocked()) {
        return 0;
    }
    
    QDateTime currentTime = QDateTime::currentDateTime();
    QDateTime unlockTime = _lockoutTime.addSecs(LOCKOUT_DURATION_MINUTES * 60);
    
    return currentTime.secsTo(unlockTime);
}

int AdminAuth::getFailedAttempts() const
{
    return _failedAttempts;
}

void AdminAuth::incrementFailedAttempts()
{
    _failedAttempts++;
    
    if (_failedAttempts >= MAX_FAILED_ATTEMPTS) {
        lockAccount();
    } else {
        saveState();
    }
    
    qCWarning(adminAuth) << "Failed attempt count:" << _failedAttempts;
}

void AdminAuth::resetFailedAttempts()
{
    _failedAttempts = 0;
    _lockoutTime = QDateTime();
    saveState();
}

void AdminAuth::loadConfig()
{
    ServerConfig *config = ServerConfig::instance();
    
    _adminUsername = config->getValue("Security/admin_username", "admin").toString();
    QString plainPassword = config->getValue("Security/admin_password", "QKchat2024!").toString();
    _adminPasswordHash = hashPassword(plainPassword);
    
    qCInfo(adminAuth) << "Admin config loaded, username:" << _adminUsername;
}

QString AdminAuth::getAdminUsername() const
{
    return _adminUsername;
}

QString AdminAuth::hashPassword(const QString &password) const
{
    // 使用SHA-256哈希加密密码
    QByteArray hash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    return hash.toHex();
}

void AdminAuth::saveState()
{
    _settings->setValue("failed_attempts", _failedAttempts);
    _settings->setValue("lockout_time", _lockoutTime);
    _settings->sync();
}

void AdminAuth::loadState()
{
    _failedAttempts = _settings->value("failed_attempts", 0).toInt();
    _lockoutTime = _settings->value("lockout_time", QDateTime()).toDateTime();
    
    // 如果锁定时间已过期，自动解锁
    if (!_lockoutTime.isNull() && !isAccountLocked()) {
        unlockAccount();
    }
} 