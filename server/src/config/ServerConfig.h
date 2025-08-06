#ifndef SERVERCONFIG_H
#define SERVERCONFIG_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QHash>

/**
 * @brief 服务器配置管理类（单例模式）
 * 
 * 负责管理服务器配置，包括：
 * - 网络配置（端口、SSL等）
 * - 数据库配置
 * - 安全配置
 * - 日志配置
 */
class ServerConfig : public QObject
{
    Q_OBJECT
    
public:
    static ServerConfig* instance();
    
    // 配置加载和保存
    bool loadConfig(const QString &configFile = "");
    bool saveConfig();
    void reloadConfig();
    
    // 配置访问
    QVariant getValue(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    
    // 网络配置
    QString getServerHost() const;
    int getServerPort() const;
    int getAdminPort() const;
    int getFileTransferPort() const;
    int getMaxConnections() const;
    int getThreadPoolSize() const;
    
    // SSL配置
    bool isSslEnabled() const;
    QString getCertificateFile() const;
    QString getPrivateKeyFile() const;
    QString getCaFile() const;
    QString getSslCertificateFile() const;
    QString getSslPrivateKeyFile() const;
    QString getSslPrivateKeyPassword() const;
    
    // 数据库配置
    QString getDatabaseType() const;
    QString getDatabaseHost() const;
    int getDatabasePort() const;
    QString getDatabaseName() const;
    QString getDatabaseUsername() const;
    QString getDatabasePassword() const;
    int getDatabasePoolSize() const;
    
    // Redis配置
    QString getRedisHost() const;
    int getRedisPort() const;
    QString getRedisPassword() const;
    int getRedisDatabase() const;
    
    // 安全配置
    QString getAdminUsername() const;
    QString getAdminPassword() const;
    int getSessionTimeout() const;
    int getMaxLoginAttempts() const;
    int getLockoutDuration() const;
    
    // SMTP配置
    QString getSmtpHost() const;
    int getSmtpPort() const;
    QString getSmtpUsername() const;
    QString getSmtpPassword() const;
    QString getFromEmail() const;
    QString getFromName() const;
    
    // 日志配置
    QString getLogLevel() const;
    QString getLogFile() const;
    int getMaxLogFileSize() const;
    int getMaxLogFiles() const;
    
signals:
    void configChanged(const QString &key, const QVariant &value);
    void configLoaded();
    void configSaved();
    
private:
    explicit ServerConfig(QObject *parent = nullptr);
    ~ServerConfig();
    
    void initializeDefaults();
    QString getDefaultConfigPath() const;
    
    // 私有辅助方法
    QString getRawCertificateFile() const;
    QString getRawPrivateKeyFile() const;
    
    static ServerConfig* _instance;
    QSettings* _settings;
    QString _configFile;
    
    // 配置缓存
    mutable QHash<QString, QVariant> _configCache;
    
    Q_DISABLE_COPY(ServerConfig)
};

#endif // SERVERCONFIG_H