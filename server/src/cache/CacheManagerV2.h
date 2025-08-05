#ifndef CACHEMANAGERV2_H
#define CACHEMANAGERV2_H

#include <QObject>
#include <QVariant>
#include <QDateTime>
#include <QTimer>
#include <QLoggingCategory>
#include <QJsonObject>
#include <QAtomicInt>
#include <memory>
#include <future>
#include "MultiLevelCache.h"
#include "CacheStrategyManager.h"
#include "CachePreloader.h"

Q_DECLARE_LOGGING_CATEGORY(cacheManagerV2)

/**
 * @brief 重构后的高性能缓存管理器 V2
 * 
 * 新特性：
 * - 集成多级缓存系统 (L1内存 + L2本地 + L3分布式)
 * - 智能缓存策略管理
 * - 自动预加载和预取
 * - 实时性能监控和优化
 * - 向后兼容的API接口
 */
class CacheManagerV2 : public QObject
{
    Q_OBJECT

public:
    // 兼容的缓存策略枚举
    enum CachePolicy {
        NoCache = 0,
        LRU = 1,
        LFU = 2,
        FIFO = 3,
        TTL = 4
    };
    Q_ENUM(CachePolicy)

    // 兼容的缓存统计结构
    struct CacheStats {
        qint64 hitCount{0};
        qint64 missCount{0};
        qint64 totalRequests{0};
        double hitRate{0.0};
        qint64 totalSize{0};
        int itemCount{0};
        QDateTime lastClearTime{};
        
        CacheStats() : hitCount(0), missCount(0), totalRequests(0), hitRate(0.0), 
                      totalSize(0), itemCount(0), lastClearTime(QDateTime::currentDateTime()) {}
    };

    struct CacheConfig {
        // 多级缓存配置
        MultiLevelCache::CacheConfig multiLevelConfig;
        
        // 策略管理配置
        StrategyConfig strategyConfig;
        
        // 预加载配置
        CachePreloader::PreloaderConfig preloaderConfig;
        
        // 兼容性配置
        bool enableLegacyAPI;
        bool enableAdvancedFeatures;
        bool enableAutoOptimization;
        
        // 监控配置
        bool enableMetrics;
        int metricsInterval;
        bool enableAlerts;
        
        // 默认构造函数
        CacheConfig()
            : multiLevelConfig()
            , strategyConfig()
            , preloaderConfig()
            , enableLegacyAPI(true)
            , enableAdvancedFeatures(true)
            , enableAutoOptimization(true)
            , enableMetrics(true)
            , metricsInterval(30000)
            , enableAlerts(true)
        {}
    };

    // 单例模式
    static CacheManagerV2* instance();
    
    explicit CacheManagerV2(QObject *parent = nullptr);
    ~CacheManagerV2();

    // 初始化和配置
    bool initialize(const CacheConfig& config = CacheConfig());
    void shutdown();
    bool isInitialized() const;

    // === 兼容的基本API (保持向后兼容) ===
    
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

    // 兼容的统计和配置API
    CacheStats getStats() const;
    double getHitRate() const;
    qint64 getTotalSize() const;
    int getItemCount() const;
    QStringList getKeys() const;
    QStringList getCategories() const;
    void setDefaultPolicy(CachePolicy policy);
    void setMaxSize(qint64 maxSizeBytes);
    void setMaxItems(int maxItems);
    void setDefaultTTL(int seconds);
    void setCleanupInterval(int seconds);

    // === 新的高级API ===
    
    // 类型安全的缓存操作
    template<typename T>
    bool setTyped(const QString& key, const T& value, int ttlSeconds = -1, 
                 const QString& category = QString(), int priority = 50);
    
    template<typename T>
    std::optional<T> getTyped(const QString& key);
    
    // 异步操作
    std::future<bool> setAsync(const QString& key, const QVariant& value, int ttlSeconds = -1);
    std::future<QVariant> getAsync(const QString& key);
    std::future<bool> removeAsync(const QString& key);
    
    // 预加载和预取
    template<typename T>
    void preload(const QString& key, std::function<T()> loader, int ttlSeconds = -1);
    
    void warmup(const QStringList& keys);
    void warmupCategory(const QString& category);
    
    // 智能缓存
    QStringList predictNextAccess(int count = 10) const;
    QStringList recommendPrefetch(const QString& key, int count = 5) const;
    void enableAdaptiveOptimization(bool enabled);
    
    // 高级统计
    QJsonObject getDetailedMetrics() const;
    QJsonObject getPerformanceReport() const;
    QStringList getHotKeys(int count = 10) const;
    QStringList getColdKeys(int count = 10) const;
    
    // 缓存级别控制
    void enableLevel(CacheLevel level, bool enabled);
    bool isLevelEnabled(CacheLevel level) const;
    CacheStatistics getLevelStatistics(CacheLevel level) const;
    
    // 策略管理
    void setStrategy(CacheStrategy strategy, CacheLevel level = CacheLevel::L1_Memory);
    CacheStrategy getStrategy(CacheLevel level = CacheLevel::L1_Memory) const;
    void updateStrategyConfig(const StrategyConfig& config);
    
    // 维护操作
    void cleanup();
    void compactMemory();
    void optimize();
    void flushToDisk();
    void loadFromDisk();
    
    // 配置管理
    void updateConfig(const CacheConfig& config);
    CacheConfig getCurrentConfig() const;

signals:
    // 兼容信号
    void itemAdded(const QString &key, const QString &category);
    void itemRemoved(const QString &key, const QString &category);
    void itemExpired(const QString &key, const QString &category);
    void categoryCleared(const QString &category);
    void cacheCleared();
    void memoryLimitExceeded(qint64 currentSize, qint64 limit);
    
    // 新增信号
    void itemCached(const QString& key, CacheLevel level);
    void itemEvicted(const QString& key, CacheLevel level);
    void itemPromoted(const QString& key, CacheLevel fromLevel, CacheLevel toLevel);
    void levelOverloaded(CacheLevel level);
    void optimizationCompleted(const QJsonObject& results);
    void performanceAlert(const QString& message);

private slots:
    void performCleanup();
    void onCacheEvent();
    void onOptimizationCompleted(const QJsonObject& results);
    void onPerformanceAlert(const QString& message);

private:
    // 核心组件
    std::unique_ptr<MultiLevelCache> m_multiLevelCache;
    std::unique_ptr<CacheStrategyManager> m_strategyManager;
    std::unique_ptr<CachePreloader> m_preloader;
    
    // 配置和状态
    CacheConfig m_config;
    QAtomicInt m_initialized{0};
    
    // 兼容性支持
    QTimer *m_cleanupTimer;
    CacheStats m_legacyStats;
    QAtomicInt m_defaultTTL{3600};
    
    // 分类管理
    QHash<QString, QStringList> m_categories;
    mutable QMutex m_categoriesMutex;
    
    // 内部方法
    void setupConnections();
    void migrateFromLegacyCache();
    
    // 兼容性方法
    void updateLegacyStats(bool hit);
    QString generateUserKey(qint64 userId) const;
    QString generateGroupKey(qint64 groupId) const;
    QString generateMessageKey(qint64 chatId) const;
    QString generateSessionKey(const QString &sessionToken) const;
    QString generateQueryCacheKey(const QString &sql, const QVariantMap &parameters) const;
    
    // 分类管理
    void addToCategory(const QString &category, const QString &key);
    void removeFromCategory(const QString &category, const QString &key);
    
    // 工具方法
    void logCacheManagerEvent(const QString& event, const QString& details = QString()) const;
};

// 模板方法实现
template<typename T>
bool CacheManagerV2::setTyped(const QString& key, const T& value, int ttlSeconds, 
                             const QString& category, int priority)
{
    if (!isInitialized() || !m_multiLevelCache) {
        return false;
    }
    
    bool success = m_multiLevelCache->set(key, value, ttlSeconds, category, priority);
    if (success && !category.isEmpty()) {
        addToCategory(category, key);
    }
    return success;
}

template<typename T>
std::optional<T> CacheManagerV2::getTyped(const QString& key)
{
    if (!isInitialized() || !m_multiLevelCache) {
        return std::nullopt;
    }
    
    auto result = m_multiLevelCache->get<T>(key);
    updateLegacyStats(result.has_value());
    return result;
}

template<typename T>
void CacheManagerV2::preload(const QString& key, std::function<T()> loader, int ttlSeconds)
{
    if (isInitialized() && m_multiLevelCache) {
        m_multiLevelCache->preload(key, loader, ttlSeconds);
    }
}

Q_DECLARE_METATYPE(CacheManagerV2::CachePolicy)
Q_DECLARE_METATYPE(CacheManagerV2::CacheStats)

#endif // CACHEMANAGERV2_H
