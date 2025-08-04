#ifndef MULTILEVELCACHE_H
#define MULTILEVELCACHE_H

#include <QObject>
#include <QVariant>
#include <QDateTime>
#include <QTimer>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <QAtomicPointer>
#include <QJsonObject>
#include <memory>
#include <functional>
#include "../utils/LockFreeStructures.h"
#include "../core/ThreadManager.h"

Q_DECLARE_LOGGING_CATEGORY(multiLevelCache)

/**
 * @brief 缓存级别
 */
enum class CacheLevel {
    L1_Memory = 1,      // L1: 内存缓存 (最快)
    L2_Local = 2,       // L2: 本地持久化缓存
    L3_Distributed = 3  // L3: 分布式缓存 (Redis)
};

/**
 * @brief 缓存策略
 */
enum class CacheStrategy {
    LRU = 0,           // 最近最少使用
    LFU = 1,           // 最少使用频率
    ARC = 2,           // 自适应替换缓存
    CLOCK = 3,         // 时钟算法
    RANDOM = 4         // 随机替换
};

/**
 * @brief 缓存项元数据
 */
struct CacheMetadata {
    QDateTime createdAt{};
    QDateTime lastAccessed{};
    QDateTime expiresAt{};
    QAtomicInt accessCount{0};
    QAtomicInt size{0};
    QString category{};
    CacheLevel level{CacheLevel::L1_Memory};
    QAtomicInt priority{0};     // 优先级 (0-100)
    QAtomicInt hotness{0};      // 热度值
    
    CacheMetadata() 
        : createdAt(QDateTime::currentDateTime())
        , lastAccessed(QDateTime::currentDateTime())
        , level(CacheLevel::L1_Memory)
    {}
    
    bool isExpired() const {
        return expiresAt.isValid() && QDateTime::currentDateTime() > expiresAt;
    }
    
    void updateAccess() {
        lastAccessed = QDateTime::currentDateTime();
        accessCount.fetchAndAddOrdered(1);
        hotness.fetchAndAddOrdered(1);
    }
    
    double getScore(CacheStrategy strategy) const {
        switch (strategy) {
        case CacheStrategy::LRU:
            return lastAccessed.toMSecsSinceEpoch();
        case CacheStrategy::LFU:
            return accessCount.loadAcquire();
        case CacheStrategy::ARC:
            return calculateARCScore();
        default:
            return 0.0;
        }
    }
    
private:
    double calculateARCScore() const {
        // ARC算法评分：结合访问频率和最近性
        double frequency = accessCount.loadAcquire();
        double recency = QDateTime::currentDateTime().msecsTo(lastAccessed);
        return frequency * 0.7 + (1.0 / (recency + 1)) * 0.3;
    }
};

/**
 * @brief 缓存项
 */
template<typename T>
struct CacheItem {
    T data{};
    CacheMetadata metadata{};
    QAtomicInt refCount{1};
    
    explicit CacheItem(const T& value) : data(value) {}
    explicit CacheItem(T&& value) : data(std::move(value)) {}
    
    void addRef() { refCount.fetchAndAddOrdered(1); }
    void release() {
        if (refCount.fetchAndSubOrdered(1) == 1) {
            delete this;
        }
    }
};

/**
 * @brief 缓存统计信息
 */
struct CacheStatistics {
    // 基础统计
    QAtomicInt l1Hits{0};
    QAtomicInt l2Hits{0};
    QAtomicInt l3Hits{0};
    QAtomicInt misses{0};
    QAtomicInt evictions{0};
    QAtomicInt promotions{0};    // 提升到上级缓存
    QAtomicInt demotions{0};     // 降级到下级缓存
    
    // 性能统计
    QAtomicInt totalRequests{0};
    QAtomicInt averageLatency{0};  // 微秒
    QAtomicInt maxLatency{0};      // 微秒
    
    // 容量统计
    QAtomicInt l1Size{0};
    QAtomicInt l2Size{0};
    QAtomicInt l3Size{0};
    QAtomicInt l1Count{0};
    QAtomicInt l2Count{0};
    QAtomicInt l3Count{0};
    
    double getHitRate() const {
        int total = totalRequests.loadAcquire();
        if (total == 0) return 0.0;
        int hits = l1Hits.loadAcquire() + l2Hits.loadAcquire() + l3Hits.loadAcquire();
        return static_cast<double>(hits) / total;
    }
    
    double getL1HitRate() const {
        int total = totalRequests.loadAcquire();
        return total > 0 ? static_cast<double>(l1Hits.loadAcquire()) / total : 0.0;
    }
};

/**
 * @brief 高性能多级缓存系统
 * 
 * 特性：
 * - 三级缓存架构 (L1内存 + L2本地 + L3分布式)
 * - 无锁数据结构和原子操作
 * - 智能缓存策略 (LRU/LFU/ARC/CLOCK)
 * - 自动数据迁移和热点识别
 * - 异步预加载和批量操作
 * - 实时性能监控和自动调优
 */
class MultiLevelCache : public QObject
{
    Q_OBJECT

public:
    struct CacheConfig {
        // L1 内存缓存配置
        int l1MaxItems;
        qint64 l1MaxSize;
        CacheStrategy l1Strategy;
        
        // L2 本地缓存配置
        int l2MaxItems;
        qint64 l2MaxSize;
        CacheStrategy l2Strategy;
        QString l2StoragePath;
        
        // L3 分布式缓存配置
        bool l3Enabled;
        QString l3Host;
        int l3Port;
        QString l3Password;
        int l3Database;
        int l3MaxConnections;
        
        // 通用配置
        int defaultTTL;
        int cleanupInterval;
        int promotionThreshold;
        int demotionThreshold;
        bool enablePreloading;
        bool enableCompression;
        bool enableEncryption;
        
        // 性能调优
        int batchSize;
        int asyncWorkers;
        double loadFactor;
        
        // 默认构造函数
        CacheConfig() 
            : l1MaxItems(10000)
            , l1MaxSize(100 * 1024 * 1024)  // 100MB
            , l1Strategy(CacheStrategy::LRU)
            , l2MaxItems(50000)
            , l2MaxSize(500 * 1024 * 1024)  // 500MB
            , l2Strategy(CacheStrategy::LFU)
            , l2StoragePath("cache/local")
            , l3Enabled(true)
            , l3Host("localhost")
            , l3Port(6379)
            , l3Password()
            , l3Database(0)
            , l3MaxConnections(10)
            , defaultTTL(3600)              // 默认过期时间(秒)
            , cleanupInterval(300)          // 清理间隔(秒)
            , promotionThreshold(5)         // 提升阈值(访问次数)
            , demotionThreshold(100)        // 降级阈值(秒未访问)
            , enablePreloading(true)        // 启用预加载
            , enableCompression(true)       // 启用压缩
            , enableEncryption(false)       // 启用加密
            , batchSize(100)                // 批量操作大小
            , asyncWorkers(4)               // 异步工作线程数
            , loadFactor(0.75)              // 负载因子
        {}
    };

    explicit MultiLevelCache(QObject *parent = nullptr);
    ~MultiLevelCache();

    // 初始化和配置
    bool initialize(const CacheConfig& config = CacheConfig());
    void shutdown();
    bool isInitialized() const;
    
    // 基本缓存操作
    template<typename T>
    bool set(const QString& key, const T& value, int ttlSeconds = -1, 
             const QString& category = QString(), int priority = 50);
    
    template<typename T>
    std::optional<T> get(const QString& key);
    
    bool remove(const QString& key);
    bool exists(const QString& key);
    void clear();
    void clearCategory(const QString& category);
    
    // 批量操作
    template<typename T>
    bool setMultiple(const QHash<QString, T>& items, int ttlSeconds = -1, 
                    const QString& category = QString());
    
    template<typename T>
    QHash<QString, T> getMultiple(const QStringList& keys);
    
    bool removeMultiple(const QStringList& keys);
    
    // 异步操作
    template<typename T>
    std::future<bool> setAsync(const QString& key, const T& value, int ttlSeconds = -1);
    
    template<typename T>
    std::future<std::optional<T>> getAsync(const QString& key);
    
    std::future<bool> removeAsync(const QString& key);
    
    // 预加载和预热
    template<typename T>
    void preload(const QString& key, std::function<T()> loader, int ttlSeconds = -1);
    
    void warmup(const QStringList& keys);
    void warmupCategory(const QString& category);
    
    // 统计和监控
    CacheStatistics getStatistics() const;
    void resetStatistics();
    QJsonObject getMetrics() const;
    QStringList getHotKeys(int count = 10) const;
    QStringList getColdKeys(int count = 10) const;
    
    // 配置管理
    void updateConfig(const CacheConfig& config);
    CacheConfig getCurrentConfig() const;
    void enableLevel(CacheLevel level, bool enabled);
    bool isLevelEnabled(CacheLevel level) const;
    
    // 维护操作
    void compact();
    void optimize();
    void flushToDisk();
    void loadFromDisk();

signals:
    void itemCached(const QString& key, CacheLevel level);
    void itemEvicted(const QString& key, CacheLevel level);
    void itemPromoted(const QString& key, CacheLevel fromLevel, CacheLevel toLevel);
    void itemDemoted(const QString& key, CacheLevel fromLevel, CacheLevel toLevel);
    void levelOverloaded(CacheLevel level);
    void performanceAlert(const QString& message);

private slots:
    void performMaintenance();
    void checkPerformance();
    void optimizeCache();

private:
    // 缓存层实现
    class L1MemoryCache;
    class L2LocalCache;
    class L3DistributedCache;
    
    // 核心组件
    std::unique_ptr<L1MemoryCache> m_l1Cache;
    std::unique_ptr<L2LocalCache> m_l2Cache;
    std::unique_ptr<L3DistributedCache> m_l3Cache;
    
    // 配置和状态
    CacheConfig m_config;
    QAtomicInt m_initialized{0};
    
    // 统计信息
    CacheStatistics m_stats;
    
    // 线程管理
    ThreadManager* m_threadManager;
    
    // 定时器
    QTimer* m_maintenanceTimer;
    QTimer* m_performanceTimer;
    QTimer* m_optimizationTimer;
    
    // 内部方法
    template<typename T>
    bool setInternal(const QString& key, const T& value, const CacheMetadata& metadata);
    
    template<typename T>
    std::optional<T> getInternal(const QString& key, bool updateStats = true);
    
    bool removeInternal(const QString& key);
    
    void promoteItem(const QString& key, CacheLevel fromLevel, CacheLevel toLevel);
    void demoteItem(const QString& key, CacheLevel fromLevel, CacheLevel toLevel);
    
    void updateStatistics(CacheLevel level, bool hit, qint64 latency = 0);
    void checkAndPromote(const QString& key, const CacheMetadata& metadata);
    void checkAndDemote();
    
    // 序列化和压缩
    QByteArray serialize(const QVariant& data) const;
    QVariant deserialize(const QByteArray& data) const;
    QByteArray compress(const QByteArray& data) const;
    QByteArray decompress(const QByteArray& data) const;
    
    // 加密和解密
    QByteArray encrypt(const QByteArray& data) const;
    QByteArray decrypt(const QByteArray& data) const;
    
    // 工具方法
    QString generateCacheKey(const QString& key, const QString& category = QString()) const;
    CacheLevel determineBestLevel(const CacheMetadata& metadata) const;
    void logCacheEvent(const QString& event, const QString& key, CacheLevel level = CacheLevel::L1_Memory) const;

    // 热点检测
    void updateHotness(const QString& key);
    QStringList identifyHotKeys() const;
    QStringList identifyColdKeys() const;
};

// 模板方法实现
template<typename T>
bool MultiLevelCache::set(const QString& key, const T& value, int ttlSeconds,
                         const QString& category, int priority)
{
    if (!isInitialized()) {
        return false;
    }

    CacheMetadata metadata;
    metadata.category = category;
    metadata.priority.storeRelease(priority);
    metadata.size.storeRelease(sizeof(T)); // 简化的大小计算

    if (ttlSeconds > 0) {
        metadata.expiresAt = QDateTime::currentDateTime().addSecs(ttlSeconds);
    } else if (ttlSeconds == -1 && m_config.defaultTTL > 0) {
        metadata.expiresAt = QDateTime::currentDateTime().addSecs(m_config.defaultTTL);
    }

    return setInternal(key, QVariant::fromValue(value), metadata);
}

template<typename T>
std::optional<T> MultiLevelCache::get(const QString& key)
{
    if (!isInitialized()) {
        return std::nullopt;
    }

    auto result = getInternal<QVariant>(key);
    if (result.has_value()) {
        if (result->canConvert<T>()) {
            return result->value<T>();
        }
    }

    return std::nullopt;
}

template<typename T>
bool MultiLevelCache::setMultiple(const QHash<QString, T>& items, int ttlSeconds,
                                 const QString& category)
{
    bool allSuccess = true;
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (!set(it.key(), it.value(), ttlSeconds, category)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

template<typename T>
QHash<QString, T> MultiLevelCache::getMultiple(const QStringList& keys)
{
    QHash<QString, T> result;
    for (const QString& key : keys) {
        auto value = get<T>(key);
        if (value.has_value()) {
            result[key] = value.value();
        }
    }
    return result;
}

template<typename T>
std::future<bool> MultiLevelCache::setAsync(const QString& key, const T& value, int ttlSeconds)
{
    return std::async(std::launch::async, [this, key, value, ttlSeconds]() {
        return set(key, value, ttlSeconds);
    });
}

template<typename T>
std::future<std::optional<T>> MultiLevelCache::getAsync(const QString& key)
{
    return std::async(std::launch::async, [this, key]() {
        return get<T>(key);
    });
}

template<typename T>
void MultiLevelCache::preload(const QString& key, std::function<T()> loader, int ttlSeconds)
{
    if (m_threadManager) {
        m_threadManager->submitServiceTask([this, key, loader, ttlSeconds]() {
            try {
                T data = loader();
                set(key, data, ttlSeconds);
                logCacheEvent("PRELOADED", key);
            } catch (const std::exception& e) {
                qCWarning(multiLevelCache) << "Preload failed for key" << key << ":" << e.what();
            }
        });
    }
}

#endif // MULTILEVELCACHE_H
