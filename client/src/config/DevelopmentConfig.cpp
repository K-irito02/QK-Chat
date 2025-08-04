#include "DevelopmentConfig.h"
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#include <QMutexLocker>
#include <QProcessEnvironment>

Q_LOGGING_CATEGORY(developmentConfig, "qkchat.client.devconfig")

DevelopmentConfig* DevelopmentConfig::_instance = nullptr;
QMutex DevelopmentConfig::_instanceMutex;

DevelopmentConfig* DevelopmentConfig::instance()
{
    QMutexLocker locker(&_instanceMutex);
    if (!_instance) {
        _instance = new DevelopmentConfig();
    }
    return _instance;
}

DevelopmentConfig::DevelopmentConfig(QObject *parent)
    : QObject(parent)
    , _settings(nullptr)
    , _environment(Development)
{
    initializeDefaults();
    
    QString configPath = getDefaultConfigPath();
    _settings = new QSettings(configPath, QSettings::IniFormat, this);
    
    loadFromFile();
    loadFromEnvironment();
    
    qCInfo(developmentConfig) << "DevelopmentConfig initialized";
    qCInfo(developmentConfig) << "Environment:" << getEnvironmentString();
    qCInfo(developmentConfig) << "Config file:" << configPath;
}

DevelopmentConfig::~DevelopmentConfig()
{
    if (_settings) {
        _settings->sync();
    }
}

void DevelopmentConfig::initializeDefaults()
{
    // 默认为开发环境
    _environment = Development;
    
    // SSL配置默认值（开发环境宽松）
    _sslVerificationEnabled = false;
    _sslIgnoreSelfSigned = true;
    _sslIgnoreHostnameMismatch = true;
    _sslIgnoreExpiredCerts = true;
    
    // 调试配置默认值
    _debugMode = true;
    _verboseLogging = true;
    _logLevel = Debug;
    _logToFile = true;
    _logToConsole = true;
    
    // 网络配置默认值
    _connectionTimeout = 30000;
    _heartbeatInterval = 30000;
    _maxRetryAttempts = 10;
    _retryInterval = 2000;
    
    // 开发工具配置默认值
    _mockServerEnabled = false;
    _autoReconnectEnabled = true;
    _performanceMonitoringEnabled = true;
    _memoryLeakDetectionEnabled = false;
    
    // 测试配置默认值
    _testDataEnabled = false;
    _simulateNetworkErrors = false;
    _simulateSlowNetwork = false;
}

void DevelopmentConfig::setEnvironment(Environment env)
{
    QMutexLocker locker(&_configMutex);
    
    if (_environment != env) {
        _environment = env;
        applyEnvironmentDefaults();
        
        if (_settings) {
            _settings->setValue("Environment/type", static_cast<int>(env));
        }
        
        emit environmentChanged(env);
        emit configurationChanged();
        
        qCInfo(developmentConfig) << "Environment changed to:" << getEnvironmentString();
    }
}

DevelopmentConfig::Environment DevelopmentConfig::getEnvironment() const
{
    QMutexLocker locker(&_configMutex);
    return _environment;
}

QString DevelopmentConfig::getEnvironmentString() const
{
    switch (_environment) {
    case Development:
        return "Development";
    case Testing:
        return "Testing";
    case Staging:
        return "Staging";
    case Production:
        return "Production";
    default:
        return "Unknown";
    }
}

bool DevelopmentConfig::isDevelopmentMode() const
{
    return _environment == Development;
}

bool DevelopmentConfig::isProductionMode() const
{
    return _environment == Production;
}

void DevelopmentConfig::setSslVerificationEnabled(bool enabled)
{
    QMutexLocker locker(&_configMutex);
    
    if (_sslVerificationEnabled != enabled) {
        _sslVerificationEnabled = enabled;
        
        if (_settings) {
            _settings->setValue("SSL/verification_enabled", enabled);
        }
        
        emit sslConfigurationChanged();
        emit configurationChanged();
    }
}

bool DevelopmentConfig::isSslVerificationEnabled() const
{
    QMutexLocker locker(&_configMutex);
    return _sslVerificationEnabled;
}

void DevelopmentConfig::setSslIgnoreSelfSigned(bool ignore)
{
    QMutexLocker locker(&_configMutex);
    
    if (_sslIgnoreSelfSigned != ignore) {
        _sslIgnoreSelfSigned = ignore;
        
        if (_settings) {
            _settings->setValue("SSL/ignore_self_signed", ignore);
        }
        
        emit sslConfigurationChanged();
        emit configurationChanged();
    }
}

bool DevelopmentConfig::isSslIgnoreSelfSigned() const
{
    QMutexLocker locker(&_configMutex);
    return _sslIgnoreSelfSigned;
}

void DevelopmentConfig::setSslIgnoreHostnameMismatch(bool ignore)
{
    QMutexLocker locker(&_configMutex);
    
    if (_sslIgnoreHostnameMismatch != ignore) {
        _sslIgnoreHostnameMismatch = ignore;
        
        if (_settings) {
            _settings->setValue("SSL/ignore_hostname_mismatch", ignore);
        }
        
        emit sslConfigurationChanged();
        emit configurationChanged();
    }
}

bool DevelopmentConfig::isSslIgnoreHostnameMismatch() const
{
    QMutexLocker locker(&_configMutex);
    return _sslIgnoreHostnameMismatch;
}

void DevelopmentConfig::setSslIgnoreExpiredCerts(bool ignore)
{
    QMutexLocker locker(&_configMutex);
    
    if (_sslIgnoreExpiredCerts != ignore) {
        _sslIgnoreExpiredCerts = ignore;
        
        if (_settings) {
            _settings->setValue("SSL/ignore_expired_certs", ignore);
        }
        
        emit sslConfigurationChanged();
        emit configurationChanged();
    }
}

bool DevelopmentConfig::isSslIgnoreExpiredCerts() const
{
    QMutexLocker locker(&_configMutex);
    return _sslIgnoreExpiredCerts;
}

void DevelopmentConfig::setDebugMode(bool enabled)
{
    QMutexLocker locker(&_configMutex);
    
    if (_debugMode != enabled) {
        _debugMode = enabled;
        
        if (_settings) {
            _settings->setValue("Debug/enabled", enabled);
        }
        
        emit debugConfigurationChanged();
        emit configurationChanged();
    }
}

bool DevelopmentConfig::isDebugMode() const
{
    QMutexLocker locker(&_configMutex);
    return _debugMode;
}

void DevelopmentConfig::setVerboseLogging(bool enabled)
{
    QMutexLocker locker(&_configMutex);
    
    if (_verboseLogging != enabled) {
        _verboseLogging = enabled;
        
        if (_settings) {
            _settings->setValue("Debug/verbose_logging", enabled);
        }
        
        emit debugConfigurationChanged();
        emit configurationChanged();
    }
}

bool DevelopmentConfig::isVerboseLogging() const
{
    QMutexLocker locker(&_configMutex);
    return _verboseLogging;
}

void DevelopmentConfig::setLogLevel(LogLevel level)
{
    QMutexLocker locker(&_configMutex);
    
    if (_logLevel != level) {
        _logLevel = level;
        
        if (_settings) {
            _settings->setValue("Debug/log_level", static_cast<int>(level));
        }
        
        emit debugConfigurationChanged();
        emit configurationChanged();
    }
}

DevelopmentConfig::LogLevel DevelopmentConfig::getLogLevel() const
{
    QMutexLocker locker(&_configMutex);
    return _logLevel;
}

void DevelopmentConfig::setLogToFile(bool enabled)
{
    QMutexLocker locker(&_configMutex);
    
    if (_logToFile != enabled) {
        _logToFile = enabled;
        
        if (_settings) {
            _settings->setValue("Debug/log_to_file", enabled);
        }
        
        emit debugConfigurationChanged();
        emit configurationChanged();
    }
}

bool DevelopmentConfig::isLogToFile() const
{
    QMutexLocker locker(&_configMutex);
    return _logToFile;
}

void DevelopmentConfig::setLogToConsole(bool enabled)
{
    QMutexLocker locker(&_configMutex);
    
    if (_logToConsole != enabled) {
        _logToConsole = enabled;
        
        if (_settings) {
            _settings->setValue("Debug/log_to_console", enabled);
        }
        
        emit debugConfigurationChanged();
        emit configurationChanged();
    }
}

bool DevelopmentConfig::isLogToConsole() const
{
    QMutexLocker locker(&_configMutex);
    return _logToConsole;
}

void DevelopmentConfig::setConnectionTimeout(int timeoutMs)
{
    QMutexLocker locker(&_configMutex);
    
    if (_connectionTimeout != timeoutMs) {
        _connectionTimeout = timeoutMs;
        
        if (_settings) {
            _settings->setValue("Network/connection_timeout", timeoutMs);
        }
        
        emit configurationChanged();
    }
}

int DevelopmentConfig::getConnectionTimeout() const
{
    QMutexLocker locker(&_configMutex);
    return _connectionTimeout;
}

void DevelopmentConfig::setHeartbeatInterval(int intervalMs)
{
    QMutexLocker locker(&_configMutex);
    
    if (_heartbeatInterval != intervalMs) {
        _heartbeatInterval = intervalMs;
        
        if (_settings) {
            _settings->setValue("Network/heartbeat_interval", intervalMs);
        }
        
        emit configurationChanged();
    }
}

int DevelopmentConfig::getHeartbeatInterval() const
{
    QMutexLocker locker(&_configMutex);
    return _heartbeatInterval;
}

void DevelopmentConfig::setMaxRetryAttempts(int maxAttempts)
{
    QMutexLocker locker(&_configMutex);
    
    if (_maxRetryAttempts != maxAttempts) {
        _maxRetryAttempts = maxAttempts;
        
        if (_settings) {
            _settings->setValue("Network/max_retry_attempts", maxAttempts);
        }
        
        emit configurationChanged();
    }
}

int DevelopmentConfig::getMaxRetryAttempts() const
{
    QMutexLocker locker(&_configMutex);
    return _maxRetryAttempts;
}

void DevelopmentConfig::setRetryInterval(int intervalMs)
{
    QMutexLocker locker(&_configMutex);
    
    if (_retryInterval != intervalMs) {
        _retryInterval = intervalMs;
        
        if (_settings) {
            _settings->setValue("Network/retry_interval", intervalMs);
        }
        
        emit configurationChanged();
    }
}

int DevelopmentConfig::getRetryInterval() const
{
    QMutexLocker locker(&_configMutex);
    return _retryInterval;
}

void DevelopmentConfig::setMockServerEnabled(bool enabled)
{
    QMutexLocker locker(&_configMutex);

    if (_mockServerEnabled != enabled) {
        _mockServerEnabled = enabled;

        if (_settings) {
            _settings->setValue("Development/mock_server_enabled", enabled);
        }

        emit configurationChanged();
    }
}

bool DevelopmentConfig::isMockServerEnabled() const
{
    QMutexLocker locker(&_configMutex);
    return _mockServerEnabled;
}

void DevelopmentConfig::setAutoReconnectEnabled(bool enabled)
{
    QMutexLocker locker(&_configMutex);

    if (_autoReconnectEnabled != enabled) {
        _autoReconnectEnabled = enabled;

        if (_settings) {
            _settings->setValue("Development/auto_reconnect_enabled", enabled);
        }

        emit configurationChanged();
    }
}

bool DevelopmentConfig::isAutoReconnectEnabled() const
{
    QMutexLocker locker(&_configMutex);
    return _autoReconnectEnabled;
}

void DevelopmentConfig::setPerformanceMonitoringEnabled(bool enabled)
{
    QMutexLocker locker(&_configMutex);

    if (_performanceMonitoringEnabled != enabled) {
        _performanceMonitoringEnabled = enabled;

        if (_settings) {
            _settings->setValue("Development/performance_monitoring_enabled", enabled);
        }

        emit configurationChanged();
    }
}

bool DevelopmentConfig::isPerformanceMonitoringEnabled() const
{
    QMutexLocker locker(&_configMutex);
    return _performanceMonitoringEnabled;
}

void DevelopmentConfig::setMemoryLeakDetectionEnabled(bool enabled)
{
    QMutexLocker locker(&_configMutex);

    if (_memoryLeakDetectionEnabled != enabled) {
        _memoryLeakDetectionEnabled = enabled;

        if (_settings) {
            _settings->setValue("Development/memory_leak_detection_enabled", enabled);
        }

        emit configurationChanged();
    }
}

bool DevelopmentConfig::isMemoryLeakDetectionEnabled() const
{
    QMutexLocker locker(&_configMutex);
    return _memoryLeakDetectionEnabled;
}

void DevelopmentConfig::setTestDataEnabled(bool enabled)
{
    QMutexLocker locker(&_configMutex);

    if (_testDataEnabled != enabled) {
        _testDataEnabled = enabled;

        if (_settings) {
            _settings->setValue("Testing/test_data_enabled", enabled);
        }

        emit configurationChanged();
    }
}

bool DevelopmentConfig::isTestDataEnabled() const
{
    QMutexLocker locker(&_configMutex);
    return _testDataEnabled;
}

void DevelopmentConfig::setSimulateNetworkErrors(bool enabled)
{
    QMutexLocker locker(&_configMutex);

    if (_simulateNetworkErrors != enabled) {
        _simulateNetworkErrors = enabled;

        if (_settings) {
            _settings->setValue("Testing/simulate_network_errors", enabled);
        }

        emit configurationChanged();
    }
}

bool DevelopmentConfig::isSimulateNetworkErrors() const
{
    QMutexLocker locker(&_configMutex);
    return _simulateNetworkErrors;
}

void DevelopmentConfig::setSimulateSlowNetwork(bool enabled)
{
    QMutexLocker locker(&_configMutex);

    if (_simulateSlowNetwork != enabled) {
        _simulateSlowNetwork = enabled;

        if (_settings) {
            _settings->setValue("Testing/simulate_slow_network", enabled);
        }

        emit configurationChanged();
    }
}

bool DevelopmentConfig::isSimulateSlowNetwork() const
{
    QMutexLocker locker(&_configMutex);
    return _simulateSlowNetwork;
}

void DevelopmentConfig::loadFromFile(const QString &filePath)
{
    QString configPath = filePath.isEmpty() ? getDefaultConfigPath() : filePath;

    if (!_settings) {
        _settings = new QSettings(configPath, QSettings::IniFormat, this);
    }

    QMutexLocker locker(&_configMutex);

    // 加载环境设置
    int env = _settings->value("Environment/type", static_cast<int>(Development)).toInt();
    _environment = static_cast<Environment>(env);

    // 加载SSL配置
    _sslVerificationEnabled = _settings->value("SSL/verification_enabled", false).toBool();
    _sslIgnoreSelfSigned = _settings->value("SSL/ignore_self_signed", true).toBool();
    _sslIgnoreHostnameMismatch = _settings->value("SSL/ignore_hostname_mismatch", true).toBool();
    _sslIgnoreExpiredCerts = _settings->value("SSL/ignore_expired_certs", true).toBool();

    // 加载调试配置
    _debugMode = _settings->value("Debug/enabled", true).toBool();
    _verboseLogging = _settings->value("Debug/verbose_logging", true).toBool();
    int logLevel = _settings->value("Debug/log_level", static_cast<int>(Debug)).toInt();
    _logLevel = static_cast<LogLevel>(logLevel);
    _logToFile = _settings->value("Debug/log_to_file", true).toBool();
    _logToConsole = _settings->value("Debug/log_to_console", true).toBool();

    // 加载网络配置
    _connectionTimeout = _settings->value("Network/connection_timeout", 30000).toInt();
    _heartbeatInterval = _settings->value("Network/heartbeat_interval", 30000).toInt();
    _maxRetryAttempts = _settings->value("Network/max_retry_attempts", 10).toInt();
    _retryInterval = _settings->value("Network/retry_interval", 2000).toInt();

    // 加载开发工具配置
    _mockServerEnabled = _settings->value("Development/mock_server_enabled", false).toBool();
    _autoReconnectEnabled = _settings->value("Development/auto_reconnect_enabled", true).toBool();
    _performanceMonitoringEnabled = _settings->value("Development/performance_monitoring_enabled", true).toBool();
    _memoryLeakDetectionEnabled = _settings->value("Development/memory_leak_detection_enabled", false).toBool();

    // 加载测试配置
    _testDataEnabled = _settings->value("Testing/test_data_enabled", false).toBool();
    _simulateNetworkErrors = _settings->value("Testing/simulate_network_errors", false).toBool();
    _simulateSlowNetwork = _settings->value("Testing/simulate_slow_network", false).toBool();

    qCInfo(developmentConfig) << "Configuration loaded from:" << configPath;
}

void DevelopmentConfig::saveToFile(const QString &filePath)
{
    QString configPath = filePath.isEmpty() ? getDefaultConfigPath() : filePath;

    if (!_settings) {
        _settings = new QSettings(configPath, QSettings::IniFormat, this);
    }

    QMutexLocker locker(&_configMutex);

    // 保存环境设置
    _settings->setValue("Environment/type", static_cast<int>(_environment));

    // 保存SSL配置
    _settings->setValue("SSL/verification_enabled", _sslVerificationEnabled);
    _settings->setValue("SSL/ignore_self_signed", _sslIgnoreSelfSigned);
    _settings->setValue("SSL/ignore_hostname_mismatch", _sslIgnoreHostnameMismatch);
    _settings->setValue("SSL/ignore_expired_certs", _sslIgnoreExpiredCerts);

    // 保存调试配置
    _settings->setValue("Debug/enabled", _debugMode);
    _settings->setValue("Debug/verbose_logging", _verboseLogging);
    _settings->setValue("Debug/log_level", static_cast<int>(_logLevel));
    _settings->setValue("Debug/log_to_file", _logToFile);
    _settings->setValue("Debug/log_to_console", _logToConsole);

    // 保存网络配置
    _settings->setValue("Network/connection_timeout", _connectionTimeout);
    _settings->setValue("Network/heartbeat_interval", _heartbeatInterval);
    _settings->setValue("Network/max_retry_attempts", _maxRetryAttempts);
    _settings->setValue("Network/retry_interval", _retryInterval);

    // 保存开发工具配置
    _settings->setValue("Development/mock_server_enabled", _mockServerEnabled);
    _settings->setValue("Development/auto_reconnect_enabled", _autoReconnectEnabled);
    _settings->setValue("Development/performance_monitoring_enabled", _performanceMonitoringEnabled);
    _settings->setValue("Development/memory_leak_detection_enabled", _memoryLeakDetectionEnabled);

    // 保存测试配置
    _settings->setValue("Testing/test_data_enabled", _testDataEnabled);
    _settings->setValue("Testing/simulate_network_errors", _simulateNetworkErrors);
    _settings->setValue("Testing/simulate_slow_network", _simulateSlowNetwork);

    _settings->sync();

    qCInfo(developmentConfig) << "Configuration saved to:" << configPath;
}

void DevelopmentConfig::loadFromEnvironment()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // 从环境变量加载配置
    if (env.contains("QKCHAT_ENVIRONMENT")) {
        QString envStr = env.value("QKCHAT_ENVIRONMENT").toUpper();
        if (envStr == "DEVELOPMENT") {
            setEnvironment(Development);
        } else if (envStr == "TESTING") {
            setEnvironment(Testing);
        } else if (envStr == "STAGING") {
            setEnvironment(Staging);
        } else if (envStr == "PRODUCTION") {
            setEnvironment(Production);
        }
    }

    if (env.contains("QKCHAT_SSL_VERIFICATION")) {
        bool enabled = env.value("QKCHAT_SSL_VERIFICATION").toLower() == "true";
        setSslVerificationEnabled(enabled);
    }

    if (env.contains("QKCHAT_DEBUG_MODE")) {
        bool enabled = env.value("QKCHAT_DEBUG_MODE").toLower() == "true";
        setDebugMode(enabled);
    }

    if (env.contains("QKCHAT_VERBOSE_LOGGING")) {
        bool enabled = env.value("QKCHAT_VERBOSE_LOGGING").toLower() == "true";
        setVerboseLogging(enabled);
    }

    qCInfo(developmentConfig) << "Environment variables loaded";
}

void DevelopmentConfig::resetToDefaults()
{
    QMutexLocker locker(&_configMutex);

    initializeDefaults();

    if (_settings) {
        _settings->clear();
        saveToFile();
    }

    emit configurationChanged();

    qCInfo(developmentConfig) << "Configuration reset to defaults";
}

void DevelopmentConfig::applyEnvironmentDefaults()
{
    switch (_environment) {
    case Development:
        _sslVerificationEnabled = false;
        _sslIgnoreSelfSigned = true;
        _sslIgnoreHostnameMismatch = true;
        _sslIgnoreExpiredCerts = true;
        _debugMode = true;
        _verboseLogging = true;
        _logLevel = Debug;
        _autoReconnectEnabled = true;
        _performanceMonitoringEnabled = true;
        break;

    case Testing:
        _sslVerificationEnabled = false;
        _sslIgnoreSelfSigned = true;
        _sslIgnoreHostnameMismatch = true;
        _sslIgnoreExpiredCerts = false;
        _debugMode = true;
        _verboseLogging = false;
        _logLevel = Info;
        _testDataEnabled = true;
        break;

    case Staging:
        _sslVerificationEnabled = true;
        _sslIgnoreSelfSigned = false;
        _sslIgnoreHostnameMismatch = false;
        _sslIgnoreExpiredCerts = false;
        _debugMode = false;
        _verboseLogging = false;
        _logLevel = Warning;
        _performanceMonitoringEnabled = false;
        break;

    case Production:
        _sslVerificationEnabled = true;
        _sslIgnoreSelfSigned = false;
        _sslIgnoreHostnameMismatch = false;
        _sslIgnoreExpiredCerts = false;
        _debugMode = false;
        _verboseLogging = false;
        _logLevel = Error;
        _autoReconnectEnabled = true;
        _performanceMonitoringEnabled = false;
        _testDataEnabled = false;
        _simulateNetworkErrors = false;
        _simulateSlowNetwork = false;
        break;
    }
}

QString DevelopmentConfig::getDefaultConfigPath() const
{
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return configDir + "/development.ini";
}

bool DevelopmentConfig::validateConfiguration() const
{
    // 基本验证
    if (_connectionTimeout <= 0 || _heartbeatInterval <= 0) {
        return false;
    }

    if (_maxRetryAttempts < 0 || _retryInterval <= 0) {
        return false;
    }

    return true;
}

QStringList DevelopmentConfig::getConfigurationWarnings() const
{
    QStringList warnings;

    if (_environment == Production && _debugMode) {
        warnings << "生产环境启用了调试模式";
    }

    if (_environment == Production && !_sslVerificationEnabled) {
        warnings << "生产环境禁用了SSL验证";
    }

    if (_heartbeatInterval < 10000) {
        warnings << "心跳间隔过短可能影响性能";
    }

    if (_connectionTimeout < 5000) {
        warnings << "连接超时时间过短";
    }

    return warnings;
}

QStringList DevelopmentConfig::getConfigurationErrors() const
{
    QStringList errors;

    if (_connectionTimeout <= 0) {
        errors << "连接超时时间必须大于0";
    }

    if (_heartbeatInterval <= 0) {
        errors << "心跳间隔必须大于0";
    }

    if (_maxRetryAttempts < 0) {
        errors << "最大重试次数不能为负数";
    }

    if (_retryInterval <= 0) {
        errors << "重试间隔必须大于0";
    }

    return errors;
}
