#ifndef CACHEPRELOADER_H
#define CACHEPRELOADER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <QJsonObject>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <memory>
#include <functional>
#include <future>
#include "MultiLevelCache.h"
#include "../core/ThreadManager.h"

Q_DECLARE_LOGGING_CATEGORY(cachePreloader)

/**
 * @brief 预加载任务类型
 */
enum class PreloadTaskType {
    Immediate = 0,      // 立即预加载
    Scheduled = 1,      // 定时预加载
    Conditional = 2,    // 条件预加载
    Batch = 3,          // 批量预加载
    Adaptive = 4        // 自适应预加载
};

/**
 * @brief 预加载优先级
 */
enum class PreloadPriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief 预加载任务
 */
struct PreloadTask {
    QString id{};
    QString key{};
    QString category{};
    PreloadTaskType type{PreloadTaskType::Immediate};
    PreloadPriority priority{PreloadPriority::Normal};
    std::function<QVariant()> loader{};
    std::function<bool()> condition{};
    QDateTime scheduledTime{};
    QDateTime createdAt{};
    int ttlSeconds{-1};
    int retryCount{0};
    int maxRetries{3};
    bool completed{false};
    QString errorMessage{};
    
    PreloadTask()
        : type(PreloadTaskType::Immediate)
        , priority(PreloadPriority::Normal)
        , createdAt(QDateTime::currentDateTime())
        , ttlSeconds(-1)
        , retryCount(0)
        , maxRetries(3)
        , completed(false)
    {}
    
    bool isReady() const {
        if (type == PreloadTaskType::Scheduled) {
            return QDateTime::currentDateTime() >= scheduledTime;
        }
        if (type == PreloadTaskType::Conditional && condition) {
            return condition();
        }
        return true;
    }
    
    bool shouldRetry() const {
        return !completed && retryCount < maxRetries;
    }
};

/**
 * @brief 预加载统计信息
 */
struct PreloadStatistics {
    QAtomicInt totalTasks{0};
    QAtomicInt completedTasks{0};
    QAtomicInt failedTasks{0};
    QAtomicInt pendingTasks{0};
    QAtomicInt retryTasks{0};
    
    // 按类型统计
    QAtomicInt immediateTasks{0};
    QAtomicInt scheduledTasks{0};
    QAtomicInt conditionalTasks{0};
    QAtomicInt batchTasks{0};
    QAtomicInt adaptiveTasks{0};
    
    // 性能统计
    QAtomicInt averageLoadTime{0};  // 毫秒
    QAtomicInt maxLoadTime{0};      // 毫秒
    QAtomicInt cacheHitImprovement{0}; // 缓存命中率改善百分比
    
    double getSuccessRate() const {
        int total = totalTasks.loadAcquire();
        if (total == 0) return 0.0;
        return static_cast<double>(completedTasks.loadAcquire()) / total;
    }
};

/**
 * @brief 智能缓存预加载器
 * 
 * 功能：
 * - 多种预加载策略 (立即、定时、条件、批量、自适应)
 * - 优先级队列管理
 * - 智能重试机制
 * - 负载均衡和限流
 * - 预加载效果评估
 */
class CachePreloader : public QObject
{
    Q_OBJECT

public:
    struct PreloaderConfig {
        int maxConcurrentTasks;            // 最大并发任务数
        int maxQueueSize;                  // 最大队列大小
        int defaultTTL;                    // 默认TTL(秒)
        int maxRetries;                    // 最大重试次数
        int retryDelay;                    // 重试延迟(毫秒)
        
        // 限流配置
        bool enableRateLimit;              // 启用限流
        int maxTasksPerSecond;             // 每秒最大任务数
        int rateLimitWindow;               // 限流窗口(毫秒)
        
        // 自适应配置
        bool enableAdaptive;               // 启用自适应
        double loadThreshold;              // 负载阈值
        int adaptiveInterval;              // 自适应间隔(毫秒)
        
        // 批量配置
        int batchSize;                     // 批量大小
        int batchTimeout;                  // 批量超时(毫秒)
        
        // 监控配置
        bool enableMetrics;                // 启用指标收集
        int metricsInterval;               // 指标收集间隔(毫秒)
        
        // 默认构造函数
        PreloaderConfig()
            : maxConcurrentTasks(5)
            , maxQueueSize(1000)
            , defaultTTL(3600)
            , maxRetries(3)
            , retryDelay(5000)
            , enableRateLimit(true)
            , maxTasksPerSecond(10)
            , rateLimitWindow(1000)
            , enableAdaptive(true)
            , loadThreshold(0.8)
            , adaptiveInterval(60000)
            , batchSize(50)
            , batchTimeout(5000)
            , enableMetrics(true)
            , metricsInterval(30000)
        {}
    };

    explicit CachePreloader(MultiLevelCache* cache, QObject *parent = nullptr);
    ~CachePreloader();

    // 初始化和配置
    bool initialize(const PreloaderConfig& config = PreloaderConfig());
    void shutdown();
    bool isRunning() const;
    
    // 任务提交
    QString submitTask(const QString& key, std::function<QVariant()> loader,
                      PreloadPriority priority = PreloadPriority::Normal,
                      int ttlSeconds = -1, const QString& category = QString());
    
    QString scheduleTask(const QString& key, std::function<QVariant()> loader,
                        const QDateTime& scheduledTime,
                        PreloadPriority priority = PreloadPriority::Normal,
                        int ttlSeconds = -1, const QString& category = QString());
    
    QString submitConditionalTask(const QString& key, std::function<QVariant()> loader,
                                 std::function<bool()> condition,
                                 PreloadPriority priority = PreloadPriority::Normal,
                                 int ttlSeconds = -1, const QString& category = QString());
    
    QStringList submitBatchTasks(const QHash<QString, std::function<QVariant()>>& loaders,
                                PreloadPriority priority = PreloadPriority::Normal,
                                int ttlSeconds = -1, const QString& category = QString());
    
    // 自适应预加载
    void enableAdaptivePreloading(bool enabled);
    void addAdaptivePattern(const QString& pattern, std::function<QStringList()> keyGenerator);
    void removeAdaptivePattern(const QString& pattern);
    
    // 任务管理
    bool cancelTask(const QString& taskId);
    bool retryTask(const QString& taskId);
    void cancelAllTasks();
    void pausePreloading();
    void resumePreloading();
    
    // 状态查询
    PreloadTask getTask(const QString& taskId) const;
    QList<PreloadTask> getPendingTasks() const;
    QList<PreloadTask> getCompletedTasks() const;
    QList<PreloadTask> getFailedTasks() const;
    
    // 统计信息
    PreloadStatistics getStatistics() const;
    void resetStatistics();
    QJsonObject getMetrics() const;
    QJsonObject getPerformanceReport() const;
    
    // 配置管理
    void updateConfig(const PreloaderConfig& config);
    PreloaderConfig getCurrentConfig() const;
    void setMaxConcurrentTasks(int maxTasks);
    void setRateLimit(int maxTasksPerSecond);

signals:
    void taskCompleted(const QString& taskId, const QString& key);
    void taskFailed(const QString& taskId, const QString& key, const QString& error);
    void batchCompleted(const QStringList& taskIds, int successCount, int failureCount);
    void queueOverflow();
    void rateLimitExceeded();
    void performanceAlert(const QString& message);

private slots:
    void processTaskQueue();
    void checkScheduledTasks();
    void performAdaptivePreloading();
    void updateMetrics();
    void cleanupCompletedTasks();

private:
    // 核心组件
    MultiLevelCache* m_cache;
    ThreadManager* m_threadManager;
    PreloaderConfig m_config;
    QAtomicInt m_running{0};
    QAtomicInt m_paused{0};
    
    // 任务队列
    QQueue<PreloadTask> m_immediateQueue;
    QQueue<PreloadTask> m_scheduledQueue;
    QQueue<PreloadTask> m_retryQueue;
    QHash<QString, PreloadTask> m_allTasks;
    mutable QMutex m_queueMutex;
    QWaitCondition m_queueCondition;
    
    // 优先级队列
    QQueue<PreloadTask> m_criticalQueue;
    QQueue<PreloadTask> m_highQueue;
    QQueue<PreloadTask> m_normalQueue;
    QQueue<PreloadTask> m_lowQueue;
    
    // 自适应模式
    QHash<QString, std::function<QStringList()>> m_adaptivePatterns;
    QMutex m_adaptiveMutex;
    
    // 限流控制
    QQueue<QDateTime> m_recentTasks;
    QMutex m_rateLimitMutex;
    
    // 统计信息
    PreloadStatistics m_stats;
    QList<QJsonObject> m_metricsHistory;
    
    // 定时器
    QTimer* m_processTimer;
    QTimer* m_scheduledTimer;
    QTimer* m_adaptiveTimer;
    QTimer* m_metricsTimer;
    QTimer* m_cleanupTimer;
    
    // 并发控制
    QAtomicInt m_activeTasks{0};
    QAtomicInt m_taskIdCounter{0};
    
    // 内部方法
    QString generateTaskId();
    PreloadTask createTask(const QString& key, std::function<QVariant()> loader,
                          PreloadTaskType type, PreloadPriority priority,
                          int ttlSeconds, const QString& category);
    
    void enqueueTask(const PreloadTask& task);
    PreloadTask dequeueNextTask();
    void executeTask(const PreloadTask& task);
    void retryFailedTask(const PreloadTask& task);
    
    // 队列管理
    void sortTasksByPriority(QQueue<PreloadTask>& queue);
    QQueue<PreloadTask>* getQueueByPriority(PreloadPriority priority);
    bool isQueueFull() const;
    
    // 限流检查
    bool checkRateLimit();
    void updateRateLimit();
    
    // 自适应逻辑
    void analyzeAccessPatterns();
    QStringList generateAdaptiveKeys();
    void adjustPreloadingStrategy();
    
    // 性能评估
    void evaluatePreloadEffectiveness();
    double calculateCacheHitImprovement() const;
    void updatePerformanceMetrics(const PreloadTask& task, qint64 loadTime);
    
    // 批量处理
    void processBatchTasks(const QList<PreloadTask>& tasks);
    QList<PreloadTask> groupTasksByCategory(const QList<PreloadTask>& tasks);
    
    // 清理和维护
    void cleanupExpiredTasks();
    void cleanupOldMetrics();
    
    // 工具方法
    bool shouldExecuteTask(const PreloadTask& task) const;
    void logPreloadEvent(const QString& event, const QString& taskId, 
                        const QString& details = QString()) const;
};

#endif // CACHEPRELOADER_H
