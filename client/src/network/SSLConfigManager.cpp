#include "SSLConfigManager.h"
#include "../utils/LogManager.h"
#include "../config/DevelopmentConfig.h"
#include <QSettings>
#include <QStandardPaths>
#include <QMutexLocker>
#include <QSslSocket>
#include <QCoreApplication>

Q_LOGGING_CATEGORY(sslConfigManager, "qkchat.client.sslconfig")

SSLConfigManager* SSLConfigManager::_instance = nullptr;
QMutex SSLConfigManager::_instanceMutex;

SSLConfigManager* SSLConfigManager::instance()
{
    QMutexLocker locker(&_instanceMutex);
    if (!_instance) {
        _instance = new SSLConfigManager();
    }
    return _instance;
}

SSLConfigManager::SSLConfigManager(QObject *parent)
    : QObject(parent)
    , _environment(Development)
    , _verificationMode(Relaxed)
    , _developmentModeEnabled(true)
{
    initializeDefaults();
    loadConfiguration();

    // 连接开发环境配置变化信号
    connect(DevelopmentConfig::instance(), &DevelopmentConfig::sslConfigurationChanged,
            this, [this]() {
        updateFromDevelopmentConfig();
    });

    // 初始化时应用开发环境配置
    updateFromDevelopmentConfig();

    qCInfo(sslConfigManager) << "SSLConfigManager initialized";
    qCInfo(sslConfigManager) << "Environment:" << _environment;
    qCInfo(sslConfigManager) << "Verification mode:" << _verificationMode;
    qCInfo(sslConfigManager) << "Development mode:" << _developmentModeEnabled;
}

SSLConfigManager::~SSLConfigManager()
{
    saveConfiguration();
}

void SSLConfigManager::initializeDefaults()
{
    // 开发环境下可忽略的SSL错误类型
    _ignorableErrorTypes = {
        QSslError::SelfSignedCertificate,
        QSslError::SelfSignedCertificateInChain,
        QSslError::CertificateUntrusted,
        QSslError::HostNameMismatch,
        QSslError::CertificateExpired,
        QSslError::CertificateNotYetValid,
        QSslError::UnableToGetLocalIssuerCertificate,
        QSslError::UnableToVerifyFirstCertificate
    };
}

void SSLConfigManager::loadConfiguration()
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/ssl_config.ini", QSettings::IniFormat);
    
    // 加载环境设置
    int env = settings.value("SSL/environment", static_cast<int>(Development)).toInt();
    _environment = static_cast<Environment>(env);
    
    // 加载验证模式
    int mode = settings.value("SSL/verification_mode", static_cast<int>(Relaxed)).toInt();
    _verificationMode = static_cast<CertificateVerificationMode>(mode);
    
    // 加载开发模式设置
    _developmentModeEnabled = settings.value("SSL/development_mode", true).toBool();
    
    LogManager::instance()->writeSslLog("CONFIG_LOADED", 
        QString("Environment: %1, Mode: %2, DevMode: %3")
        .arg(_environment).arg(_verificationMode).arg(_developmentModeEnabled));
}

void SSLConfigManager::saveConfiguration()
{
    QSettings settings(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/ssl_config.ini", QSettings::IniFormat);
    
    settings.setValue("SSL/environment", static_cast<int>(_environment));
    settings.setValue("SSL/verification_mode", static_cast<int>(_verificationMode));
    settings.setValue("SSL/development_mode", _developmentModeEnabled);
    
    settings.sync();
}

void SSLConfigManager::setEnvironment(Environment env)
{
    QMutexLocker locker(&_configMutex);
    
    if (_environment != env) {
        _environment = env;
        
        // 根据环境自动调整验证模式
        if (env == Production) {
            _verificationMode = Strict;
            _developmentModeEnabled = false;
        } else {
            _verificationMode = Relaxed;
            _developmentModeEnabled = true;
        }
        
        emit environmentChanged(env);
        LogManager::instance()->writeSslLog("ENVIRONMENT_CHANGED", QString("New environment: %1").arg(env));
    }
}

SSLConfigManager::Environment SSLConfigManager::getEnvironment() const
{
    QMutexLocker locker(&_configMutex);
    return _environment;
}

void SSLConfigManager::setCertificateVerificationMode(CertificateVerificationMode mode)
{
    QMutexLocker locker(&_configMutex);
    
    if (_verificationMode != mode) {
        _verificationMode = mode;
        emit verificationModeChanged(mode);
        LogManager::instance()->writeSslLog("VERIFICATION_MODE_CHANGED", QString("New mode: %1").arg(mode));
    }
}

SSLConfigManager::CertificateVerificationMode SSLConfigManager::getCertificateVerificationMode() const
{
    QMutexLocker locker(&_configMutex);
    return _verificationMode;
}

QSslConfiguration SSLConfigManager::createSslConfiguration() const
{
    QMutexLocker locker(&_configMutex);
    
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    
    // 根据环境和验证模式配置SSL
    switch (_verificationMode) {
    case Strict:
        // 生产环境：严格验证
        config.setPeerVerifyMode(QSslSocket::VerifyPeer);
        config.setPeerVerifyDepth(3);
        break;
        
    case Relaxed:
        // 开发环境：宽松验证
        config.setPeerVerifyMode(QSslSocket::QueryPeer);
        config.setPeerVerifyDepth(1);
        break;
        
    case Disabled:
        // 测试环境：禁用验证
        config.setPeerVerifyMode(QSslSocket::VerifyNone);
        break;
    }
    
    // 添加受信任的证书
    if (!_trustedCertificates.isEmpty()) {
        QList<QSslCertificate> caCerts = config.caCertificates();
        caCerts.append(_trustedCertificates);
        config.setCaCertificates(caCerts);
    }
    
    // 设置协议版本
    config.setProtocol(QSsl::TlsV1_2OrLater);
    
    LogManager::instance()->writeSslLog("CONFIG_CREATED", 
        QString("VerifyMode: %1, Depth: %2, TrustedCerts: %3")
        .arg(config.peerVerifyMode()).arg(config.peerVerifyDepth()).arg(_trustedCertificates.size()));
    
    return config;
}

bool SSLConfigManager::shouldIgnoreSslErrors(const QList<QSslError> &errors) const
{
    QMutexLocker locker(&_configMutex);
    
    // 生产环境下不忽略任何错误
    if (_environment == Production && _verificationMode == Strict) {
        return false;
    }
    
    // 开发环境下检查是否为可忽略的错误
    if (_developmentModeEnabled) {
        for (const QSslError &error : errors) {
            if (!isErrorIgnorable(error)) {
                LogManager::instance()->writeSslLog("ERROR_NOT_IGNORABLE", 
                    QString("Error: %1").arg(error.errorString()));
                return false;
            }
        }
        return true;
    }
    
    return false;
}

QList<QSslError> SSLConfigManager::getIgnorableErrors() const
{
    QMutexLocker locker(&_configMutex);
    
    QList<QSslError> ignorableErrors;
    
    if (_developmentModeEnabled && _verificationMode != Strict) {
        // 创建可忽略的错误列表
        for (QSslError::SslError errorType : _ignorableErrorTypes) {
            ignorableErrors.append(QSslError(errorType));
        }
    }
    
    return ignorableErrors;
}

void SSLConfigManager::addTrustedCertificate(const QSslCertificate &certificate)
{
    QMutexLocker locker(&_configMutex);
    
    if (!_trustedCertificates.contains(certificate)) {
        _trustedCertificates.append(certificate);
        emit trustedCertificateAdded(certificate);
        LogManager::instance()->writeSslLog("TRUSTED_CERT_ADDED", 
            QString("Subject: %1").arg(certificate.subjectInfo(QSslCertificate::CommonName).join(", ")));
    }
}

void SSLConfigManager::removeTrustedCertificate(const QSslCertificate &certificate)
{
    QMutexLocker locker(&_configMutex);
    
    if (_trustedCertificates.removeOne(certificate)) {
        emit trustedCertificateRemoved(certificate);
        LogManager::instance()->writeSslLog("TRUSTED_CERT_REMOVED", 
            QString("Subject: %1").arg(certificate.subjectInfo(QSslCertificate::CommonName).join(", ")));
    }
}

QList<QSslCertificate> SSLConfigManager::getTrustedCertificates() const
{
    QMutexLocker locker(&_configMutex);
    return _trustedCertificates;
}

void SSLConfigManager::clearTrustedCertificates()
{
    QMutexLocker locker(&_configMutex);
    
    _trustedCertificates.clear();
    LogManager::instance()->writeSslLog("TRUSTED_CERTS_CLEARED", "All trusted certificates removed");
}

void SSLConfigManager::enableDevelopmentMode(bool enable)
{
    QMutexLocker locker(&_configMutex);
    
    if (_developmentModeEnabled != enable) {
        _developmentModeEnabled = enable;
        LogManager::instance()->writeSslLog("DEV_MODE_CHANGED", QString("Enabled: %1").arg(enable));
    }
}

bool SSLConfigManager::isDevelopmentModeEnabled() const
{
    QMutexLocker locker(&_configMutex);
    return _developmentModeEnabled;
}

QString SSLConfigManager::analyzeSslError(const QSslError &error) const
{
    QString analysis;
    
    switch (error.error()) {
    case QSslError::SelfSignedCertificate:
        analysis = "服务器使用自签名证书。这在开发环境中是常见的，但在生产环境中可能存在安全风险。";
        break;
    case QSslError::SelfSignedCertificateInChain:
        analysis = "证书链中包含自签名证书。";
        break;
    case QSslError::CertificateUntrusted:
        analysis = "证书不受信任。证书颁发机构(CA)不在系统的受信任列表中。";
        break;
    case QSslError::HostNameMismatch:
        analysis = "主机名不匹配。证书中的主机名与连接的主机名不一致。";
        break;
    case QSslError::CertificateExpired:
        analysis = "证书已过期。需要更新服务器证书。";
        break;
    case QSslError::CertificateNotYetValid:
        analysis = "证书尚未生效。检查系统时间或证书的有效期。";
        break;
    default:
        analysis = QString("SSL错误: %1").arg(error.errorString());
        break;
    }
    
    return analysis;
}

QString SSLConfigManager::getSslErrorSolution(const QSslError &error) const
{
    QString solution;
    
    switch (error.error()) {
    case QSslError::SelfSignedCertificate:
    case QSslError::SelfSignedCertificateInChain:
        solution = "开发环境：可以在SSL配置中忽略此错误。生产环境：使用由受信任CA签发的证书。";
        break;
    case QSslError::CertificateUntrusted:
        solution = "将证书添加到受信任证书列表，或使用由受信任CA签发的证书。";
        break;
    case QSslError::HostNameMismatch:
        solution = "确保证书中的主机名与连接的主机名一致，或在证书中添加正确的SAN(Subject Alternative Name)。";
        break;
    case QSslError::CertificateExpired:
        solution = "更新服务器证书，确保证书在有效期内。";
        break;
    case QSslError::CertificateNotYetValid:
        solution = "检查系统时间是否正确，或等待证书生效时间。";
        break;
    default:
        solution = "请检查SSL配置和证书设置。";
        break;
    }
    
    return solution;
}

bool SSLConfigManager::isErrorIgnorable(const QSslError &error) const
{
    return _ignorableErrorTypes.contains(error.error());
}

bool SSLConfigManager::isSelfSignedCertificateError(const QSslError &error) const
{
    return error.error() == QSslError::SelfSignedCertificate ||
           error.error() == QSslError::SelfSignedCertificateInChain;
}

bool SSLConfigManager::isHostnameVerificationError(const QSslError &error) const
{
    return error.error() == QSslError::HostNameMismatch;
}

void SSLConfigManager::updateFromDevelopmentConfig()
{
    DevelopmentConfig* devConfig = DevelopmentConfig::instance();

    QMutexLocker locker(&_configMutex);

    // 根据开发环境配置更新SSL设置
    bool verificationEnabled = devConfig->isSslVerificationEnabled();

    if (verificationEnabled) {
        _verificationMode = Strict;
        _developmentModeEnabled = false;
    } else {
        _verificationMode = Relaxed;
        _developmentModeEnabled = true;
    }

    // 根据环境类型设置
    switch (devConfig->getEnvironment()) {
    case DevelopmentConfig::Development:
        _environment = Development;
        _developmentModeEnabled = true;
        if (!verificationEnabled) {
            _verificationMode = Relaxed;
        }
        break;

    case DevelopmentConfig::Testing:
        _environment = Development; // 测试环境也使用开发模式
        _developmentModeEnabled = true;
        break;

    case DevelopmentConfig::Staging:
    case DevelopmentConfig::Production:
        _environment = Production;
        _developmentModeEnabled = false;
        _verificationMode = Strict;
        break;
    }

    qCInfo(sslConfigManager) << "SSL configuration updated from DevelopmentConfig";
    qCInfo(sslConfigManager) << "Verification enabled:" << verificationEnabled;
    qCInfo(sslConfigManager) << "Development mode:" << _developmentModeEnabled;
    qCInfo(sslConfigManager) << "Verification mode:" << _verificationMode;

    LogManager::instance()->writeSslLog("CONFIG_UPDATED",
        QString("VerificationEnabled: %1, DevMode: %2, Mode: %3")
        .arg(verificationEnabled).arg(_developmentModeEnabled).arg(_verificationMode));
}
