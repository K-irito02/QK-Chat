#include "ConfigManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QSettings>

Q_LOGGING_CATEGORY(configManager, "qkchat.client.configmanager")

// 默认值定义
const QString ConfigManager::DEFAULT_PRIMARY_COLOR = "#2196F3";
const QString ConfigManager::DEFAULT_ACCENT_COLOR = "#FF4081";
const QString ConfigManager::DEFAULT_LANGUAGE = "zh_CN";
const QString ConfigManager::DEFAULT_SERVER_HOST = "localhost";

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent)
    , _settings(nullptr)
{
    // 初始化设置文件路径
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configPath);
    
    _settings = new QSettings(configPath + "/config.ini", QSettings::IniFormat, this);
    
    // 初始化默认值
    initializeDefaults();
    
    qCInfo(configManager) << "ConfigManager initialized with config path:" << configPath;
}

ConfigManager::~ConfigManager()
{
    if (_settings) {
        _settings->sync();
    }
}

void ConfigManager::setIsDarkTheme(bool isDark)
{
    if (_isDarkTheme != isDark) {
        _isDarkTheme = isDark;
        emit isDarkThemeChanged();
    }
}

void ConfigManager::setPrimaryColor(const QString &color)
{
    if (_primaryColor != color) {
        _primaryColor = color;
        emit primaryColorChanged();
    }
}

void ConfigManager::setAccentColor(const QString &color)
{
    if (_accentColor != color) {
        _accentColor = color;
        emit accentColorChanged();
    }
}

void ConfigManager::setLanguage(const QString &language)
{
    if (_language != language) {
        _language = language;
        emit languageChanged();
    }
}

void ConfigManager::setRememberPassword(bool remember)
{
    if (_rememberPassword != remember) {
        _rememberPassword = remember;
        emit rememberPasswordChanged();
    }
}

void ConfigManager::setAutoLogin(bool autoLogin)
{
    if (_autoLogin != autoLogin) {
        _autoLogin = autoLogin;
        emit autoLoginChanged();
    }
}

void ConfigManager::loadConfig()
{
    if (!_settings) {
        qCWarning(configManager) << "Settings not initialized";
        return;
    }
    
    // 主题配置
    _isDarkTheme = _settings->value("UI/dark_theme", DEFAULT_DARK_THEME).toBool();
    _primaryColor = _settings->value("UI/primary_color", DEFAULT_PRIMARY_COLOR).toString();
    _accentColor = _settings->value("UI/accent_color", DEFAULT_ACCENT_COLOR).toString();
    _language = _settings->value("UI/language", DEFAULT_LANGUAGE).toString();
    
    // 登录配置
    _rememberPassword = _settings->value("Security/remember_password", false).toBool();
    _autoLogin = _settings->value("Security/auto_login", false).toBool();
    
    // 服务器配置
    _serverHost = _settings->value("Network/server_host", DEFAULT_SERVER_HOST).toString();
    _serverPort = _settings->value("Network/server_port", DEFAULT_SERVER_PORT).toInt();
    
    // 发射信号通知界面更新
    emit isDarkThemeChanged();
    emit primaryColorChanged();
    emit accentColorChanged();
    emit languageChanged();
    emit rememberPasswordChanged();
    emit autoLoginChanged();
    emit configLoaded();
    
    qCInfo(configManager) << "Configuration loaded successfully";
}

void ConfigManager::saveConfig()
{
    if (!_settings) {
        qCWarning(configManager) << "Settings not initialized";
        return;
    }
    
    // 主题配置
    _settings->setValue("UI/dark_theme", _isDarkTheme);
    _settings->setValue("UI/primary_color", _primaryColor);
    _settings->setValue("UI/accent_color", _accentColor);
    _settings->setValue("UI/language", _language);
    
    // 登录配置
    _settings->setValue("Security/remember_password", _rememberPassword);
    _settings->setValue("Security/auto_login", _autoLogin);
    
    // 服务器配置
    _settings->setValue("Network/server_host", _serverHost);
    _settings->setValue("Network/server_port", _serverPort);
    
    _settings->sync();
    emit configSaved();
    
    qCInfo(configManager) << "Configuration saved successfully";
}

void ConfigManager::resetToDefault()
{
    initializeDefaults();
    saveConfig();
    
    qCInfo(configManager) << "Configuration reset to defaults";
}

QString ConfigManager::getServerHost() const
{
    return _serverHost;
}

int ConfigManager::getServerPort() const
{
    return _serverPort;
}

void ConfigManager::setServerConfig(const QString &host, int port)
{
    _serverHost = host;
    _serverPort = port;
}

void ConfigManager::saveUserCredentials(const QString &username, const QString &password)
{
    if (!_settings) {
        qCWarning(configManager) << "Settings not initialized";
        return;
    }
    
    // 加密存储密码
    QString encryptedPassword = encryptPassword(password);
    
    _settings->setValue("Credentials/username", username);
    _settings->setValue("Credentials/password", encryptedPassword);
    _settings->sync();
    
    qCInfo(configManager) << "User credentials saved for:" << username;
}

QString ConfigManager::loadUsername()
{
    if (!_settings) {
        qCWarning(configManager) << "Settings not initialized";
        return QString();
    }
    
    return _settings->value("Credentials/username").toString();
}

QString ConfigManager::loadPassword()
{
    if (!_settings) {
        qCWarning(configManager) << "Settings not initialized";
        return QString();
    }
    
    QString encryptedPassword = _settings->value("Credentials/password").toString();
    return decryptPassword(encryptedPassword);
}

void ConfigManager::clearUserCredentials()
{
    if (!_settings) {
        qCWarning(configManager) << "Settings not initialized";
        return;
    }
    
    _settings->remove("Credentials/username");
    _settings->remove("Credentials/password");
    _settings->sync();
    
    qCInfo(configManager) << "User credentials cleared";
}

void ConfigManager::initializeDefaults()
{
    _isDarkTheme = DEFAULT_DARK_THEME;
    _primaryColor = DEFAULT_PRIMARY_COLOR;
    _accentColor = DEFAULT_ACCENT_COLOR;
    _language = DEFAULT_LANGUAGE;
    _rememberPassword = false;
    _autoLogin = false;
    _serverHost = DEFAULT_SERVER_HOST;
    _serverPort = DEFAULT_SERVER_PORT;
}

QString ConfigManager::encryptPassword(const QString &password) const
{
    // 简单的异或加密（生产环境应使用更强的加密算法）
    QByteArray data = password.toUtf8();
    QByteArray key = "QKChatSecret";
    
    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i] ^ key[i % key.size()];
    }
    
    return data.toBase64();
}

QString ConfigManager::decryptPassword(const QString &encrypted) const
{
    QByteArray data = QByteArray::fromBase64(encrypted.toUtf8());
    QByteArray key = "QKChatSecret";
    
    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i] ^ key[i % key.size()];
    }
    
    return QString::fromUtf8(data);
} 