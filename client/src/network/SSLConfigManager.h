#ifndef SSLCONFIGMANAGER_H
#define SSLCONFIGMANAGER_H

#include <QObject>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslError>
#include <QLoggingCategory>
#include <QMutex>

Q_DECLARE_LOGGING_CATEGORY(sslConfigManager)

/**
 * @brief SSL配置管理器
 * 
 * 负责管理SSL连接的配置，包括：
 * - 开发环境和生产环境的不同配置
 * - 证书验证策略
 * - SSL错误处理
 */
class SSLConfigManager : public QObject
{
    Q_OBJECT
    
public:
    enum Environment {
        Development,    // 开发环境
        Production     // 生产环境
    };
    Q_ENUM(Environment)
    
    enum CertificateVerificationMode {
        Strict,        // 严格验证
        Relaxed,       // 宽松验证（开发环境）
        Disabled       // 禁用验证（仅测试）
    };
    Q_ENUM(CertificateVerificationMode)
    
    static SSLConfigManager* instance();
    
    // 配置管理
    void setEnvironment(Environment env);
    Environment getEnvironment() const;
    
    void setCertificateVerificationMode(CertificateVerificationMode mode);
    CertificateVerificationMode getCertificateVerificationMode() const;
    
    // SSL配置
    QSslConfiguration createSslConfiguration() const;
    bool shouldIgnoreSslErrors(const QList<QSslError> &errors) const;
    QList<QSslError> getIgnorableErrors() const;
    
    // 证书管理
    void addTrustedCertificate(const QSslCertificate &certificate);
    void removeTrustedCertificate(const QSslCertificate &certificate);
    QList<QSslCertificate> getTrustedCertificates() const;
    void clearTrustedCertificates();
    
    // 开发环境特殊处理
    void enableDevelopmentMode(bool enable = true);
    bool isDevelopmentModeEnabled() const;
    
    // 错误分析
    QString analyzeSslError(const QSslError &error) const;
    QString getSslErrorSolution(const QSslError &error) const;
    
signals:
    void environmentChanged(Environment env);
    void verificationModeChanged(CertificateVerificationMode mode);
    void trustedCertificateAdded(const QSslCertificate &certificate);
    void trustedCertificateRemoved(const QSslCertificate &certificate);
    
private:
    explicit SSLConfigManager(QObject *parent = nullptr);
    ~SSLConfigManager();
    
    void initializeDefaults();
    void loadConfiguration();
    void saveConfiguration();
    void updateFromDevelopmentConfig();
    
    bool isErrorIgnorable(const QSslError &error) const;
    bool isSelfSignedCertificateError(const QSslError &error) const;
    bool isHostnameVerificationError(const QSslError &error) const;
    
    static SSLConfigManager* _instance;
    static QMutex _instanceMutex;
    
    Environment _environment;
    CertificateVerificationMode _verificationMode;
    bool _developmentModeEnabled;
    
    QList<QSslCertificate> _trustedCertificates;
    QList<QSslError::SslError> _ignorableErrorTypes;
    
    mutable QMutex _configMutex;
};

#endif // SSLCONFIGMANAGER_H
