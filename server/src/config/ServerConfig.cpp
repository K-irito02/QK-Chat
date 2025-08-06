#include "ServerConfig.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QLoggingCategory>
#include <QThread>
#include <QFileInfo>
#include <QFile>

Q_LOGGING_CATEGORY(serverConfig, "qkchat.server.config")

ServerConfig* ServerConfig::_instance = nullptr;

ServerConfig::ServerConfig(QObject *parent)
    : QObject(parent)
    , _settings(nullptr)
{
    initializeDefaults();
}

ServerConfig::~ServerConfig()
{
    if (_settings) {
        _settings->deleteLater();
    }
}

ServerConfig* ServerConfig::instance()
{
    if (!_instance) {
        _instance = new ServerConfig();
    }
    return _instance;
}

bool ServerConfig::loadConfig(const QString &configFile)
{
    QString configPath = configFile.isEmpty() ? getDefaultConfigPath() : configFile;
    
    // 确保配置目录存在
    QFileInfo fileInfo(configPath);
    QDir().mkpath(fileInfo.absolutePath());
    
    if (_settings) {
        _settings->deleteLater();
    }
    
    _settings = new QSettings(configPath, QSettings::IniFormat, this);
    _configFile = configPath;
    
    // 清空缓存
    _configCache.clear();
    
    // 检查配置文件是否存在，如果不存在则创建默认配置
    if (!QFile::exists(configPath)) {
        qCWarning(serverConfig) << "Config file not found, creating default:" << configPath;
        saveConfig();
    }
    
    emit configLoaded();
    qCInfo(serverConfig) << "Configuration loaded from:" << configPath;
    
    return true;
}

bool ServerConfig::saveConfig()
{
    if (!_settings) {
        qCWarning(serverConfig) << "Settings not initialized";
        return false;
    }
    
    _settings->sync();
    
    if (_settings->status() != QSettings::NoError) {
        qCWarning(serverConfig) << "Failed to save configuration";
        return false;
    }
    
    emit configSaved();
    qCInfo(serverConfig) << "Configuration saved to:" << _configFile;
    
    return true;
}

void ServerConfig::reloadConfig()
{
    loadConfig(_configFile);
}

QVariant ServerConfig::getValue(const QString &key, const QVariant &defaultValue) const
{
    // 首先检查缓存
    if (_configCache.contains(key)) {
        return _configCache[key];
    }
    
    if (!_settings) {
        return defaultValue;
    }
    
    QVariant value = _settings->value(key, defaultValue);
    _configCache[key] = value;
    
    return value;
}

void ServerConfig::setValue(const QString &key, const QVariant &value)
{
    if (!_settings) {
        qCWarning(serverConfig) << "Settings not initialized";
        return;
    }
    
    _settings->setValue(key, value);
    _configCache[key] = value;
    
    emit configChanged(key, value);
}

QString ServerConfig::getServerHost() const
{
    return getValue("Server/host", "localhost").toString();
}

int ServerConfig::getServerPort() const
{
    return getValue("Server/port", 8443).toInt();
}

int ServerConfig::getAdminPort() const
{
    return getValue("Server/admin_port", 8080).toInt();
}

int ServerConfig::getFileTransferPort() const
{
    return getValue("Server/file_transfer_port", 8444).toInt();
}

int ServerConfig::getMaxConnections() const
{
    return getValue("Server/max_connections", 10000).toInt();
}

int ServerConfig::getThreadPoolSize() const
{
    return getValue("Server/thread_pool_size", QThread::idealThreadCount() * 2 + 1).toInt();
}

bool ServerConfig::isSslEnabled() const
{
    return getValue("Security/ssl_enabled", true).toBool();
}

QString ServerConfig::getRawCertificateFile() const
{
    return getValue("Security/cert_file", "../certs/server.crt").toString();
}

QString ServerConfig::getRawPrivateKeyFile() const
{
    return getValue("Security/key_file", "../certs/server.key").toString();
}

QString ServerConfig::getCaFile() const
{
    return getValue("Security/ca_file", "").toString();
}

QString ServerConfig::getSslCertificateFile() const
{
    QString certPath = getRawCertificateFile();
    qCDebug(serverConfig) << "Raw certificate path:" << certPath;

    if (QDir::isAbsolutePath(certPath)) {
        return certPath;
    }

    QDir appDir(QCoreApplication::applicationDirPath());
    QString absolutePath = appDir.absoluteFilePath(certPath);
    qCDebug(serverConfig) << "Application directory:" << appDir.path();
    qCDebug(serverConfig) << "Calculated absolute certificate path:" << absolutePath;

    return absolutePath;
}

QString ServerConfig::getSslPrivateKeyFile() const
{
    QString keyPath = getRawPrivateKeyFile();
    qCDebug(serverConfig) << "Raw private key path:" << keyPath;

    if (QDir::isAbsolutePath(keyPath)) {
        return keyPath;
    }

    QDir appDir(QCoreApplication::applicationDirPath());
    QString absolutePath = appDir.absoluteFilePath(keyPath);
    qCDebug(serverConfig) << "Application directory:" << appDir.path();
    qCDebug(serverConfig) << "Calculated absolute private key path:" << absolutePath;

    return absolutePath;
}

QString ServerConfig::getSslPrivateKeyPassword() const
{
    return getValue("Security/key_password", "").toString();
}

QString ServerConfig::getDatabaseType() const
{
    return getValue("Database/type", "mysql").toString();
}

QString ServerConfig::getDatabaseHost() const
{
    return getValue("Database/host", "localhost").toString();
}

int ServerConfig::getDatabasePort() const
{
    return getValue("Database/port", 3306).toInt();
}

QString ServerConfig::getDatabaseName() const
{
    return getValue("Database/name", "qkchat").toString();
}

QString ServerConfig::getDatabaseUsername() const
{
    return getValue("Database/username", "qkchat_user").toString();
}

QString ServerConfig::getDatabasePassword() const
{
    return getValue("Database/password", "qkchat_pass").toString();
}

int ServerConfig::getDatabasePoolSize() const
{
    return getValue("Database/pool_size", 10).toInt();
}

QString ServerConfig::getRedisHost() const
{
    return getValue("Redis/host", "localhost").toString();
}

int ServerConfig::getRedisPort() const
{
    return getValue("Redis/port", 6379).toInt();
}

QString ServerConfig::getRedisPassword() const
{
    return getValue("Redis/password", "").toString();
}

int ServerConfig::getRedisDatabase() const
{
    return getValue("Redis/database", 0).toInt();
}

QString ServerConfig::getAdminUsername() const
{
    return getValue("Security/admin_username", "admin").toString();
}

QString ServerConfig::getAdminPassword() const
{
    return getValue("Security/admin_password", "QKchat2024!").toString();
}

int ServerConfig::getSessionTimeout() const
{
    return getValue("Security/session_timeout", 1800).toInt();
}

int ServerConfig::getMaxLoginAttempts() const
{
    return getValue("Security/max_login_attempts", 5).toInt();
}

int ServerConfig::getLockoutDuration() const
{
    return getValue("Security/lockout_duration", 1800).toInt();
}

QString ServerConfig::getLogLevel() const
{
    return getValue("Logging/level", "info").toString();
}

QString ServerConfig::getLogFile() const
{
    return getValue("Logging/file", "logs/server.log").toString();
}

int ServerConfig::getMaxLogFileSize() const
{
    return getValue("Logging/max_file_size", 10485760).toInt(); // 10MB
}

int ServerConfig::getMaxLogFiles() const
{
    return getValue("Logging/max_files", 5).toInt();
}

// SMTP配置方法
QString ServerConfig::getSmtpHost() const
{
    return getValue("SMTP/host", "smtp.qq.com").toString();
}

int ServerConfig::getSmtpPort() const
{
    return getValue("SMTP/port", 587).toInt();
}

QString ServerConfig::getSmtpUsername() const
{
    return getValue("SMTP/username", "saokiritoasuna00@qq.com").toString();
}

QString ServerConfig::getSmtpPassword() const
{
    return getValue("SMTP/password", "ssvbzaqvotjcchjh").toString();
}

QString ServerConfig::getFromEmail() const
{
    return getValue("SMTP/from_email", "saokiritoasuna00@qq.com").toString();
}

QString ServerConfig::getFromName() const
{
    return getValue("SMTP/from_name", "QK Chat").toString();
}

void ServerConfig::initializeDefaults()
{
    // 如果没有设置对象，创建临时的默认配置
    _configCache.clear();
    
    // 服务器配置默认值
    _configCache["Server/host"] = "localhost";
    _configCache["Server/port"] = 8443;
    _configCache["Server/admin_port"] = 8080;
    _configCache["Server/file_transfer_port"] = 8444;
    _configCache["Server/max_connections"] = 10000;
    _configCache["Server/thread_pool_size"] = QThread::idealThreadCount() * 2 + 1;
    
    // 安全配置默认值
    _configCache["Security/ssl_enabled"] = true;
    _configCache["Security/cert_file"] = "../certs/server.crt";
    _configCache["Security/key_file"] = "../certs/server.key";
    _configCache["Security/admin_username"] = "admin";
    _configCache["Security/admin_password"] = "QKchat2024!";
    _configCache["Security/session_timeout"] = 1800;
    _configCache["Security/max_login_attempts"] = 5;
    _configCache["Security/lockout_duration"] = 1800;
    
    // 数据库配置默认值
    _configCache["Database/type"] = "mysql";
    _configCache["Database/host"] = "localhost";
    _configCache["Database/port"] = 3306;
    _configCache["Database/name"] = "qkchat";
    _configCache["Database/username"] = "qkchat_user";
    _configCache["Database/password"] = "3143285505";
    _configCache["Database/pool_size"] = 10;
    
    // Redis配置默认值
    _configCache["Redis/host"] = "localhost";
    _configCache["Redis/port"] = 6379;
    _configCache["Redis/password"] = "";
    _configCache["Redis/database"] = 0;
    
    // 日志配置默认值
    _configCache["Logging/level"] = "info";
    _configCache["Logging/file"] = "../logs/server.log";
    _configCache["Logging/max_file_size"] = 10485760; // 10MB
    _configCache["Logging/max_files"] = 5;
    
    // SMTP配置默认值
    _configCache["SMTP/host"] = "smtp.qq.com";
    _configCache["SMTP/port"] = 587;
    _configCache["SMTP/username"] = "saokiritoasuna00@qq.com";
    _configCache["SMTP/password"] = "ssvbzaqvotjcchjh";
    _configCache["SMTP/from_email"] = "saokiritoasuna00@qq.com";
    _configCache["SMTP/from_name"] = "QK Chat";
}

QString ServerConfig::getDefaultConfigPath() const
{
    // 首先尝试程序目录下的config文件
    QString appDir = QCoreApplication::applicationDirPath();
    QString configPath = appDir + "/config/dev.conf";
    
    if (QFile::exists(configPath)) {
        return configPath;
    }
    
    // 如果程序目录下没有，使用标准配置目录
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return configDir + "/server.conf";
}