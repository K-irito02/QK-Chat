#ifndef EMAILVERIFICATIONSERVICE_H
#define EMAILVERIFICATIONSERVICE_H

#include <QObject>
#include <QString>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutex>
#include <QMap>
#include "RedisService.h"
#include "SimpleSmtpClient.h"

/**
 * @brief 邮箱验证服务类
 * 
 * 负责邮箱验证码的生成、发送和验证
 * 使用Redis存储验证码，支持过期时间
 */
class EmailVerificationService : public QObject
{
    Q_OBJECT
    
public:
    explicit EmailVerificationService(QObject *parent = nullptr);
    ~EmailVerificationService();
    
    // 验证码管理
    bool sendVerificationCode(const QString &email);
    bool sendVerificationCodeWithRetry(const QString &email, int maxRetries = 3);
    bool verifyCode(const QString &email, const QString &code);
    bool isCodeExpired(const QString &email);
    void clearExpiredCodes();
    
    // Redis操作
    bool initializeRedis();
    bool isRedisConnected() const;
    void closeRedis();
    
    // 邮件发送配置
    void setSmtpConfig(const QString &host, int port, const QString &username, const QString &password);
    void setFromEmail(const QString &fromEmail);
    void setFromName(const QString &fromName);
    
signals:
    void verificationCodeSent(const QString &email, bool success, const QString &message);
    void verificationCodeVerified(const QString &email, bool success, const QString &message);
    void redisError(const QString &error);
    void emailError(const QString &error);
    
private slots:
    void onEmailSent(const QString &email, bool success, const QString &message);
    void onEmailError(const QString &error);
    
private:
    // 验证码生成
    QString generateVerificationCode();
    QString generateEmailContent(const QString &code);
    
    // Redis操作
    bool setCodeToRedis(const QString &email, const QString &code, int expirationSeconds = 300);
    QString getCodeFromRedis(const QString &email);
    bool deleteCodeFromRedis(const QString &email);
    
    // 本地缓存操作（Redis备选方案）
    bool setCodeToLocalCache(const QString &email, const QString &code, int expirationSeconds = 300);
    QString getCodeFromLocalCache(const QString &email);
    bool deleteCodeFromLocalCache(const QString &email);
    void clearExpiredLocalCache();
    
    // 邮件发送
    void sendEmailViaHttp(const QString &toEmail, const QString &subject, const QString &content);
    
    // 配置
    QString _smtpHost;
    int _smtpPort;
    QString _smtpUsername;
    QString _smtpPassword;
    QString _fromEmail;
    QString _fromName;
    
    // Redis服务
    RedisService* _redisService;
    
    // 网络管理器
    QNetworkAccessManager *_networkManager;
    
    // SMTP客户端
    SimpleSmtpClient *_smtpClient;
    
    // 互斥锁
    QMutex _mutex;
    
    // 定时器
    QTimer *_cleanupTimer;
    
    // 本地缓存（Redis备选方案）
    QMap<QString, QPair<QString, QDateTime>> _localCache; // email -> (code, expiration)
    QMutex _cacheMutex;
    
    // 验证码过期时间（秒）
    static const int CODE_EXPIRATION_SECONDS = 300; // 5分钟
};

#endif // EMAILVERIFICATIONSERVICE_H