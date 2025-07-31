#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QColor>

/**
 * @brief 配置管理器类
 * 
 * 负责管理应用程序的配置信息，包括主题、颜色、用户偏好等
 * 支持配置的持久化存储和自动加载
 */
class ConfigManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isDarkTheme READ isDarkTheme WRITE setIsDarkTheme NOTIFY isDarkThemeChanged)
    Q_PROPERTY(QString primaryColor READ primaryColor WRITE setPrimaryColor NOTIFY primaryColorChanged)
    Q_PROPERTY(QString accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(bool rememberPassword READ rememberPassword WRITE setRememberPassword NOTIFY rememberPasswordChanged)
    Q_PROPERTY(bool autoLogin READ autoLogin WRITE setAutoLogin NOTIFY autoLoginChanged)
    
public:
    explicit ConfigManager(QObject *parent = nullptr);
    ~ConfigManager();
    
    // 主题配置
    bool isDarkTheme() const { return _isDarkTheme; }
    void setIsDarkTheme(bool isDark);
    
    // 颜色配置
    QString primaryColor() const { return _primaryColor; }
    void setPrimaryColor(const QString &color);
    
    QString accentColor() const { return _accentColor; }
    void setAccentColor(const QString &color);
    
    // 语言配置
    QString language() const { return _language; }
    void setLanguage(const QString &language);
    
    // 登录配置
    bool rememberPassword() const { return _rememberPassword; }
    void setRememberPassword(bool remember);
    
    bool autoLogin() const { return _autoLogin; }
    void setAutoLogin(bool autoLogin);
    
    // 配置管理方法
    Q_INVOKABLE void loadConfig();
    Q_INVOKABLE void saveConfig();
    Q_INVOKABLE void resetToDefault();
    
    // 服务器配置
    Q_INVOKABLE QString getServerHost() const;
    Q_INVOKABLE int getServerPort() const;
    Q_INVOKABLE void setServerConfig(const QString &host, int port);
    
    // 用户凭据管理
    Q_INVOKABLE void saveUserCredentials(const QString &username, const QString &password);
    Q_INVOKABLE QString loadUsername();
    Q_INVOKABLE QString loadPassword();
    Q_INVOKABLE void clearUserCredentials();
    
signals:
    void isDarkThemeChanged();
    void primaryColorChanged();
    void accentColorChanged();
    void languageChanged();
    void rememberPasswordChanged();
    void autoLoginChanged();
    void configLoaded();
    void configSaved();
    
private:
    void initializeDefaults();
    QString encryptPassword(const QString &password) const;
    QString decryptPassword(const QString &encrypted) const;
    
    QSettings *_settings;
    
    // 主题配置
    bool _isDarkTheme;
    QString _primaryColor;
    QString _accentColor;
    QString _language;
    
    // 登录配置
    bool _rememberPassword;
    bool _autoLogin;
    
    // 服务器配置
    QString _serverHost;
    int _serverPort;
    
    // 默认值
    static const bool DEFAULT_DARK_THEME = false;
    static const QString DEFAULT_PRIMARY_COLOR;
    static const QString DEFAULT_ACCENT_COLOR;
    static const QString DEFAULT_LANGUAGE;
    static const QString DEFAULT_SERVER_HOST;
    static const int DEFAULT_SERVER_PORT = 8443;
};

#endif // CONFIGMANAGER_H 