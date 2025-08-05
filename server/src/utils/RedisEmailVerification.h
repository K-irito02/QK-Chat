#ifndef REDISEMAILVERIFICATION_H
#define REDISEMAILVERIFICATION_H

#include <QObject>
#include <QString>
#include "../cache/CacheManagerV2.h"

/**
 * @brief Redis邮箱验证管理器
 * 
 * 使用Redis存储邮箱验证码，提供高性能的验证码存储和验证功能
 */
class RedisEmailVerification : public QObject
{
    Q_OBJECT

public:
    static RedisEmailVerification& instance();
    
    // 保存邮箱验证码
    bool saveVerificationCode(const QString& email, const QString& code, int expirySeconds = 600);
    
    // 验证邮箱验证码
    bool verifyCode(const QString& email, const QString& code);
    
    // 删除邮箱验证码
    bool deleteCode(const QString& email);
    
    // 检查邮箱是否已有验证码
    bool hasCode(const QString& email);
    
    // 获取剩余有效期（秒）
    int getRemainingTime(const QString& email);
    
    // 检查邮箱是否已注册
    bool isEmailRegistered(const QString& email);

private:
    explicit RedisEmailVerification(QObject* parent = nullptr);
    ~RedisEmailVerification() = default;
    
    QString getRedisKey(const QString& email) const;
    
    Q_DISABLE_COPY(RedisEmailVerification)
};

#endif // REDISEMAILVERIFICATION_H