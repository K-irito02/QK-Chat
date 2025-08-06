#include "RedisService.h"
#include "../config/ServerConfig.h"
#include "../utils/LogManager.h"
#include <QTimer>
#include <QDateTime>
#include <QLoggingCategory>
#include <QThread>
#include <QElapsedTimer>

Q_LOGGING_CATEGORY(redisService, "qkchat.server.redis")

// 静态常量定义
const QString RedisService::VERIFICATION_CODE_PREFIX = "verification:";
const QString RedisService::SESSION_PREFIX = "session:";
const QString RedisService::CACHE_PREFIX = "cache:";

RedisService::RedisService(QObject *parent)
    : QObject(parent)
    , _port(6379)
    , _database(0)
    , _timeout(5000)
    , _connected(false)
    , _reconnectTimer(nullptr)
    , _reconnectAttempts(0)
    , _maxReconnectAttempts(5)
{
    // 从配置加载Redis设置
    ServerConfig *config = ServerConfig::instance();
    if (config) {
        _host = config->getRedisHost();
        _port = config->getRedisPort();
        _password = config->getRedisPassword();
        _database = config->getRedisDatabase();
    } else {
        _host = "localhost";
    }
    
    // 初始化重连定时器
    _reconnectTimer = new QTimer(this);
    _reconnectTimer->setSingleShot(true);
    connect(_reconnectTimer, &QTimer::timeout, this, &RedisService::onReconnectTimer);
    
    qCInfo(redisService) << "RedisService initialized with host:" << _host << "port:" << _port;
}

RedisService::~RedisService()
{
    close();
}

bool RedisService::initialize()
{
    QMutexLocker locker(&_mutex);
    
    if (_connected) {
        qCInfo(redisService) << "Redis already connected";
        return true;
    }
    
    qCInfo(redisService) << "Initializing Redis connection...";
    
    // 在实际实现中，这里应该使用Redis客户端库
    // 为了简化，我们使用内存存储模拟Redis
    _connected = true;
    _storage.clear();
    
    qCInfo(redisService) << "Redis connection initialized (simulated)";
    emit connected();
    
    return true;
}

bool RedisService::isConnected() const
{
    QMutexLocker locker(&_mutex);
    return _connected;
}

void RedisService::close()
{
    QMutexLocker locker(&_mutex);
    
    if (_reconnectTimer) {
        _reconnectTimer->stop();
    }
    
    _connected = false;
    _storage.clear();
    
    qCInfo(redisService) << "Redis connection closed";
    emit disconnected();
}

bool RedisService::reconnect()
{
    close();
    return initialize();
}

bool RedisService::set(const QString &key, const QString &value, int expirationSeconds)
{
    QMutexLocker locker(&_mutex);
    
    if (!_connected) {
        emit operationError("set", "Redis not connected");
        return false;
    }
    
    QDateTime expiresAt;
    if (expirationSeconds > 0) {
        expiresAt = QDateTime::currentDateTime().addSecs(expirationSeconds);
    }
    
    _storage[key] = qMakePair(value, expiresAt);
    
    qCDebug(redisService) << "Set key:" << key << "value:" << value << "expires:" << expiresAt;
    return true;
}

QString RedisService::get(const QString &key)
{
    QMutexLocker locker(&_mutex);
    
    if (!_connected) {
        emit operationError("get", "Redis not connected");
        return QString();
    }
    
    if (!_storage.contains(key)) {
        return QString();
    }
    
    QPair<QString, QDateTime> data = _storage[key];
    
    // 检查是否过期
    if (data.second.isValid() && QDateTime::currentDateTime() > data.second) {
        _storage.remove(key);
        return QString();
    }
    
    return data.first;
}

bool RedisService::deleteKey(const QString &key)
{
    QMutexLocker locker(&_mutex);
    
    if (!_connected) {
        emit operationError("delete", "Redis not connected");
        return false;
    }
    
    bool removed = _storage.remove(key) > 0;
    qCDebug(redisService) << "Delete key:" << key << "result:" << removed;
    return removed;
}

bool RedisService::exists(const QString &key)
{
    QMutexLocker locker(&_mutex);
    
    if (!_connected) {
        return false;
    }
    
    if (!_storage.contains(key)) {
        return false;
    }
    
    QPair<QString, QDateTime> data = _storage[key];
    
    // 检查是否过期
    if (data.second.isValid() && QDateTime::currentDateTime() > data.second) {
        _storage.remove(key);
        return false;
    }
    
    return true;
}

bool RedisService::expire(const QString &key, int seconds)
{
    QMutexLocker locker(&_mutex);
    
    if (!_connected) {
        emit operationError("expire", "Redis not connected");
        return false;
    }
    
    if (!_storage.contains(key)) {
        return false;
    }
    
    QDateTime newExpiresAt = QDateTime::currentDateTime().addSecs(seconds);
    _storage[key].second = newExpiresAt;
    
    qCDebug(redisService) << "Set expiration for key:" << key << "seconds:" << seconds;
    return true;
}

int RedisService::ttl(const QString &key)
{
    QMutexLocker locker(&_mutex);
    
    if (!_connected) {
        return -2; // Redis not connected
    }
    
    if (!_storage.contains(key)) {
        return -2; // Key doesn't exist
    }
    
    QPair<QString, QDateTime> data = _storage[key];
    
    if (!data.second.isValid()) {
        return -1; // No expiration
    }
    
    int remaining = QDateTime::currentDateTime().secsTo(data.second);
    return remaining > 0 ? remaining : -2; // Expired
}

bool RedisService::setVerificationCode(const QString &email, const QString &code, int expirationSeconds)
{
    QString key = VERIFICATION_CODE_PREFIX + email;
    return set(key, code, expirationSeconds);
}

QString RedisService::getVerificationCode(const QString &email)
{
    QString key = VERIFICATION_CODE_PREFIX + email;
    return get(key);
}

bool RedisService::deleteVerificationCode(const QString &email)
{
    QString key = VERIFICATION_CODE_PREFIX + email;
    return deleteKey(key);
}

bool RedisService::isVerificationCodeExpired(const QString &email)
{
    QString key = VERIFICATION_CODE_PREFIX + email;
    return !exists(key);
}

bool RedisService::setSession(const QString &sessionToken, const QString &sessionData, int expirationSeconds)
{
    QString key = SESSION_PREFIX + sessionToken;
    return set(key, sessionData, expirationSeconds);
}

QString RedisService::getSession(const QString &sessionToken)
{
    QString key = SESSION_PREFIX + sessionToken;
    return get(key);
}

bool RedisService::deleteSession(const QString &sessionToken)
{
    QString key = SESSION_PREFIX + sessionToken;
    return deleteKey(key);
}

bool RedisService::updateSessionExpiration(const QString &sessionToken, int expirationSeconds)
{
    QString key = SESSION_PREFIX + sessionToken;
    return expire(key, expirationSeconds);
}

bool RedisService::setCache(const QString &key, const QString &value, int expirationSeconds)
{
    QString fullKey = CACHE_PREFIX + key;
    return set(fullKey, value, expirationSeconds);
}

QString RedisService::getCache(const QString &key)
{
    QString fullKey = CACHE_PREFIX + key;
    return get(fullKey);
}

bool RedisService::deleteCache(const QString &key)
{
    QString fullKey = CACHE_PREFIX + key;
    return deleteKey(fullKey);
}

void RedisService::clearExpiredCache()
{
    QMutexLocker locker(&_mutex);
    
    QDateTime now = QDateTime::currentDateTime();
    QStringList keysToRemove;
    
    for (auto it = _storage.begin(); it != _storage.end(); ++it) {
        if (it.value().second.isValid() && now > it.value().second) {
            keysToRemove.append(it.key());
        }
    }
    
    for (const QString &key : keysToRemove) {
        _storage.remove(key);
    }
    
    if (!keysToRemove.isEmpty()) {
        qCDebug(redisService) << "Cleared" << keysToRemove.size() << "expired keys";
    }
}

void RedisService::setHost(const QString &host)
{
    _host = host;
}

void RedisService::setPort(int port)
{
    _port = port;
}

void RedisService::setPassword(const QString &password)
{
    _password = password;
}

void RedisService::setDatabase(int database)
{
    _database = database;
}

void RedisService::setTimeout(int timeout)
{
    _timeout = timeout;
}

bool RedisService::connectToRedis()
{
    // 在实际实现中，这里应该使用Redis客户端库连接
    // 为了简化，我们返回true表示连接成功
    return true;
}

void RedisService::disconnectFromRedis()
{
    _connected = false;
    _storage.clear();
}

void RedisService::scheduleReconnect()
{
    if (_reconnectAttempts < _maxReconnectAttempts) {
        int delay = qMin(1000 * (1 << _reconnectAttempts), 30000); // 指数退避，最大30秒
        _reconnectTimer->start(delay);
        qCInfo(redisService) << "Scheduling Redis reconnect attempt" << (_reconnectAttempts + 1) << "in" << delay << "ms";
    } else {
        qCWarning(redisService) << "Max Redis reconnect attempts reached";
    }
}

void RedisService::cleanupExpiredKeys()
{
    clearExpiredCache();
}

void RedisService::onConnectionTimeout()
{
    qCWarning(redisService) << "Redis connection timeout";
    emit connectionError("Connection timeout");
    scheduleReconnect();
}

void RedisService::onReconnectTimer()
{
    _reconnectAttempts++;
    qCInfo(redisService) << "Attempting Redis reconnect" << _reconnectAttempts;
    
    if (connectToRedis()) {
        _connected = true;
        _reconnectAttempts = 0;
        qCInfo(redisService) << "Redis reconnected successfully";
        emit connected();
    } else {
        qCWarning(redisService) << "Redis reconnect attempt" << _reconnectAttempts << "failed";
        if (_reconnectAttempts < _maxReconnectAttempts) {
            scheduleReconnect();
        } else {
            emit connectionError("Max reconnect attempts reached");
        }
    }
}