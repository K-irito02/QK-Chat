#include "RedisEmailVerification.h"
#include "../cache/CacheManagerV2.h"
#include "../database/Database.h"
#include "../database/DatabasePool.h"
#include <QLoggingCategory>
#include <QSqlQuery>
#include <QDateTime>

Q_LOGGING_CATEGORY(redisEmailVerification, "qkchat.server.redisemail")

RedisEmailVerification& RedisEmailVerification::instance()
{
    static RedisEmailVerification instance;
    return instance;
}

RedisEmailVerification::RedisEmailVerification(QObject* parent)
    : QObject(parent)
{
}

QString RedisEmailVerification::getRedisKey(const QString& email) const
{
    return QString("email:verification:%1").arg(email);
}

bool RedisEmailVerification::saveVerificationCode(const QString& email, const QString& code, int expirySeconds)
{
    QString key = getRedisKey(email);
    
    // 将验证码存储到Redis，设置过期时间
    QVariantMap data;
    data["code"] = code;
    data["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    // 使用CacheManagerV2的正确方法
    return CacheManagerV2::instance()->set(key, data, expirySeconds);
}

bool RedisEmailVerification::verifyCode(const QString& email, const QString& code)
{
    QString key = getRedisKey(email);
    
    // 从Redis获取验证码
    QVariantMap data = CacheManagerV2::instance()->get(key).toMap();
    if (data.isEmpty()) {
        qCDebug(redisEmailVerification) << "No verification code found for email:" << email;
        return false;
    }
    
    QString storedCode = data["code"].toString();
    bool valid = (storedCode == code);
    
    if (valid) {
        // 验证成功后删除验证码
        deleteCode(email);
        qCDebug(redisEmailVerification) << "Verification successful for email:" << email;
    } else {
        qCDebug(redisEmailVerification) << "Invalid verification code for email:" << email;
    }
    
    return valid;
}

bool RedisEmailVerification::deleteCode(const QString& email)
{
    QString key = getRedisKey(email);
    return CacheManagerV2::instance()->remove(key);
}

bool RedisEmailVerification::hasCode(const QString& email)
{
    QString key = getRedisKey(email);
    return CacheManagerV2::instance()->exists(key);
}

int RedisEmailVerification::getRemainingTime(const QString& email)
{
    QString key = getRedisKey(email);
    
    // 尝试从Redis获取TTL（剩余时间）
    QVariantMap data = CacheManagerV2::instance()->get(key).toMap();
    if (data.isEmpty()) {
        return 0;  // 验证码不存在或已过期
    }
    
    // 从创建时间和当前时间计算剩余时间
    QString createdStr = data["created"].toString();
    if (!createdStr.isEmpty()) {
        QDateTime createdTime = QDateTime::fromString(createdStr, Qt::ISODate);
        QDateTime currentTime = QDateTime::currentDateTime();
        qint64 elapsed = createdTime.secsTo(currentTime);
        
        // 默认10分钟有效期
        int remaining = 600 - static_cast<int>(elapsed);
        return qMax(0, remaining);
    }
    
    return 60;  // 默认返回1分钟，防止无限重发
}

bool RedisEmailVerification::isEmailRegistered(const QString& email)
{
    // 使用数据库池检查邮箱是否已注册
    DatabasePool* dbPool = DatabasePool::instance();
    if (!dbPool) {
        // 回退到单数据库连接
        Database database;
        if (!database.initialize()) {
            return false;
        }
        
        QString query = "SELECT COUNT(*) FROM users WHERE email = ?";
        QSqlQuery sqlQuery(database.getDatabase());
        sqlQuery.prepare(query);
        sqlQuery.addBindValue(email);
        
        if (database.executeQuery(sqlQuery) && sqlQuery.next()) {
            return sqlQuery.value(0).toInt() > 0;
        }
        return false;
    }
    
    // 使用数据库池的正确方法
    auto db = dbPool->acquireConnection();
    if (!db) return false;
    
    QString query = "SELECT COUNT(*) FROM users WHERE email = ?";
    QSqlQuery sqlQuery(db->database);
    sqlQuery.prepare(query);
    sqlQuery.addBindValue(email);
    
    bool result = false;
    if (sqlQuery.exec() && sqlQuery.next()) {
        result = sqlQuery.value(0).toInt() > 0;
    }
    
    dbPool->releaseConnection(db);
    return result;
}