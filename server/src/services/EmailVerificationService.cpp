#include "EmailVerificationService.h"
#include "../config/ServerConfig.h"
#include "../utils/LogManager.h"
#include "RedisService.h"
#include "SimpleSmtpClient.h"
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QDateTime>
#include <QRandomGenerator>
#include <QLoggingCategory>
#include <QThread>
#include <QElapsedTimer>
#include <QMutexLocker>

Q_LOGGING_CATEGORY(emailService, "qkchat.server.email")

EmailVerificationService::EmailVerificationService(QObject *parent)
    : QObject(parent)
    , _smtpPort(587)
    , _redisService(nullptr)
    , _networkManager(nullptr)
    , _cleanupTimer(nullptr)
    , _smtpClient(nullptr)
{
    _networkManager = new QNetworkAccessManager(this);
    _cleanupTimer = new QTimer(this);
    
    // 从配置加载设置
    ServerConfig *config = ServerConfig::instance();
    if (config) {
        _smtpHost = config->getSmtpHost();
        _smtpPort = config->getSmtpPort();
        _smtpUsername = config->getSmtpUsername();
        _smtpPassword = config->getSmtpPassword();
        _fromEmail = config->getFromEmail();
        _fromName = config->getFromName();
    } else {
        qCWarning(emailService) << "ServerConfig instance is null, using default values";
        // 使用默认值
        _smtpHost = "smtp.qq.com";
        _smtpPort = 587;
        _smtpUsername = "saokiritoasuna00@qq.com";
        _smtpPassword = "ssvbzaqvotjcchjh";
        _fromEmail = "saokiritoasuna00@qq.com";
        _fromName = "QK Chat";
    }
    
    // 初始化Redis服务
    _redisService = new RedisService(this);
    if (_redisService) {
        initializeRedis();
    } else {
        qCWarning(emailService) << "Failed to create RedisService";
    }
    
    // 初始化SMTP客户端
    _smtpClient = new SimpleSmtpClient(this);
    if (_smtpClient) {
        _smtpClient->setSmtpConfig(_smtpHost, _smtpPort, _smtpUsername, _smtpPassword);
    } else {
        qCWarning(emailService) << "Failed to create SimpleSmtpClient";
    }
    
    // 连接SMTP客户端信号
    if (_smtpClient) {
        connect(_smtpClient, &SimpleSmtpClient::emailSent,
                this, &EmailVerificationService::onEmailSent);
        connect(_smtpClient, &SimpleSmtpClient::connectionError,
                this, &EmailVerificationService::onEmailError);
    }
    
    // 设置定时清理过期验证码
    connect(_cleanupTimer, &QTimer::timeout, this, &EmailVerificationService::clearExpiredCodes);
    _cleanupTimer->start(60000); // 每分钟清理一次
    
    qCInfo(emailService) << "EmailVerificationService initialized";
}

EmailVerificationService::~EmailVerificationService()
{
    closeRedis();
}

bool EmailVerificationService::initializeRedis()
{
    if (!_redisService) {
        return false;
    }
    
    bool success = _redisService->initialize();
    if (success) {
        qCInfo(emailService) << "Redis service initialized successfully";
    } else {
        qCWarning(emailService) << "Failed to initialize Redis service";
    }
    
    return success;
}

bool EmailVerificationService::isRedisConnected() const
{
    return _redisService && _redisService->isConnected();
}

void EmailVerificationService::closeRedis()
{
    if (_redisService) {
        _redisService->close();
    }
}

bool EmailVerificationService::sendVerificationCode(const QString &email)
{
    qCInfo(emailService) << "=== SENDING VERIFICATION CODE ===";
    qCInfo(emailService) << "Email:" << email;
    qCInfo(emailService) << "Redis connected:" << isRedisConnected();
    qCInfo(emailService) << "SMTP client available:" << (_smtpClient != nullptr);
    
    QMutexLocker locker(&_mutex);
    
    // 检查Redis连接，如果不可用则使用本地缓存
    if (!isRedisConnected()) {
        qCWarning(emailService) << "Redis not connected, will use local cache";
        // 不直接返回false，继续使用本地缓存
    }
    
    // 生成验证码
    QString code = generateVerificationCode();
    qCInfo(emailService) << "Generated verification code:" << code;
    
    // 存储验证码（优先Redis，失败则使用本地缓存）
    qCInfo(emailService) << "Storing code to Redis...";
    bool storedToRedis = setCodeToRedis(email, code);
    if (!storedToRedis) {
        qCWarning(emailService) << "Failed to store code to Redis, using local cache";
        if (!setCodeToLocalCache(email, code)) {
            qCWarning(emailService) << "Failed to store code to local cache";
            emit verificationCodeSent(email, false, "验证码存储失败");
            return false;
        }
        qCInfo(emailService) << "Code stored to local cache successfully";
    } else {
        qCInfo(emailService) << "Code stored to Redis successfully";
    }
    
    // 生成邮件内容
    QString subject = "QK Chat - 邮箱验证码";
    QString content = generateEmailContent(code);
    
    // 发送邮件
    sendEmailViaHttp(email, subject, content);
    
    // 在开发环境中，也输出验证码到日志（仅用于测试）
    qCInfo(emailService) << "=== DEVELOPMENT MODE ===";
    qCInfo(emailService) << "Email:" << email;
    qCInfo(emailService) << "Verification Code:" << code;
    qCInfo(emailService) << "=== END DEVELOPMENT MODE ===";
    
    qCInfo(emailService) << "Verification code sent to" << email;
    return true;
}

bool EmailVerificationService::sendVerificationCodeWithRetry(const QString &email, int maxRetries)
{
    qCInfo(emailService) << "=== SENDING VERIFICATION CODE WITH RETRY ===";
    qCInfo(emailService) << "Email:" << email << "Max retries:" << maxRetries;
    
    for (int i = 0; i < maxRetries; i++) {
        qCInfo(emailService) << "Attempt" << (i + 1) << "of" << maxRetries;
        
        if (sendVerificationCode(email)) {
            qCInfo(emailService) << "Email verification code sent successfully on attempt" << (i + 1);
            return true;
        }
        
        if (i < maxRetries - 1) {
            int delay = 1000 * (i + 1); // 递增延迟：1秒、2秒、3秒
            qCInfo(emailService) << "Retrying in" << delay << "ms...";
            QThread::msleep(delay);
        }
    }
    
    qCWarning(emailService) << "Failed to send verification code after" << maxRetries << "attempts";
    emit verificationCodeSent(email, false, "发送失败，请稍后重试");
    return false;
}

bool EmailVerificationService::verifyCode(const QString &email, const QString &code)
{
    QMutexLocker locker(&_mutex);
    
    // 优先从Redis获取，失败则从本地缓存获取
    QString storedCode = getCodeFromRedis(email);
    bool fromRedis = true;
    
    if (storedCode.isEmpty()) {
        qCInfo(emailService) << "Code not found in Redis, checking local cache";
        storedCode = getCodeFromLocalCache(email);
        fromRedis = false;
    }
    
    if (storedCode.isEmpty()) {
        emit verificationCodeVerified(email, false, "验证码不存在或已过期");
        return false;
    }
    
    if (storedCode != code) {
        emit verificationCodeVerified(email, false, "验证码错误");
        return false;
    }
    
    // 验证成功后删除验证码
    if (fromRedis) {
        deleteCodeFromRedis(email);
    } else {
        deleteCodeFromLocalCache(email);
    }
    
    emit verificationCodeVerified(email, true, "验证成功");
    qCInfo(emailService) << "Verification code verified for" << email << "from" << (fromRedis ? "Redis" : "local cache");
    return true;
}

bool EmailVerificationService::isCodeExpired(const QString &email)
{
    return getCodeFromRedis(email).isEmpty();
}

void EmailVerificationService::clearExpiredCodes()
{
    // 清理Redis过期缓存
    if (_redisService) {
        _redisService->clearExpiredCache();
    }
    
    // 清理本地缓存过期数据
    clearExpiredLocalCache();
    
    qCDebug(emailService) << "Clearing expired verification codes";
}

QString EmailVerificationService::generateVerificationCode()
{
    // 生成6位数字验证码
    QString code;
    for (int i = 0; i < 6; ++i) {
        code += QString::number(QRandomGenerator::global()->bounded(10));
    }
    return code;
}

QString EmailVerificationService::generateEmailContent(const QString &code)
{
    QString content = QString(
        "<html><body>"
        "<h2>QK Chat 邮箱验证</h2>"
        "<p>您好！</p>"
        "<p>您的验证码是：<strong style='color: #2196F3; font-size: 20px;'>%1</strong></p>"
        "<p>验证码有效期为5分钟，请尽快完成验证。</p>"
        "<p>如果这不是您的操作，请忽略此邮件。</p>"
        "<br>"
        "<p>此邮件由系统自动发送，请勿回复。</p>"
        "</body></html>"
    ).arg(code);
    
    return content;
}

bool EmailVerificationService::setCodeToRedis(const QString &email, const QString &code, int expirationSeconds)
{
    if (!_redisService) {
        return false;
    }
    
    return _redisService->setVerificationCode(email, code, expirationSeconds);
}

bool EmailVerificationService::setCodeToLocalCache(const QString &email, const QString &code, int expirationSeconds)
{
    QMutexLocker locker(&_cacheMutex);
    QDateTime expiration = QDateTime::currentDateTime().addSecs(expirationSeconds);
    _localCache[email] = qMakePair(code, expiration);
    qCInfo(emailService) << "Code stored in local cache for" << email << "expires at" << expiration;
    return true;
}

QString EmailVerificationService::getCodeFromRedis(const QString &email)
{
    if (!_redisService) {
        return QString();
    }
    
    return _redisService->getVerificationCode(email);
}

QString EmailVerificationService::getCodeFromLocalCache(const QString &email)
{
    QMutexLocker locker(&_cacheMutex);
    if (_localCache.contains(email)) {
        QPair<QString, QDateTime> codeData = _localCache[email];
        if (QDateTime::currentDateTime() < codeData.second) {
            qCInfo(emailService) << "Code retrieved from local cache for" << email;
            return codeData.first;
        } else {
            qCInfo(emailService) << "Code expired in local cache for" << email;
            _localCache.remove(email);
        }
    }
    return QString();
}

bool EmailVerificationService::deleteCodeFromRedis(const QString &email)
{
    if (!_redisService) {
        return false;
    }
    
    return _redisService->deleteVerificationCode(email);
}

bool EmailVerificationService::deleteCodeFromLocalCache(const QString &email)
{
    QMutexLocker locker(&_cacheMutex);
    bool removed = _localCache.remove(email) > 0;
    if (removed) {
        qCInfo(emailService) << "Code removed from local cache for" << email;
    }
    return removed;
}

void EmailVerificationService::clearExpiredLocalCache()
{
    QMutexLocker locker(&_cacheMutex);
    QDateTime now = QDateTime::currentDateTime();
    QList<QString> expiredKeys;
    
    for (auto it = _localCache.begin(); it != _localCache.end(); ++it) {
        if (now >= it.value().second) {
            expiredKeys.append(it.key());
        }
    }
    
    for (const QString& key : expiredKeys) {
        _localCache.remove(key);
    }
    
    if (!expiredKeys.isEmpty()) {
        qCInfo(emailService) << "Cleared" << expiredKeys.size() << "expired codes from local cache";
    }
}

void EmailVerificationService::sendEmailViaHttp(const QString &toEmail, const QString &subject, const QString &content)
{
    qCInfo(emailService) << "=== SENDING EMAIL VIA HTTP ===";
    qCInfo(emailService) << "To:" << toEmail;
    qCInfo(emailService) << "Subject:" << subject;
    qCInfo(emailService) << "SMTP client available:" << (_smtpClient != nullptr);
    
    // 使用SimpleSmtpClient发送邮件
    if (_smtpClient) {
        qCInfo(emailService) << "Sending email via SMTP to:" << toEmail;
        _smtpClient->sendEmail(toEmail, subject, content);
    } else {
        qCWarning(emailService) << "SMTP client not initialized";
        emit verificationCodeSent(toEmail, false, "SMTP客户端未初始化");
    }
}

void EmailVerificationService::setSmtpConfig(const QString &host, int port, const QString &username, const QString &password)
{
    _smtpHost = host;
    _smtpPort = port;
    _smtpUsername = username;
    _smtpPassword = password;
}

void EmailVerificationService::setFromEmail(const QString &fromEmail)
{
    _fromEmail = fromEmail;
}

void EmailVerificationService::setFromName(const QString &fromName)
{
    _fromName = fromName;
}

void EmailVerificationService::onEmailSent(const QString &email, bool success, const QString &message)
{
    emit verificationCodeSent(email, success, message);
}

void EmailVerificationService::onEmailError(const QString &error)
{
    qCWarning(emailService) << "Email sending error:" << error;
    emit emailError(error);
}