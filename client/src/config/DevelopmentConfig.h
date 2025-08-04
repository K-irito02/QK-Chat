#ifndef DEVELOPMENTCONFIG_H
#define DEVELOPMENTCONFIG_H

#include <QObject>
#include <QSettings>
#include <QLoggingCategory>
#include <QMutex>

Q_DECLARE_LOGGING_CATEGORY(developmentConfig)

/**
 * @brief 开发环境配置管理器
 * 
 * 管理开发环境特有的配置，包括：
 * - SSL证书验证设置
 * - 调试选项
 * - 开发工具配置
 * - 测试环境参数
 */
class DevelopmentConfig : public QObject
{
    Q_OBJECT
    
public:
    enum Environment {
        Development,    // 开发环境
        Testing,        // 测试环境
        Staging,        // 预发布环境
        Production      // 生产环境
    };
    Q_ENUM(Environment)
    
    enum LogLevel {
        Debug,          // 调试级别
        Info,           // 信息级别
        Warning,        // 警告级别
        Error,          // 错误级别
        Critical        // 严重错误级别
    };
    Q_ENUM(LogLevel)
    
    static DevelopmentConfig* instance();
    
    // 环境配置
    void setEnvironment(Environment env);
    Environment getEnvironment() const;
    QString getEnvironmentString() const;
    bool isDevelopmentMode() const;
    bool isProductionMode() const;
    
    // SSL配置
    void setSslVerificationEnabled(bool enabled);
    bool isSslVerificationEnabled() const;
    void setSslIgnoreSelfSigned(bool ignore);
    bool isSslIgnoreSelfSigned() const;
    void setSslIgnoreHostnameMismatch(bool ignore);
    bool isSslIgnoreHostnameMismatch() const;
    void setSslIgnoreExpiredCerts(bool ignore);
    bool isSslIgnoreExpiredCerts() const;
    
    // 调试配置
    void setDebugMode(bool enabled);
    bool isDebugMode() const;
    void setVerboseLogging(bool enabled);
    bool isVerboseLogging() const;
    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;
    void setLogToFile(bool enabled);
    bool isLogToFile() const;
    void setLogToConsole(bool enabled);
    bool isLogToConsole() const;
    
    // 网络配置
    void setConnectionTimeout(int timeoutMs);
    int getConnectionTimeout() const;
    void setHeartbeatInterval(int intervalMs);
    int getHeartbeatInterval() const;
    void setMaxRetryAttempts(int maxAttempts);
    int getMaxRetryAttempts() const;
    void setRetryInterval(int intervalMs);
    int getRetryInterval() const;
    
    // 开发工具配置
    void setMockServerEnabled(bool enabled);
    bool isMockServerEnabled() const;
    void setAutoReconnectEnabled(bool enabled);
    bool isAutoReconnectEnabled() const;
    void setPerformanceMonitoringEnabled(bool enabled);
    bool isPerformanceMonitoringEnabled() const;
    void setMemoryLeakDetectionEnabled(bool enabled);
    bool isMemoryLeakDetectionEnabled() const;
    
    // 测试配置
    void setTestDataEnabled(bool enabled);
    bool isTestDataEnabled() const;
    void setSimulateNetworkErrors(bool enabled);
    bool isSimulateNetworkErrors() const;
    void setSimulateSlowNetwork(bool enabled);
    bool isSimulateSlowNetwork() const;
    
    // 配置管理
    void loadFromFile(const QString &filePath = "");
    void saveToFile(const QString &filePath = "");
    void loadFromEnvironment();
    void resetToDefaults();
    
    // 配置验证
    bool validateConfiguration() const;
    QStringList getConfigurationWarnings() const;
    QStringList getConfigurationErrors() const;
    
signals:
    void environmentChanged(Environment env);
    void configurationChanged();
    void sslConfigurationChanged();
    void debugConfigurationChanged();
    
private:
    explicit DevelopmentConfig(QObject *parent = nullptr);
    ~DevelopmentConfig();
    
    void initializeDefaults();
    void applyEnvironmentDefaults();
    QString getDefaultConfigPath() const;
    
    static DevelopmentConfig* _instance;
    static QMutex _instanceMutex;
    
    QSettings *_settings;
    Environment _environment;
    
    // SSL配置
    bool _sslVerificationEnabled;
    bool _sslIgnoreSelfSigned;
    bool _sslIgnoreHostnameMismatch;
    bool _sslIgnoreExpiredCerts;
    
    // 调试配置
    bool _debugMode;
    bool _verboseLogging;
    LogLevel _logLevel;
    bool _logToFile;
    bool _logToConsole;
    
    // 网络配置
    int _connectionTimeout;
    int _heartbeatInterval;
    int _maxRetryAttempts;
    int _retryInterval;
    
    // 开发工具配置
    bool _mockServerEnabled;
    bool _autoReconnectEnabled;
    bool _performanceMonitoringEnabled;
    bool _memoryLeakDetectionEnabled;
    
    // 测试配置
    bool _testDataEnabled;
    bool _simulateNetworkErrors;
    bool _simulateSlowNetwork;
    
    mutable QMutex _configMutex;
};

#endif // DEVELOPMENTCONFIG_H
