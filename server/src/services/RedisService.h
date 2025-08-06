#ifndef REDISSERVICE_H
#define REDISSERVICE_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QTimer>
#include <QMutex>
#include <QMap>

/**
 * @brief Redis服务类
 * 
 * 提供Redis连接和操作功能，包括：
 * - 连接管理
 * - 键值操作
 * - 过期时间管理
 * - 验证码存储
 */
class RedisService : public QObject
{
    Q_OBJECT
    
public:
    explicit RedisService(QObject *parent = nullptr);
    ~RedisService();
    
    // 连接管理
    bool initialize();
    bool isConnected() const;
    void close();
    bool reconnect();
    
    // 基本操作
    bool set(const QString &key, const QString &value, int expirationSeconds = 0);
    QString get(const QString &key);
    bool deleteKey(const QString &key);
    bool exists(const QString &key);
    bool expire(const QString &key, int seconds);
    int ttl(const QString &key);
    
    // 验证码操作
    bool setVerificationCode(const QString &email, const QString &code, int expirationSeconds = 300);
    QString getVerificationCode(const QString &email);
    bool deleteVerificationCode(const QString &email);
    bool isVerificationCodeExpired(const QString &email);
    
    // 会话管理
    bool setSession(const QString &sessionToken, const QString &sessionData, int expirationSeconds = 1800);
    QString getSession(const QString &sessionToken);
    bool deleteSession(const QString &sessionToken);
    bool updateSessionExpiration(const QString &sessionToken, int expirationSeconds);
    
    // 缓存管理
    bool setCache(const QString &key, const QString &value, int expirationSeconds = 3600);
    QString getCache(const QString &key);
    bool deleteCache(const QString &key);
    void clearExpiredCache();
    
    // 配置
    void setHost(const QString &host);
    void setPort(int port);
    void setPassword(const QString &password);
    void setDatabase(int database);
    void setTimeout(int timeout);
    
signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    void operationError(const QString &operation, const QString &error);
    
private slots:
    void onConnectionTimeout();
    void onReconnectTimer();
    
private:
    // 连接参数
    QString _host;
    int _port;
    QString _password;
    int _database;
    int _timeout;
    bool _connected;
    
    // 重连管理
    QTimer *_reconnectTimer;
    int _reconnectAttempts;
    int _maxReconnectAttempts;
    
    // 互斥锁
    mutable QMutex _mutex;
    
    // 内存存储（简化实现，实际应该使用Redis客户端库）
    QMap<QString, QPair<QString, QDateTime>> _storage;
    
    // 内部方法
    bool connectToRedis();
    void disconnectFromRedis();
    void scheduleReconnect();
    void cleanupExpiredKeys();
    
    // 键前缀
    static const QString VERIFICATION_CODE_PREFIX;
    static const QString SESSION_PREFIX;
    static const QString CACHE_PREFIX;
};

#endif // REDISSERVICE_H