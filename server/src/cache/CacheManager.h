#ifndef CACHEMANAGER_H
#define CACHEMANAGER_H

#include <QObject>
#include <QVariant>
#include <QDateTime>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(cacheManager)

class CacheManager : public QObject
{
    Q_OBJECT

public:
    // 缓存策略
    enum CachePolicy {
        NoCache = 0,        // 不缓存
        LRU = 1,           // 最近最少使用
        LFU = 2,           // 最少使用频率
        FIFO = 3,          // 先进先出
        TTL = 4            // 基于时间过期
    };
    Q_ENUM(CachePolicy)

    // 缓存项结构
    struct CacheItem {
        QVariant data;              // 缓存数据
        QDateTime createdAt;        // 创建时间
        QDateTime lastAccessed;     // 最后访问时间
        QDateTime expiresAt;        // 过期时间
        int accessCount;            // 访问次数
        qint64 size;               // 数据大小（字节）
        QString category;           // 缓存分类
    };

    // 缓存统计信息
    struct CacheStats {
        qint64 hitCount;           // 命中次数
        qint64 missCount;          // 未命中次数
        qint64 totalRequests;      // 总请求次数
        double hitRate;            // 命中率
        qint64 totalSize;          // 总缓存大小
        int itemCount;             // 缓存项数量
        QDateTime lastClearTime;   // 最后清理时间
    };

    explicit CacheManager(QObject *parent = nullptr);
    ~CacheManager();

    // 初始化和配置
    bool initialize();
    void setDefaultPolicy(CachePolicy policy);
    void setMaxSize(qint64 maxSizeBytes);
    void setMaxItems(int maxItems);
    void setDefaultTTL(int seconds);
    void setCleanupInterval(int seconds);

    // 基本缓存操作
    bool set(const QString &key, const QVariant &value, int ttlSeconds = -1, const QString &category = QString());
    QVariant get(const QString &key, const QVariant &defaultValue = QVariant());
    bool remove(const QString &key);
    bool exists(const QString &key);
    void clear();
    void clearCategory(const QString &category);

    // 批量操作
    bool setMultiple(const QHash<QString, QVariant> &items, int ttlSeconds = -1, const QString &category = QString());
    QHash<QString, QVariant> getMultiple(const QStringList &keys);
    bool removeMultiple(const QStringList &keys);

    // 分类缓存
    bool setInCategory(const QString &category, const QString &key, const QVariant &value, int ttlSeconds = -1);
    QVariant getFromCategory(const QString &category, const QString &key, const QVariant &defaultValue = QVariant());
    QStringList getCategoryKeys(const QString &category);
    QHash<QString, QVariant> getCategoryData(const QString &category);

    // 用户相关缓存
    bool cacheUserInfo(qint64 userId, const QVariantMap &userInfo, int ttlSeconds = 3600);
    QVariantMap getUserInfo(qint64 userId);
    void invalidateUserCache(qint64 userId);

    // 群组相关缓存
    bool cacheGroupInfo(qint64 groupId, const QVariantMap &groupInfo, int ttlSeconds = 1800);
    QVariantMap getGroupInfo(qint64 groupId);
    void invalidateGroupCache(qint64 groupId);
    bool cacheGroupMembers(qint64 groupId, const QVariantList &members, int ttlSeconds = 600);
    QVariantList getGroupMembers(qint64 groupId);

    // 消息相关缓存
    bool cacheRecentMessages(qint64 chatId, const QVariantList &messages, int ttlSeconds = 300);
    QVariantList getRecentMessages(qint64 chatId);
    void invalidateMessageCache(qint64 chatId);

    // 会话相关缓存
    bool cacheUserSession(const QString &sessionToken, qint64 userId, int ttlSeconds = 7200);
    qint64 getUserFromSession(const QString &sessionToken);
    void invalidateSession(const QString &sessionToken);

    // 数据库查询缓存
    bool cacheQuery(const QString &sql, const QVariantMap &parameters, const QVariantList &result, int ttlSeconds = 300);
    QVariantList getCachedQuery(const QString &sql, const QVariantMap &parameters);

    // 统计和监控
    CacheStats getStats() const;
    double getHitRate() const;
    qint64 getTotalSize() const;
    int getItemCount() const;
    QStringList getKeys() const;
    QStringList getCategories() const;

    // 内存管理
    void cleanup();
    void compactMemory();
    qint64 getMemoryUsage() const;
    void setMemoryLimit(qint64 limitBytes);

    // 持久化
    bool saveToFile(const QString &filePath);
    bool loadFromFile(const QString &filePath);
    void enablePersistence(bool enable, const QString &filePath = QString());

signals:
    void itemAdded(const QString &key, const QString &category);
    void itemRemoved(const QString &key, const QString &category);
    void itemExpired(const QString &key, const QString &category);
    void categoryCleared(const QString &category);
    void cacheCleared();
    void memoryLimitExceeded(qint64 currentSize, qint64 limit);

private slots:
    void performCleanup();
    void checkMemoryUsage();

private:
    // 配置参数
    CachePolicy m_defaultPolicy;
    qint64 m_maxSize;
    int m_maxItems;
    int m_defaultTTL;
    int m_cleanupInterval;
    qint64 m_memoryLimit;
    bool m_persistenceEnabled;
    QString m_persistenceFile;

    // 缓存存储
    QHash<QString, CacheItem> m_cache;
    QHash<QString, QStringList> m_categories; // 分类到键的映射
    
    // 统计信息
    mutable QMutex m_mutex;
    CacheStats m_stats;
    
    // 定时器
    QTimer *m_cleanupTimer;
    QTimer *m_memoryCheckTimer;

    // 内部方法
    QString generateKey(const QString &prefix, qint64 id);
    QString generateUserKey(qint64 userId);
    QString generateGroupKey(qint64 groupId);
    QString generateMessageKey(qint64 chatId);
    QString generateSessionKey(const QString &sessionToken);

    bool shouldEvict() const;
    void evictItems();
    void evictLRU();
    void evictLFU();
    void evictFIFO();
    void evictExpired();

    qint64 calculateSize(const QVariant &data) const;
    bool isExpired(const CacheItem &item) const;
    void updateStats(bool hit);
    void addToCategory(const QString &category, const QString &key);
    void removeFromCategory(const QString &category, const QString &key);

    QString generateQueryCacheKey(const QString &sql, const QVariantMap &parameters);

    // 序列化
    QByteArray serializeItem(const CacheItem &item) const;
    CacheItem deserializeItem(const QByteArray &data) const;
};

Q_DECLARE_METATYPE(CacheManager::CachePolicy)
Q_DECLARE_METATYPE(CacheManager::CacheItem)
Q_DECLARE_METATYPE(CacheManager::CacheStats)

#endif // CACHEMANAGER_H