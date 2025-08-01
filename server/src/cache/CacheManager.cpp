#include "CacheManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>
#include <algorithm>

Q_LOGGING_CATEGORY(cacheManager, "qkchat.server.cache")

CacheManager::CacheManager(QObject *parent)
    : QObject(parent)
    , m_defaultPolicy(LRU)
    , m_maxSize(100 * 1024 * 1024) // 100MB
    , m_maxItems(10000)
    , m_defaultTTL(3600) // 1 hour
    , m_cleanupInterval(300) // 5 minutes
    , m_memoryLimit(200 * 1024 * 1024) // 200MB
    , m_persistenceEnabled(false)
    , m_cleanupTimer(nullptr)
    , m_memoryCheckTimer(nullptr)
{
    // 初始化统计信息
    m_stats.hitCount = 0;
    m_stats.missCount = 0;
    m_stats.totalRequests = 0;
    m_stats.hitRate = 0.0;
    m_stats.totalSize = 0;
    m_stats.itemCount = 0;
    m_stats.lastClearTime = QDateTime::currentDateTime();
    
    qCInfo(cacheManager) << "CacheManager created";
}

CacheManager::~CacheManager()
{
    if (m_persistenceEnabled && !m_persistenceFile.isEmpty()) {
        saveToFile(m_persistenceFile);
    }
    
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
        delete m_cleanupTimer;
    }
    
    if (m_memoryCheckTimer) {
        m_memoryCheckTimer->stop();
        delete m_memoryCheckTimer;
    }
    
    qCInfo(cacheManager) << "CacheManager destroyed";
}

bool CacheManager::initialize()
{
    // 创建清理定时器
    m_cleanupTimer = new QTimer(this);
    connect(m_cleanupTimer, &QTimer::timeout, this, &CacheManager::performCleanup);
    m_cleanupTimer->start(m_cleanupInterval * 1000);
    
    // 创建内存检查定时器
    m_memoryCheckTimer = new QTimer(this);
    connect(m_memoryCheckTimer, &QTimer::timeout, this, &CacheManager::checkMemoryUsage);
    m_memoryCheckTimer->start(60 * 1000); // 每分钟检查一次
    
    qCInfo(cacheManager) << "CacheManager initialized successfully";
    return true;
}

void CacheManager::setDefaultPolicy(CachePolicy policy)
{
    QMutexLocker locker(&m_mutex);
    m_defaultPolicy = policy;
    qCDebug(cacheManager) << "Default cache policy set to:" << policy;
}

void CacheManager::setMaxSize(qint64 maxSizeBytes)
{
    QMutexLocker locker(&m_mutex);
    m_maxSize = maxSizeBytes;
    qCDebug(cacheManager) << "Max cache size set to:" << maxSizeBytes << "bytes";
}

void CacheManager::setMaxItems(int maxItems)
{
    QMutexLocker locker(&m_mutex);
    m_maxItems = maxItems;
    qCDebug(cacheManager) << "Max cache items set to:" << maxItems;
}

void CacheManager::setDefaultTTL(int seconds)
{
    QMutexLocker locker(&m_mutex);
    m_defaultTTL = seconds;
    qCDebug(cacheManager) << "Default TTL set to:" << seconds << "seconds";
}

void CacheManager::setCleanupInterval(int seconds)
{
    m_cleanupInterval = seconds;
    if (m_cleanupTimer) {
        m_cleanupTimer->setInterval(seconds * 1000);
    }
    qCDebug(cacheManager) << "Cleanup interval set to:" << seconds << "seconds";
}

// 基本缓存操作
bool CacheManager::set(const QString &key, const QVariant &value, int ttlSeconds, const QString &category)
{
    QMutexLocker locker(&m_mutex);
    
    if (key.isEmpty()) {
        return false;
    }
    
    // 计算数据大小
    qint64 dataSize = calculateSize(value);
    
    // 检查是否需要清理空间
    if (shouldEvict()) {
        evictItems();
    }
    
    CacheItem item;
    item.data = value;
    item.createdAt = QDateTime::currentDateTime();
    item.lastAccessed = item.createdAt;
    item.accessCount = 1;
    item.size = dataSize;
    item.category = category;
    
    // 设置过期时间
    if (ttlSeconds > 0) {
        item.expiresAt = item.createdAt.addSecs(ttlSeconds);
    } else if (ttlSeconds == -1 && m_defaultTTL > 0) {
        item.expiresAt = item.createdAt.addSecs(m_defaultTTL);
    }
    
    // 如果键已存在，先移除旧的
    if (m_cache.contains(key)) {
        const CacheItem &oldItem = m_cache[key];
        m_stats.totalSize -= oldItem.size;
        removeFromCategory(oldItem.category, key);
    }
    
    m_cache[key] = item;
    m_stats.totalSize += dataSize;
    m_stats.itemCount = m_cache.size();
    
    // 添加到分类
    if (!category.isEmpty()) {
        addToCategory(category, key);
    }
    
    emit itemAdded(key, category);
    qCDebug(cacheManager) << "Item cached:" << key << "size:" << dataSize;
    
    return true;
}

QVariant CacheManager::get(const QString &key, const QVariant &defaultValue)
{
    QMutexLocker locker(&m_mutex);
    
    m_stats.totalRequests++;
    
    if (!m_cache.contains(key)) {
        m_stats.missCount++;
        updateStats(false);
        return defaultValue;
    }
    
    CacheItem &item = m_cache[key];
    
    // 检查是否过期
    if (isExpired(item)) {
        m_cache.remove(key);
        m_stats.totalSize -= item.size;
        m_stats.itemCount--;
        removeFromCategory(item.category, key);
        m_stats.missCount++;
        updateStats(false);
        emit itemExpired(key, item.category);
        return defaultValue;
    }
    
    // 更新访问信息
    item.lastAccessed = QDateTime::currentDateTime();
    item.accessCount++;
    
    m_stats.hitCount++;
    updateStats(true);
    
    return item.data;
}

bool CacheManager::remove(const QString &key)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_cache.contains(key)) {
        return false;
    }
    
    const CacheItem &item = m_cache[key];
    m_stats.totalSize -= item.size;
    removeFromCategory(item.category, key);
    m_cache.remove(key);
    m_stats.itemCount--;
    
    emit itemRemoved(key, item.category);
    qCDebug(cacheManager) << "Item removed:" << key;
    
    return true;
}

bool CacheManager::exists(const QString &key)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_cache.contains(key)) {
        return false;
    }
    
    const CacheItem &item = m_cache[key];
    if (isExpired(item)) {
        // 自动清理过期项
        m_cache.remove(key);
        m_stats.totalSize -= item.size;
        m_stats.itemCount--;
        removeFromCategory(item.category, key);
        emit itemExpired(key, item.category);
        return false;
    }
    
    return true;
}

void CacheManager::clear()
{
    QMutexLocker locker(&m_mutex);
    
    m_cache.clear();
    m_categories.clear();
    m_stats.totalSize = 0;
    m_stats.itemCount = 0;
    m_stats.lastClearTime = QDateTime::currentDateTime();
    
    emit cacheCleared();
    qCInfo(cacheManager) << "Cache cleared";
}

void CacheManager::clearCategory(const QString &category)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_categories.contains(category)) {
        return;
    }
    
    const QStringList &keys = m_categories[category];
    for (const QString &key : keys) {
        if (m_cache.contains(key)) {
            const CacheItem &item = m_cache[key];
            m_stats.totalSize -= item.size;
            m_cache.remove(key);
        }
    }
    
    m_categories.remove(category);
    m_stats.itemCount = m_cache.size();
    
    emit categoryCleared(category);
    qCInfo(cacheManager) << "Category cleared:" << category;
}

// 批量操作
bool CacheManager::setMultiple(const QHash<QString, QVariant> &items, int ttlSeconds, const QString &category)
{
    bool allSuccess = true;
    
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (!set(it.key(), it.value(), ttlSeconds, category)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

QHash<QString, QVariant> CacheManager::getMultiple(const QStringList &keys)
{
    QHash<QString, QVariant> result;
    
    for (const QString &key : keys) {
        QVariant value = get(key);
        if (value.isValid()) {
            result[key] = value;
        }
    }
    
    return result;
}

bool CacheManager::removeMultiple(const QStringList &keys)
{
    bool allSuccess = true;
    
    for (const QString &key : keys) {
        if (!remove(key)) {
            allSuccess = false;
        }
    }
    
    return allSuccess;
}

// 分类缓存
bool CacheManager::setInCategory(const QString &category, const QString &key, const QVariant &value, int ttlSeconds)
{
    return set(key, value, ttlSeconds, category);
}

QVariant CacheManager::getFromCategory(const QString &category, const QString &key, const QVariant &defaultValue)
{
    Q_UNUSED(category) // 分类信息已经在缓存项中
    return get(key, defaultValue);
}

QStringList CacheManager::getCategoryKeys(const QString &category)
{
    QMutexLocker locker(&m_mutex);
    return m_categories.value(category);
}

QHash<QString, QVariant> CacheManager::getCategoryData(const QString &category)
{
    QMutexLocker locker(&m_mutex);
    
    QHash<QString, QVariant> result;
    const QStringList &keys = m_categories.value(category);
    
    for (const QString &key : keys) {
        if (m_cache.contains(key)) {
            const CacheItem &item = m_cache[key];
            if (!isExpired(item)) {
                result[key] = item.data;
            }
        }
    }
    
    return result;
}

// 用户相关缓存
bool CacheManager::cacheUserInfo(qint64 userId, const QVariantMap &userInfo, int ttlSeconds)
{
    QString key = generateUserKey(userId);
    return set(key, userInfo, ttlSeconds, "users");
}

QVariantMap CacheManager::getUserInfo(qint64 userId)
{
    QString key = generateUserKey(userId);
    return get(key).toMap();
}

void CacheManager::invalidateUserCache(qint64 userId)
{
    QString key = generateUserKey(userId);
    remove(key);
}

// 群组相关缓存
bool CacheManager::cacheGroupInfo(qint64 groupId, const QVariantMap &groupInfo, int ttlSeconds)
{
    QString key = generateGroupKey(groupId);
    return set(key, groupInfo, ttlSeconds, "groups");
}

QVariantMap CacheManager::getGroupInfo(qint64 groupId)
{
    QString key = generateGroupKey(groupId);
    return get(key).toMap();
}

void CacheManager::invalidateGroupCache(qint64 groupId)
{
    QString key = generateGroupKey(groupId);
    remove(key);
}

bool CacheManager::cacheGroupMembers(qint64 groupId, const QVariantList &members, int ttlSeconds)
{
    QString key = QString("group_members:%1").arg(groupId);
    return set(key, members, ttlSeconds, "group_members");
}

QVariantList CacheManager::getGroupMembers(qint64 groupId)
{
    QString key = QString("group_members:%1").arg(groupId);
    return get(key).toList();
}

// 消息相关缓存
bool CacheManager::cacheRecentMessages(qint64 chatId, const QVariantList &messages, int ttlSeconds)
{
    QString key = generateMessageKey(chatId);
    return set(key, messages, ttlSeconds, "messages");
}

QVariantList CacheManager::getRecentMessages(qint64 chatId)
{
    QString key = generateMessageKey(chatId);
    return get(key).toList();
}

void CacheManager::invalidateMessageCache(qint64 chatId)
{
    QString key = generateMessageKey(chatId);
    remove(key);
}

// 会话相关缓存
bool CacheManager::cacheUserSession(const QString &sessionToken, qint64 userId, int ttlSeconds)
{
    QString key = generateSessionKey(sessionToken);
    return set(key, userId, ttlSeconds, "sessions");
}

qint64 CacheManager::getUserFromSession(const QString &sessionToken)
{
    QString key = generateSessionKey(sessionToken);
    return get(key, -1).toLongLong();
}

void CacheManager::invalidateSession(const QString &sessionToken)
{
    QString key = generateSessionKey(sessionToken);
    remove(key);
}

// 统计和监控
CacheManager::CacheStats CacheManager::getStats() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats;
}

double CacheManager::getHitRate() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats.hitRate;
}

qint64 CacheManager::getTotalSize() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats.totalSize;
}

int CacheManager::getItemCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats.itemCount;
}

QStringList CacheManager::getKeys() const
{
    QMutexLocker locker(&m_mutex);
    return m_cache.keys();
}

QStringList CacheManager::getCategories() const
{
    QMutexLocker locker(&m_mutex);
    return m_categories.keys();
}

// 内存管理
void CacheManager::cleanup()
{
    QMutexLocker locker(&m_mutex);
    
    // 清理过期项
    evictExpired();
    
    // 如果仍然超出限制，执行其他清理策略
    if (shouldEvict()) {
        evictItems();
    }
    
    qCDebug(cacheManager) << "Cleanup completed. Items:" << m_cache.size() << "Size:" << m_stats.totalSize;
}

void CacheManager::compactMemory()
{
    QMutexLocker locker(&m_mutex);
    
    // 重建哈希表以减少内存碎片
    QHash<QString, CacheItem> newCache;
    QHash<QString, QStringList> newCategories;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (!isExpired(it.value())) {
            newCache[it.key()] = it.value();
            if (!it.value().category.isEmpty()) {
                newCategories[it.value().category].append(it.key());
            }
        }
    }
    
    m_cache = std::move(newCache);
    m_categories = std::move(newCategories);
    
    // 重新计算统计信息
    m_stats.itemCount = m_cache.size();
    m_stats.totalSize = 0;
    for (const auto &item : m_cache) {
        m_stats.totalSize += item.size;
    }
    
    qCInfo(cacheManager) << "Memory compacted. Items:" << m_cache.size() << "Size:" << m_stats.totalSize;
}

qint64 CacheManager::getMemoryUsage() const
{
    QMutexLocker locker(&m_mutex);
    return m_stats.totalSize;
}

void CacheManager::setMemoryLimit(qint64 limitBytes)
{
    QMutexLocker locker(&m_mutex);
    m_memoryLimit = limitBytes;
    qCDebug(cacheManager) << "Memory limit set to:" << limitBytes << "bytes";
}

// 持久化
bool CacheManager::saveToFile(const QString &filePath)
{
    QMutexLocker locker(&m_mutex);
    
    QJsonObject root;
    QJsonObject cacheData;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        const CacheItem &item = it.value();
        if (!isExpired(item)) {
            QJsonObject itemObj;
            itemObj["data"] = QJsonValue::fromVariant(item.data);
            itemObj["createdAt"] = item.createdAt.toMSecsSinceEpoch();
            itemObj["lastAccessed"] = item.lastAccessed.toMSecsSinceEpoch();
            itemObj["expiresAt"] = item.expiresAt.toMSecsSinceEpoch();
            itemObj["accessCount"] = item.accessCount;
            itemObj["size"] = item.size;
            itemObj["category"] = item.category;
            
            cacheData[it.key()] = itemObj;
        }
    }
    
    root["cache"] = cacheData;
    root["version"] = "1.0";
    root["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    
    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(cacheManager) << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    file.write(doc.toJson());
    qCInfo(cacheManager) << "Cache saved to file:" << filePath;
    return true;
}

bool CacheManager::loadFromFile(const QString &filePath)
{
    QMutexLocker locker(&m_mutex);
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(cacheManager) << "Failed to open file for reading:" << filePath;
        return false;
    }
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError) {
        qCWarning(cacheManager) << "JSON parse error:" << error.errorString();
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonObject cacheData = root["cache"].toObject();
    
    clear();
    
    for (auto it = cacheData.begin(); it != cacheData.end(); ++it) {
        QJsonObject itemObj = it.value().toObject();
        
        CacheItem item;
        item.data = itemObj["data"].toVariant();
        item.createdAt = QDateTime::fromMSecsSinceEpoch(itemObj["createdAt"].toVariant().toLongLong());
        item.lastAccessed = QDateTime::fromMSecsSinceEpoch(itemObj["lastAccessed"].toVariant().toLongLong());
        item.expiresAt = QDateTime::fromMSecsSinceEpoch(itemObj["expiresAt"].toVariant().toLongLong());
        item.accessCount = itemObj["accessCount"].toInt();
        item.size = itemObj["size"].toVariant().toLongLong();
        item.category = itemObj["category"].toString();
        
        if (!isExpired(item)) {
            m_cache[it.key()] = item;
            m_stats.totalSize += item.size;
            if (!item.category.isEmpty()) {
                addToCategory(item.category, it.key());
            }
        }
    }
    
    m_stats.itemCount = m_cache.size();
    qCInfo(cacheManager) << "Cache loaded from file:" << filePath << "Items:" << m_cache.size();
    return true;
}

void CacheManager::enablePersistence(bool enable, const QString &filePath)
{
    m_persistenceEnabled = enable;
    if (!filePath.isEmpty()) {
        m_persistenceFile = filePath;
    }
    
    if (enable && m_persistenceFile.isEmpty()) {
        // 使用默认路径
        QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(appDataPath);
        m_persistenceFile = appDataPath + "/cache.json";
    }
    
    qCDebug(cacheManager) << "Persistence enabled:" << enable << "File:" << m_persistenceFile;
}

// 私有槽
void CacheManager::performCleanup()
{
    cleanup();
}

void CacheManager::checkMemoryUsage()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_stats.totalSize > m_memoryLimit) {
        emit memoryLimitExceeded(m_stats.totalSize, m_memoryLimit);
        qCWarning(cacheManager) << "Memory limit exceeded:" << m_stats.totalSize << ">" << m_memoryLimit;
        
        // 自动清理
        evictItems();
    }
}

// 私有方法
QString CacheManager::generateKey(const QString &prefix, qint64 id)
{
    return QString("%1:%2").arg(prefix).arg(id);
}

QString CacheManager::generateUserKey(qint64 userId)
{
    return generateKey("user", userId);
}

QString CacheManager::generateGroupKey(qint64 groupId)
{
    return generateKey("group", groupId);
}

QString CacheManager::generateMessageKey(qint64 chatId)
{
    return generateKey("messages", chatId);
}

QString CacheManager::generateSessionKey(const QString &sessionToken)
{
    return QString("session:%1").arg(sessionToken);
}

bool CacheManager::shouldEvict() const
{
    return m_stats.totalSize > m_maxSize || m_cache.size() > m_maxItems;
}

void CacheManager::evictItems()
{
    switch (m_defaultPolicy) {
    case LRU:
        evictLRU();
        break;
    case LFU:
        evictLFU();
        break;
    case FIFO:
        evictFIFO();
        break;
    case TTL:
        evictExpired();
        break;
    default:
        evictLRU();
        break;
    }
}

void CacheManager::evictLRU()
{
    if (m_cache.isEmpty()) {
        return;
    }
    
    // 找出最近最少使用的项
    QString oldestKey;
    QDateTime oldestTime = QDateTime::currentDateTime();
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.value().lastAccessed < oldestTime) {
            oldestTime = it.value().lastAccessed;
            oldestKey = it.key();
        }
    }
    
    if (!oldestKey.isEmpty()) {
        remove(oldestKey);
        qCDebug(cacheManager) << "LRU evicted:" << oldestKey;
    }
}

void CacheManager::evictLFU()
{
    if (m_cache.isEmpty()) {
        return;
    }
    
    // 找出使用频率最低的项
    QString leastUsedKey;
    int leastCount = INT_MAX;
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.value().accessCount < leastCount) {
            leastCount = it.value().accessCount;
            leastUsedKey = it.key();
        }
    }
    
    if (!leastUsedKey.isEmpty()) {
        remove(leastUsedKey);
        qCDebug(cacheManager) << "LFU evicted:" << leastUsedKey;
    }
}

void CacheManager::evictFIFO()
{
    if (m_cache.isEmpty()) {
        return;
    }
    
    // 找出最早创建的项
    QString oldestKey;
    QDateTime oldestTime = QDateTime::currentDateTime();
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (it.value().createdAt < oldestTime) {
            oldestTime = it.value().createdAt;
            oldestKey = it.key();
        }
    }
    
    if (!oldestKey.isEmpty()) {
        remove(oldestKey);
        qCDebug(cacheManager) << "FIFO evicted:" << oldestKey;
    }
}

void CacheManager::evictExpired()
{
    QStringList expiredKeys;
    QDateTime now = QDateTime::currentDateTime();
    
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        if (isExpired(it.value())) {
            expiredKeys.append(it.key());
        }
    }
    
    for (const QString &key : expiredKeys) {
        const CacheItem &item = m_cache[key];
        m_stats.totalSize -= item.size;
        removeFromCategory(item.category, key);
        m_cache.remove(key);
        emit itemExpired(key, item.category);
    }
    
    m_stats.itemCount = m_cache.size();
    
    if (!expiredKeys.isEmpty()) {
        qCDebug(cacheManager) << "Expired items evicted:" << expiredKeys.size();
    }
}

qint64 CacheManager::calculateSize(const QVariant &data) const
{
    // 简单的大小估算
    switch (data.type()) {
    case QVariant::String:
        return data.toString().size() * sizeof(QChar);
    case QVariant::ByteArray:
        return data.toByteArray().size();
    case QVariant::Map:
    case QVariant::Hash:
        // 使用JSON序列化来估算大小
        return QJsonDocument::fromVariant(data).toJson().size();
    case QVariant::List:
        return QJsonDocument::fromVariant(data).toJson().size();
    default:
        return 64; // 默认大小
    }
}

bool CacheManager::isExpired(const CacheItem &item) const
{
    if (!item.expiresAt.isValid()) {
        return false; // 永不过期
    }
    
    return QDateTime::currentDateTime() > item.expiresAt;
}

void CacheManager::updateStats(bool hit)
{
    if (m_stats.totalRequests > 0) {
        m_stats.hitRate = static_cast<double>(m_stats.hitCount) / m_stats.totalRequests;
    }
}

void CacheManager::addToCategory(const QString &category, const QString &key)
{
    if (!category.isEmpty()) {
        m_categories[category].append(key);
    }
}

void CacheManager::removeFromCategory(const QString &category, const QString &key)
{
    if (!category.isEmpty() && m_categories.contains(category)) {
        m_categories[category].removeAll(key);
        if (m_categories[category].isEmpty()) {
            m_categories.remove(category);
        }
    }
}

QByteArray CacheManager::serializeItem(const CacheItem &item) const
{
    QJsonObject obj;
    obj["data"] = QJsonValue::fromVariant(item.data);
    obj["createdAt"] = item.createdAt.toMSecsSinceEpoch();
    obj["lastAccessed"] = item.lastAccessed.toMSecsSinceEpoch();
    obj["expiresAt"] = item.expiresAt.toMSecsSinceEpoch();
    obj["accessCount"] = item.accessCount;
    obj["size"] = item.size;
    obj["category"] = item.category;
    
    return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

CacheManager::CacheItem CacheManager::deserializeItem(const QByteArray &data) const
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    CacheItem item;
    if (error.error == QJsonParseError::NoError) {
        QJsonObject obj = doc.object();
        item.data = obj["data"].toVariant();
        item.createdAt = QDateTime::fromMSecsSinceEpoch(obj["createdAt"].toVariant().toLongLong());
        item.lastAccessed = QDateTime::fromMSecsSinceEpoch(obj["lastAccessed"].toVariant().toLongLong());
        item.expiresAt = QDateTime::fromMSecsSinceEpoch(obj["expiresAt"].toVariant().toLongLong());
        item.accessCount = obj["accessCount"].toInt();
        item.size = obj["size"].toVariant().toLongLong();
        item.category = obj["category"].toString();
    }
    
    return item;
}

// 数据库查询缓存
bool CacheManager::cacheQuery(const QString &sql, const QVariantMap &parameters, const QVariantList &result, int ttlSeconds)
{
    QString key = generateQueryCacheKey(sql, parameters);
    return set(key, QVariant::fromValue(result), ttlSeconds, "db_query");
}

QVariantList CacheManager::getCachedQuery(const QString &sql, const QVariantMap &parameters)
{
    QString key = generateQueryCacheKey(sql, parameters);
    QVariant value = get(key);
    if (value.isValid() && value.canConvert<QVariantList>()) {
        return value.toList();
    }
    return QVariantList();
}

QString CacheManager::generateQueryCacheKey(const QString &sql, const QVariantMap &parameters)
{
    QByteArray data = sql.toUtf8();
    QJsonDocument doc = QJsonDocument::fromVariant(parameters);
    data.append(doc.toJson(QJsonDocument::Compact));
    return QString("query_").append(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
}