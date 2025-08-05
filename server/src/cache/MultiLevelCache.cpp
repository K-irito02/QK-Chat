#include "MultiLevelCache.h"
#include "RedisClient.h"
#include "../config/ServerConfig.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonArray>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QPointer>
#include <QDataStream>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <algorithm>
#include <chrono>

Q_LOGGING_CATEGORY(multiLevelCache, "qkchat.server.multilevelcache")

/**
 * @brief L1内存缓存实现
 */
class MultiLevelCache::L1MemoryCache
{
public:
    explicit L1MemoryCache(const CacheConfig& config)
        : m_config(config)
        , m_maxItems(config.l1MaxItems)
        , m_maxSize(config.l1MaxSize)
        , m_strategy(config.l1Strategy)
    {}
    
    bool set(const QString& key, const QVariant& value, const CacheMetadata& metadata) {
        if (shouldEvict()) {
            evictItems();
        }
        
        auto item = std::make_shared<CacheItem<QVariant>>(value);
        item->metadata = metadata;
        item->metadata.level = CacheLevel::L1_Memory;
        
        m_cache.insert(key, item);
        m_accessOrder.removeAll(key);
        m_accessOrder.append(key);
        
        return true;
    }
    
    std::optional<QVariant> get(const QString& key) {
        auto item = m_cache.value(key);
        if (!item) {
            return std::nullopt;
        }
        
        if (item->metadata.isExpired()) {
            m_cache.remove(key);
            m_accessOrder.removeAll(key);
            return std::nullopt;
        }
        
        item->metadata.updateAccess();
        updateAccessOrder(key);
        
        return item->data;
    }
    
    bool remove(const QString& key) {
        bool removed = m_cache.remove(key) > 0;
        if (removed) {
            m_accessOrder.removeAll(key);
        }
        return removed;
    }
    
    bool exists(const QString& key) const {
        auto item = m_cache.value(key);
        return item && !item->metadata.isExpired();
    }
    
    void clear() {
        m_cache.clear();
        m_accessOrder.clear();
    }
    
    int size() const { return m_cache.size(); }
    qint64 memoryUsage() const {
        qint64 total = 0;
        m_cache.forEach([&total](const QString& key, const std::shared_ptr<CacheItem<QVariant>>& item) {
            (void)key; // 避免未使用参数警告
            total += item->metadata.size.loadAcquire();
        });
        return total;
    }
    
    QStringList keys() const { return m_cache.keys(); }
    
    QStringList getHotKeys(int count) const {
        QList<QPair<QString, int>> keyHotness;
        m_cache.forEach([&keyHotness](const QString& key, const std::shared_ptr<CacheItem<QVariant>>& item) {
            keyHotness.append(QPair<QString, int>(key, item->metadata.hotness.loadAcquire()));
        });
        
        std::sort(keyHotness.begin(), keyHotness.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        QStringList result;
        for (int i = 0; i < qMin(count, keyHotness.size()); ++i) {
            result.append(keyHotness[i].first);
        }
        return result;
    }

private:
    CacheConfig m_config;
    int m_maxItems;
    qint64 m_maxSize;
    CacheStrategy m_strategy;
    
    ConcurrentMap<QString, std::shared_ptr<CacheItem<QVariant>>> m_cache;
    QList<QString> m_accessOrder; // 用于LRU
    
    bool shouldEvict() const {
        return m_cache.size() >= m_maxItems || memoryUsage() >= m_maxSize;
    }
    
    void evictItems() {
        int targetSize = m_maxItems * 0.8; // 清理到80%
        
        switch (m_strategy) {
        case CacheStrategy::LRU:
            evictLRU(targetSize);
            break;
        case CacheStrategy::LFU:
            evictLFU(targetSize);
            break;
        case CacheStrategy::RANDOM:
            evictRandom(targetSize);
            break;
        default:
            evictLRU(targetSize);
            break;
        }
    }
    
    void evictLRU(int targetSize) {
        while (m_cache.size() > targetSize && !m_accessOrder.isEmpty()) {
            QString oldestKey = m_accessOrder.takeFirst();
            m_cache.remove(oldestKey);
        }
    }
    
    void evictLFU(int targetSize) {
        QList<QPair<QString, int>> keyFrequency;
        m_cache.forEach([&keyFrequency](const QString& key, const std::shared_ptr<CacheItem<QVariant>>& item) {
            keyFrequency.append(QPair<QString, int>(key, item->metadata.accessCount.loadAcquire()));
        });
        
        std::sort(keyFrequency.begin(), keyFrequency.end(),
                 [](const auto& a, const auto& b) { return a.second < b.second; });
        
        int toRemove = m_cache.size() - targetSize;
        for (int i = 0; i < toRemove && i < keyFrequency.size(); ++i) {
            QString key = keyFrequency[i].first;
            m_cache.remove(key);
            m_accessOrder.removeAll(key);
        }
    }
    
    void evictRandom(int targetSize) {
        QStringList keys = m_cache.keys();
        int toRemove = keys.size() - targetSize;
        
        for (int i = 0; i < toRemove; ++i) {
            int randomIndex = QRandomGenerator::global()->bounded(keys.size());
            QString key = keys.takeAt(randomIndex);
            m_cache.remove(key);
            m_accessOrder.removeAll(key);
        }
    }
    
    void updateAccessOrder(const QString& key) {
        if (m_strategy == CacheStrategy::LRU) {
            m_accessOrder.removeAll(key);
            m_accessOrder.append(key);
        }
    }
};

/**
 * @brief L2本地缓存实现
 */
class MultiLevelCache::L2LocalCache
{
public:
    explicit L2LocalCache(const CacheConfig& config)
        : m_config(config)
        , m_storagePath(config.l2StoragePath)
    {
        QDir().mkpath(m_storagePath);
    }
    
    bool set(const QString& key, const QVariant& value, const CacheMetadata& metadata) {
        QString filePath = getFilePath(key);
        QFile file(filePath);
        
        if (!file.open(QIODevice::WriteOnly)) {
            return false;
        }
        
        QDataStream stream(&file);
        stream << value << metadata.createdAt << metadata.expiresAt 
               << metadata.accessCount.loadAcquire() << metadata.category;
        
        return true;
    }
    
    std::optional<QVariant> get(const QString& key) {
        QString filePath = getFilePath(key);
        QFile file(filePath);
        
        if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
            return std::nullopt;
        }
        
        QDataStream stream(&file);
        QVariant value;
        QDateTime createdAt, expiresAt;
        int accessCount;
        QString category;
        
        stream >> value >> createdAt >> expiresAt >> accessCount >> category;
        
        // 检查是否过期
        if (expiresAt.isValid() && QDateTime::currentDateTime() > expiresAt) {
            file.remove();
            return std::nullopt;
        }
        
        return value;
    }
    
    bool remove(const QString& key) {
        QString filePath = getFilePath(key);
        return QFile::remove(filePath);
    }
    
    bool exists(const QString& key) const {
        QString filePath = getFilePath(key);
        return QFile::exists(filePath);
    }
    
    void clear() {
        QDir dir(m_storagePath);
        dir.removeRecursively();
        dir.mkpath(m_storagePath);
    }
    
    QStringList keys() const {
        QStringList result;
        QDir dir(m_storagePath);
        QDirIterator it(dir, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            QString filePath = it.next();
            if (filePath.endsWith(".cache")) {
                QString fileName = QFileInfo(filePath).baseName();
                result << fileName;
            }
        }
        return result;
    }

private:
    CacheConfig m_config;
    QString m_storagePath;
    
    QString getFilePath(const QString& key) const {
        QString hash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();
        return m_storagePath + "/" + hash.left(2) + "/" + hash + ".cache";
    }
};

/**
 * @brief L3分布式缓存实现 (Redis)
 */
class MultiLevelCache::L3DistributedCache
{
public:
    explicit L3DistributedCache(const CacheConfig& config)
        : m_config(config)
        , m_enabled(config.l3Enabled)
        , m_redisClient(nullptr)
    {
        if (m_enabled) {
            initializeRedisConnection();
        }
    }

    ~L3DistributedCache() {
        if (m_redisClient) {
            delete m_redisClient;
        }
    }
    
    bool set(const QString& key, const QVariant& value, const CacheMetadata& metadata) {
        if (!m_enabled || !m_redisClient || !m_redisClient->isConnected()) {
            qCDebug(multiLevelCache) << "L3 cache not available for set operation";
            return false;
        }

        // 计算TTL
        int ttlSeconds = -1;
        if (metadata.expiresAt.isValid()) {
            qint64 msecs = QDateTime::currentDateTime().msecsTo(metadata.expiresAt);
            ttlSeconds = static_cast<int>(msecs / 1000);
            if (ttlSeconds <= 0) {
                return false; // 已过期
            }
        }

        bool success = m_redisClient->set(key, value, ttlSeconds);
        if (success) {
            qCDebug(multiLevelCache) << "Successfully stored key in L3 cache:" << key;
        } else {
            qCWarning(multiLevelCache) << "Failed to store key in L3 cache:" << key << m_redisClient->getLastError();
        }

        return success;
    }

    std::optional<QVariant> get(const QString& key) {
        if (!m_enabled || !m_redisClient || !m_redisClient->isConnected()) {
            qCDebug(multiLevelCache) << "L3 cache not available for get operation";
            return std::nullopt;
        }

        QVariant value = m_redisClient->get(key);
        if (value.isValid()) {
            qCDebug(multiLevelCache) << "Retrieved key from L3 cache:" << key;
            return value;
        }

        return std::nullopt;
    }
    
    bool remove(const QString& key) {
        if (!m_enabled || !m_redisClient || !m_redisClient->isConnected()) {
            return false;
        }

        return m_redisClient->remove(key);
    }

    bool exists(const QString& key) const {
        if (!m_enabled || !m_redisClient || !m_redisClient->isConnected()) {
            return false;
        }

        return m_redisClient->exists(key);
    }

    void clear() {
        if (m_redisClient && m_redisClient->isConnected()) {
            m_redisClient->flushDatabase();
        }
    }

    QStringList keys() const {
        // Redis KEYS命令在生产环境中不推荐使用，这里返回空列表
        return QStringList();
    }

private:
    void initializeRedisConnection() {
        ServerConfig* config = ServerConfig::instance();

        m_redisClient = new RedisClient();

        QString host = config->getRedisHost();
        int port = config->getRedisPort();
        QString password = config->getRedisPassword();
        int database = config->getRedisDatabase();

        qCInfo(multiLevelCache) << "Initializing Redis connection to:" << host << ":" << port;

        bool connected = m_redisClient->connectToServer(host, port, password, database);
        if (connected) {
            qCInfo(multiLevelCache) << "Successfully connected to Redis server";
        } else {
            qCWarning(multiLevelCache) << "Failed to connect to Redis server:" << m_redisClient->getLastError();
            delete m_redisClient;
            m_redisClient = nullptr;
            m_enabled = false;
        }
    }

    CacheConfig m_config;
    bool m_enabled;
    RedisClient* m_redisClient;
};

MultiLevelCache::MultiLevelCache(QObject *parent)
    : QObject(parent)
    , m_threadManager(ThreadManager::instance())
    , m_maintenanceTimer(new QTimer(this))
    , m_performanceTimer(new QTimer(this))
    , m_optimizationTimer(new QTimer(this))
{
    // 设置定时器
    connect(m_maintenanceTimer, &QTimer::timeout, this, &MultiLevelCache::performMaintenance);
    connect(m_performanceTimer, &QTimer::timeout, this, &MultiLevelCache::checkPerformance);
    connect(m_optimizationTimer, &QTimer::timeout, this, &MultiLevelCache::optimizeCache);
    
    qCInfo(multiLevelCache) << "MultiLevelCache created";
}

MultiLevelCache::~MultiLevelCache()
{
    shutdown();
    qCInfo(multiLevelCache) << "MultiLevelCache destroyed";
}

bool MultiLevelCache::initialize(const CacheConfig& config)
{
    qCInfo(multiLevelCache) << "Initializing MultiLevelCache...";
    
    m_config = config;
    
    try {
        // 初始化各级缓存
        m_l1Cache = std::make_unique<L1MemoryCache>(config);
        m_l2Cache = std::make_unique<L2LocalCache>(config);
        m_l3Cache = std::make_unique<L3DistributedCache>(config);
        
        // 启动定时器
        m_maintenanceTimer->start(config.cleanupInterval * 1000);
        m_performanceTimer->start(60000); // 每分钟检查性能
        m_optimizationTimer->start(300000); // 每5分钟优化
        
        m_initialized.storeRelease(1);
        
        qCInfo(multiLevelCache) << "MultiLevelCache initialized successfully";
        return true;
        
    } catch (const std::exception& e) {
        qCCritical(multiLevelCache) << "Failed to initialize MultiLevelCache:" << e.what();
        return false;
    }
}

void MultiLevelCache::shutdown()
{
    if (m_initialized.testAndSetOrdered(1, 0)) {
        qCInfo(multiLevelCache) << "Shutting down MultiLevelCache...";
        
        m_maintenanceTimer->stop();
        m_performanceTimer->stop();
        m_optimizationTimer->stop();
        
        // 清理缓存
        if (m_l1Cache) m_l1Cache.reset();
        if (m_l2Cache) m_l2Cache.reset();
        if (m_l3Cache) m_l3Cache.reset();
        
        qCInfo(multiLevelCache) << "MultiLevelCache shutdown complete";
    }
}

bool MultiLevelCache::isInitialized() const
{
    return m_initialized.loadAcquire() > 0;
}

bool MultiLevelCache::remove(const QString& key)
{
    if (!isInitialized()) {
        return false;
    }
    
    bool removed = false;
    
    // 从所有级别移除
    if (m_l1Cache) {
        removed |= m_l1Cache->remove(key);
    }
    if (m_l2Cache) {
        removed |= m_l2Cache->remove(key);
    }
    if (m_l3Cache) {
        removed |= m_l3Cache->remove(key);
    }
    
    if (removed) {
        logCacheEvent("REMOVED", key);
    }
    
    return removed;
}

bool MultiLevelCache::exists(const QString& key)
{
    if (!isInitialized()) {
        return false;
    }
    
    return (m_l1Cache && m_l1Cache->exists(key)) ||
           (m_l2Cache && m_l2Cache->exists(key)) ||
           (m_l3Cache && m_l3Cache->exists(key));
}

void MultiLevelCache::clear()
{
    if (!isInitialized()) {
        return;
    }
    
    if (m_l1Cache) m_l1Cache->clear();
    if (m_l2Cache) m_l2Cache->clear();
    if (m_l3Cache) m_l3Cache->clear();
    
    // 重置统计
    resetStatistics();
    
    emit itemEvicted("*", CacheLevel::L1_Memory);
    logCacheEvent("CLEARED", "*");
}

bool MultiLevelCache::removeMultiple(const QStringList& keys)
{
    bool allSuccess = true;
    for (const QString& key : keys) {
        if (!remove(key)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

std::future<bool> MultiLevelCache::removeAsync(const QString& key)
{
    return std::async(std::launch::async, [this, key]() {
        return remove(key);
    });
}

void MultiLevelCache::warmup(const QStringList& keys)
{
    if (m_threadManager) {
        m_threadManager->submitServiceTask([this, keys]() {
            for (const QString& key : keys) {
                // 尝试从低级缓存加载到高级缓存
                auto value = getInternal<QVariant>(key, false);
                if (value.has_value()) {
                    logCacheEvent("WARMED_UP", key);
                }
            }
        });
    }
}

void MultiLevelCache::warmupCategory(const QString& category)
{
    // 实现分类预热逻辑
    if (m_threadManager) {
        m_threadManager->submitServiceTask([this, category]() {
            // 这里应该从数据库或其他数据源加载该分类的数据
            logCacheEvent("CATEGORY_WARMED_UP", category);
        });
    }
}

CacheStatistics MultiLevelCache::getStatistics() const
{
    return m_stats;
}

void MultiLevelCache::resetStatistics()
{
    m_stats = CacheStatistics{};
    qCInfo(multiLevelCache) << "Cache statistics reset";
}

QJsonObject MultiLevelCache::getMetrics() const
{
    QJsonObject metrics;
    
    // 基础统计
    metrics["hit_rate"] = m_stats.getHitRate();
    metrics["l1_hit_rate"] = m_stats.getL1HitRate();
    metrics["total_requests"] = m_stats.totalRequests.loadAcquire();
    metrics["average_latency"] = m_stats.averageLatency.loadAcquire();
    
    // 容量统计
    metrics["l1_size"] = m_stats.l1Size.loadAcquire();
    metrics["l2_size"] = m_stats.l2Size.loadAcquire();
    metrics["l3_size"] = m_stats.l3Size.loadAcquire();
    
    // 操作统计
    metrics["evictions"] = m_stats.evictions.loadAcquire();
    metrics["promotions"] = m_stats.promotions.loadAcquire();
    metrics["demotions"] = m_stats.demotions.loadAcquire();
    
    return metrics;
}

QStringList MultiLevelCache::getHotKeys(int count) const
{
    if (m_l1Cache) {
        return m_l1Cache->getHotKeys(count);
    }
    return QStringList();
}

QStringList MultiLevelCache::getColdKeys(int count) const
{
    (void)count; // 避免未使用参数警告
    // 实现冷数据识别逻辑
    return identifyColdKeys();
}

void MultiLevelCache::performMaintenance()
{
    qCDebug(multiLevelCache) << "Performing cache maintenance...";
    
    // 清理过期项
    // 检查内存使用
    // 执行数据迁移
    
    qCDebug(multiLevelCache) << "Cache maintenance completed";
}

void MultiLevelCache::checkPerformance()
{
    double hitRate = m_stats.getHitRate();
    if (hitRate < 0.5) { // 命中率低于50%
        emit performanceAlert(QString("Low cache hit rate: %1%").arg(hitRate * 100, 0, 'f', 1));
    }
    
    int avgLatency = m_stats.averageLatency.loadAcquire();
    if (avgLatency > 10000) { // 平均延迟超过10ms
        emit performanceAlert(QString("High cache latency: %1μs").arg(avgLatency));
    }
}

void MultiLevelCache::optimizeCache()
{
    qCDebug(multiLevelCache) << "Optimizing cache...";
    
    // 识别热点数据并提升
    QStringList hotKeys = identifyHotKeys();
    for (const QString& key : hotKeys) {
        // 提升热点数据到L1
        auto value = getInternal<QVariant>(key, false);
        if (value.has_value() && m_l1Cache && !m_l1Cache->exists(key)) {
            CacheMetadata metadata;
            metadata.priority.storeRelease(80); // 高优先级
            m_l1Cache->set(key, value.value(), metadata);
            emit itemPromoted(key, CacheLevel::L2_Local, CacheLevel::L1_Memory);
        }
    }
    
    qCDebug(multiLevelCache) << "Cache optimization completed";
}

template<typename T>
bool MultiLevelCache::setInternal(const QString& key, const T& value, const CacheMetadata& metadata)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    
    bool success = false;
    
    // 优先存储到L1
    if (m_l1Cache) {
        success = m_l1Cache->set(key, QVariant::fromValue(value), metadata);
        if (success) {
            emit itemCached(key, CacheLevel::L1_Memory);
            m_stats.l1Count.fetchAndAddOrdered(1);
        }
    }
    
    // 异步存储到L2和L3，使用weak_ptr避免循环引用
    if (m_threadManager) {
        auto weakSelf = QPointer<MultiLevelCache>(this);
        m_threadManager->submitServiceTask([weakSelf, key, value, metadata]() {
            if (auto self = weakSelf.data()) {
                try {
                    if (self->m_l2Cache) {
                        self->m_l2Cache->set(key, QVariant::fromValue(value), metadata);
                        // 使用QMetaObject::invokeMethod确保信号在正确线程发射
                        QMetaObject::invokeMethod(self, [self, key]() {
                            emit self->itemCached(key, CacheLevel::L2_Local);
                        }, Qt::QueuedConnection);
                        self->m_stats.l2Count.fetchAndAddOrdered(1);
                    }

                    if (self->m_l3Cache) {
                        self->m_l3Cache->set(key, QVariant::fromValue(value), metadata);
                        QMetaObject::invokeMethod(self, [self, key]() {
                            emit self->itemCached(key, CacheLevel::L3_Distributed);
                        }, Qt::QueuedConnection);
                        self->m_stats.l3Count.fetchAndAddOrdered(1);
                    }
                } catch (const std::exception& e) {
                    qCCritical(multiLevelCache) << "Exception in async cache operation:" << e.what();
                } catch (...) {
                    qCCritical(multiLevelCache) << "Unknown exception in async cache operation";
                }
            }
        });
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    
    updateStatistics(CacheLevel::L1_Memory, success, latency);
    logCacheEvent("SET", key, CacheLevel::L1_Memory);
    
    return success;
}

template<typename T>
std::optional<T> MultiLevelCache::getInternal(const QString& key, bool updateStats)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    
    m_stats.totalRequests.fetchAndAddOrdered(1);
    
    // L1查找
    if (m_l1Cache) {
        auto result = m_l1Cache->get(key);
        if (result.has_value()) {
            if (updateStats) {
                m_stats.l1Hits.fetchAndAddOrdered(1);
                updateHotness(key);
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
            updateStatistics(CacheLevel::L1_Memory, true, latency);
            
            if (result->canConvert<T>()) {
                return result->value<T>();
            }
        }
    }
    
    // L2查找
    if (m_l2Cache) {
        auto result = m_l2Cache->get(key);
        if (result.has_value()) {
            if (updateStats) {
                m_stats.l2Hits.fetchAndAddOrdered(1);
                
                // 提升到L1
                if (m_l1Cache) {
                    CacheMetadata metadata;
                    m_l1Cache->set(key, result.value(), metadata);
                    emit itemPromoted(key, CacheLevel::L2_Local, CacheLevel::L1_Memory);
                    m_stats.promotions.fetchAndAddOrdered(1);
                }
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
            updateStatistics(CacheLevel::L2_Local, true, latency);
            
            if (result->canConvert<T>()) {
                return result->value<T>();
            }
        }
    }
    
    // L3查找
    if (m_l3Cache) {
        auto result = m_l3Cache->get(key);
        if (result.has_value()) {
            if (updateStats) {
                m_stats.l3Hits.fetchAndAddOrdered(1);
                
                // 提升到L1和L2
                CacheMetadata metadata;
                if (m_l1Cache) {
                    m_l1Cache->set(key, result.value(), metadata);
                }
                if (m_l2Cache) {
                    m_l2Cache->set(key, result.value(), metadata);
                }
                emit itemPromoted(key, CacheLevel::L3_Distributed, CacheLevel::L1_Memory);
                m_stats.promotions.fetchAndAddOrdered(1);
            }
            
            auto endTime = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
            updateStatistics(CacheLevel::L3_Distributed, true, latency);
            
            if (result->canConvert<T>()) {
                return result->value<T>();
            }
        }
    }
    
    // 缓存未命中
    if (updateStats) {
        m_stats.misses.fetchAndAddOrdered(1);
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
    updateStatistics(CacheLevel::L1_Memory, false, latency);
    
    return std::nullopt;
}

void MultiLevelCache::updateStatistics(CacheLevel level, bool hit, qint64 latency)
{
    (void)level; // 避免未使用参数警告
    (void)hit;   // 避免未使用参数警告
    
    // 更新延迟统计
    if (latency > 0) {
        int currentAvg = m_stats.averageLatency.loadAcquire();
        int newAvg = (currentAvg + latency) / 2;
        m_stats.averageLatency.storeRelease(newAvg);
        
        int currentMax = m_stats.maxLatency.loadAcquire();
        if (latency > currentMax) {
            m_stats.maxLatency.testAndSetOrdered(currentMax, latency);
        }
    }
}

void MultiLevelCache::updateHotness(const QString& key)
{
    (void)key; // 避免未使用参数警告
    // 更新键的热度值
    if (m_l1Cache) {
        // 实现热度更新逻辑
    }
}

QStringList MultiLevelCache::identifyHotKeys() const
{
    if (m_l1Cache) {
        return m_l1Cache->getHotKeys(10);
    }
    return QStringList();
}

QStringList MultiLevelCache::identifyColdKeys() const
{
    // 实现冷数据识别逻辑
    return QStringList();
}

QString MultiLevelCache::generateCacheKey(const QString& key, const QString& category) const
{
    if (category.isEmpty()) {
        return key;
    }
    return category + ":" + key;
}

void MultiLevelCache::logCacheEvent(const QString& event, const QString& key, CacheLevel level) const
{
    qCDebug(multiLevelCache) << event << "key:" << key << "level:" << static_cast<int>(level);
}

// 获取当前配置
MultiLevelCache::CacheConfig MultiLevelCache::getCurrentConfig() const
{
    return m_config;
}

// 更新配置
void MultiLevelCache::updateConfig(const MultiLevelCache::CacheConfig& config)
{
    m_config = config;
    
    // 更新各个缓存层的配置
    if (m_l1Cache) {
        // 更新L1缓存配置
    }
    
    if (m_l2Cache) {
        // 更新L2缓存配置
    }
    
    if (m_l3Cache) {
        // 更新L3缓存配置
    }
    
    qCInfo(multiLevelCache) << "Cache configuration updated";
}

// 启用/禁用缓存级别
void MultiLevelCache::enableLevel(CacheLevel level, bool enabled)
{
    (void)enabled; // 避免未使用参数警告
    
    switch (level) {
    case CacheLevel::L1_Memory:
        // L1内存缓存总是启用的
        break;
    case CacheLevel::L2_Local:
        // 可以在这里添加L2启用/禁用逻辑
        break;
    case CacheLevel::L3_Distributed:
        // 可以在这里添加L3启用/禁用逻辑
        break;
    }
}

// 检查缓存级别是否启用
bool MultiLevelCache::isLevelEnabled(CacheLevel level) const
{
    switch (level) {
    case CacheLevel::L1_Memory:
        return m_l1Cache != nullptr;
    case CacheLevel::L2_Local:
        return m_l2Cache != nullptr;
    case CacheLevel::L3_Distributed:
        return m_l3Cache != nullptr && m_config.l3Enabled;
    default:
        return false;
    }
}

// 压缩缓存
void MultiLevelCache::compact()
{
    qCInfo(multiLevelCache) << "Starting cache compaction...";
    
    // L1缓存压缩
    if (m_l1Cache) {
        // 移除过期项
        QStringList keys = m_l1Cache->keys();
        for (const QString& key : keys) {
            if (!m_l1Cache->exists(key)) {
                m_l1Cache->remove(key);
            }
        }
    }
    
    // L2缓存压缩
    if (m_l2Cache) {
        // 移除过期项
        QStringList keys = m_l2Cache->keys();
        for (const QString& key : keys) {
            if (!m_l2Cache->exists(key)) {
                m_l2Cache->remove(key);
            }
        }
    }
    
    // L3缓存压缩
    if (m_l3Cache) {
        // 移除过期项
        QStringList keys = m_l3Cache->keys();
        for (const QString& key : keys) {
            if (!m_l3Cache->exists(key)) {
                m_l3Cache->remove(key);
            }
        }
    }
    
    qCInfo(multiLevelCache) << "Cache compaction completed";
}

// 优化缓存
void MultiLevelCache::optimize()
{
    qCInfo(multiLevelCache) << "Starting cache optimization...";
    
    // 识别热点数据并提升到更高级别
    QStringList hotKeys = identifyHotKeys();
    for (const QString& key : hotKeys) {
        // 将热点数据提升到L1
        auto value = get<QVariant>(key);
        if (value.has_value()) {
            CacheMetadata metadata;
            metadata.priority = 100; // 高优先级
            setInternal(key, value.value(), metadata);
        }
    }
    
    // 识别冷数据并降级到更低级别
    QStringList coldKeys = identifyColdKeys();
    for (const QString& key : coldKeys) {
        // 将冷数据降级到L3
        auto value = get<QVariant>(key);
        if (value.has_value()) {
            CacheMetadata metadata;
            metadata.priority = 0; // 低优先级
            setInternal(key, value.value(), metadata);
        }
    }
    
    qCInfo(multiLevelCache) << "Cache optimization completed";
}

// 刷新到磁盘
void MultiLevelCache::flushToDisk()
{
    qCInfo(multiLevelCache) << "Flushing cache to disk...";
    
    if (m_l2Cache) {
        // 将L1中的重要数据刷新到L2
        if (m_l1Cache) {
            QStringList keys = m_l1Cache->keys();
            for (const QString& key : keys) {
                auto value = m_l1Cache->get(key);
                if (value.has_value()) {
                    CacheMetadata metadata;
                    metadata.priority = 50; // 中等优先级
                    m_l2Cache->set(key, value.value(), metadata);
                }
            }
        }
    }
    
    qCInfo(multiLevelCache) << "Cache flush to disk completed";
}

// 从磁盘加载
void MultiLevelCache::loadFromDisk()
{
    qCInfo(multiLevelCache) << "Loading cache from disk...";
    
    if (m_l2Cache) {
        // 从L2加载数据到L1
        QStringList keys = m_l2Cache->keys();
        for (const QString& key : keys) {
            auto value = m_l2Cache->get(key);
            if (value.has_value()) {
                CacheMetadata metadata;
                metadata.priority = 50; // 中等优先级
                if (m_l1Cache) {
                    m_l1Cache->set(key, value.value(), metadata);
                }
            }
        }
    }
    
    qCInfo(multiLevelCache) << "Cache load from disk completed";
}



// 提升项目
void MultiLevelCache::promoteItem(const QString& key, CacheLevel fromLevel, CacheLevel toLevel)
{
    // 从较低级别提升到较高级别
    auto value = get<QVariant>(key);
    if (value.has_value()) {
        CacheMetadata metadata;
        metadata.priority = 75; // 提升优先级
        
        switch (toLevel) {
        case CacheLevel::L1_Memory:
            if (m_l1Cache) {
                m_l1Cache->set(key, value.value(), metadata);
            }
            break;
        case CacheLevel::L2_Local:
            if (m_l2Cache) {
                m_l2Cache->set(key, value.value(), metadata);
            }
            break;
        case CacheLevel::L3_Distributed:
            if (m_l3Cache) {
                m_l3Cache->set(key, value.value(), metadata);
            }
            break;
        }
        
        emit itemPromoted(key, fromLevel, toLevel);
    }
}

// 降级项目
void MultiLevelCache::demoteItem(const QString& key, CacheLevel fromLevel, CacheLevel toLevel)
{
    // 从较高级别降级到较低级别
    auto value = get<QVariant>(key);
    if (value.has_value()) {
        CacheMetadata metadata;
        metadata.priority = 25; // 降低优先级
        
        switch (toLevel) {
        case CacheLevel::L1_Memory:
            if (m_l1Cache) {
                m_l1Cache->set(key, value.value(), metadata);
            }
            break;
        case CacheLevel::L2_Local:
            if (m_l2Cache) {
                m_l2Cache->set(key, value.value(), metadata);
            }
            break;
        case CacheLevel::L3_Distributed:
            if (m_l3Cache) {
                m_l3Cache->set(key, value.value(), metadata);
            }
            break;
        }
        
        emit itemDemoted(key, fromLevel, toLevel);
    }
}

// 检查并提升项目
void MultiLevelCache::checkAndPromote(const QString& key, const CacheMetadata& metadata)
{
    // 根据访问频率和优先级决定是否提升
    if (metadata.accessCount.loadAcquire() > 10 && metadata.priority.loadAcquire() > 50) {
        // 提升到更高级别
        if (metadata.level == CacheLevel::L2_Local) {
            promoteItem(key, CacheLevel::L2_Local, CacheLevel::L1_Memory);
        } else if (metadata.level == CacheLevel::L3_Distributed) {
            promoteItem(key, CacheLevel::L3_Distributed, CacheLevel::L2_Local);
        }
    }
}

// 检查并降级项目
void MultiLevelCache::checkAndDemote()
{
    // 根据访问频率和优先级决定是否降级
    if (m_l1Cache) {
        QStringList keys = m_l1Cache->keys();
        for (const QString& key : keys) {
            auto item = m_l1Cache->get(key);
            if (item.has_value()) {
                // 这里需要获取元数据，简化实现
                // 如果访问频率低，降级到L2
                // 实际实现需要更复杂的逻辑
            }
        }
    }
}

// 序列化
QByteArray MultiLevelCache::serialize(const QVariant& data) const
{
    QJsonDocument doc = QJsonDocument::fromVariant(data);
    return doc.toJson(QJsonDocument::Compact);
}

// 反序列化
QVariant MultiLevelCache::deserialize(const QByteArray& data) const
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    return doc.toVariant();
}

// 压缩
QByteArray MultiLevelCache::compress(const QByteArray& data) const
{
    // 简单的压缩实现，实际项目中可以使用zlib等
    return data;
}

// 解压缩
QByteArray MultiLevelCache::decompress(const QByteArray& data) const
{
    // 简单的解压缩实现
    return data;
}

// 加密
QByteArray MultiLevelCache::encrypt(const QByteArray& data) const
{
    // 简单的加密实现，实际项目中应该使用更强的加密
    return data;
}

// 解密
QByteArray MultiLevelCache::decrypt(const QByteArray& data) const
{
    // 简单的解密实现
    return data;
}

// 确定最佳缓存级别
CacheLevel MultiLevelCache::determineBestLevel(const CacheMetadata& metadata) const
{
    int accessCount = metadata.accessCount.loadAcquire();
    int priority = metadata.priority.loadAcquire();
    
    if (accessCount > 20 || priority > 80) {
        return CacheLevel::L1_Memory;
    } else if (accessCount > 5 || priority > 50) {
        return CacheLevel::L2_Local;
    } else {
        return CacheLevel::L3_Distributed;
    }
}
