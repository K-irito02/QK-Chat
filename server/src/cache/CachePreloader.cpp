#include "CachePreloader.h"
#include <QDebug>
#include <QUuid>
#include <QCoreApplication>
#include <algorithm>

Q_LOGGING_CATEGORY(cachePreloader, "qkchat.server.cachepreloader")

CachePreloader::CachePreloader(MultiLevelCache* cache, QObject *parent)
    : QObject(parent)
    , m_cache(cache)
    , m_threadManager(ThreadManager::instance())
    , m_processTimer(new QTimer(this))
    , m_scheduledTimer(new QTimer(this))
    , m_adaptiveTimer(new QTimer(this))
    , m_metricsTimer(new QTimer(this))
    , m_cleanupTimer(new QTimer(this))
{
    // 设置定时器
    connect(m_processTimer, &QTimer::timeout, this, &CachePreloader::processTaskQueue);
    connect(m_scheduledTimer, &QTimer::timeout, this, &CachePreloader::checkScheduledTasks);
    connect(m_adaptiveTimer, &QTimer::timeout, this, &CachePreloader::performAdaptivePreloading);
    connect(m_metricsTimer, &QTimer::timeout, this, &CachePreloader::updateMetrics);
    connect(m_cleanupTimer, &QTimer::timeout, this, &CachePreloader::cleanupCompletedTasks);
    
    qCInfo(cachePreloader) << "CachePreloader created";
}

CachePreloader::~CachePreloader()
{
    shutdown();
    qCInfo(cachePreloader) << "CachePreloader destroyed";
}

bool CachePreloader::initialize(const PreloaderConfig& config)
{
    qCInfo(cachePreloader) << "Initializing CachePreloader...";
    
    if (!m_cache) {
        qCCritical(cachePreloader) << "Cache is null";
        return false;
    }
    
    if (!m_threadManager) {
        qCCritical(cachePreloader) << "ThreadManager is null";
        return false;
    }
    
    m_config = config;
    
    // 启动定时器
    m_processTimer->start(100); // 100ms处理间隔
    m_scheduledTimer->start(1000); // 1秒检查定时任务
    m_adaptiveTimer->start(config.adaptiveInterval);
    m_metricsTimer->start(config.metricsInterval);
    m_cleanupTimer->start(60000); // 1分钟清理一次
    
    m_running.storeRelease(1);
    
    qCInfo(cachePreloader) << "CachePreloader initialized successfully";
    return true;
}

void CachePreloader::shutdown()
{
    if (m_running.testAndSetOrdered(1, 0)) {
        qCInfo(cachePreloader) << "Shutting down CachePreloader...";
        
        // 停止定时器
        m_processTimer->stop();
        m_scheduledTimer->stop();
        m_adaptiveTimer->stop();
        m_metricsTimer->stop();
        m_cleanupTimer->stop();
        
        // 等待活跃任务完成
        while (m_activeTasks.loadAcquire() > 0) {
            QCoreApplication::processEvents();
            QThread::msleep(10);
        }
        
        // 清理队列
        QMutexLocker locker(&m_queueMutex);
        m_immediateQueue.clear();
        m_scheduledQueue.clear();
        m_retryQueue.clear();
        m_allTasks.clear();
        
        qCInfo(cachePreloader) << "CachePreloader shutdown complete";
    }
}

bool CachePreloader::isRunning() const
{
    return m_running.loadAcquire() > 0;
}

QString CachePreloader::submitTask(const QString& key, std::function<QVariant()> loader,
                                  PreloadPriority priority, int ttlSeconds, const QString& category)
{
    if (!isRunning()) {
        return QString();
    }
    
    PreloadTask task = createTask(key, loader, PreloadTaskType::Immediate, priority, ttlSeconds, category);
    enqueueTask(task);
    
    logPreloadEvent("TASK_SUBMITTED", task.id, QString("key=%1, priority=%2").arg(key).arg(static_cast<int>(priority)));
    return task.id;
}

QString CachePreloader::scheduleTask(const QString& key, std::function<QVariant()> loader,
                                    const QDateTime& scheduledTime, PreloadPriority priority,
                                    int ttlSeconds, const QString& category)
{
    if (!isRunning()) {
        return QString();
    }
    
    PreloadTask task = createTask(key, loader, PreloadTaskType::Scheduled, priority, ttlSeconds, category);
    task.scheduledTime = scheduledTime;
    enqueueTask(task);
    
    logPreloadEvent("TASK_SCHEDULED", task.id, QString("key=%1, time=%2").arg(key).arg(scheduledTime.toString()));
    return task.id;
}

QString CachePreloader::submitConditionalTask(const QString& key, std::function<QVariant()> loader,
                                             std::function<bool()> condition, PreloadPriority priority,
                                             int ttlSeconds, const QString& category)
{
    if (!isRunning()) {
        return QString();
    }
    
    PreloadTask task = createTask(key, loader, PreloadTaskType::Conditional, priority, ttlSeconds, category);
    task.condition = condition;
    enqueueTask(task);
    
    logPreloadEvent("CONDITIONAL_TASK_SUBMITTED", task.id, QString("key=%1").arg(key));
    return task.id;
}

QStringList CachePreloader::submitBatchTasks(const QHash<QString, std::function<QVariant()>>& loaders,
                                            PreloadPriority priority, int ttlSeconds, const QString& category)
{
    QStringList taskIds;
    for (auto it = loaders.begin(); it != loaders.end(); ++it) {
        QString taskId = submitTask(it.key(), it.value(), priority, ttlSeconds, category);
        if (!taskId.isEmpty()) {
            taskIds.append(taskId);
        }
    }
    
    m_stats.batchTasks.fetchAndAddOrdered(1);
    return taskIds;
}

bool CachePreloader::cancelTask(const QString& taskId)
{
    QMutexLocker locker(&m_queueMutex);
    
    auto it = m_allTasks.find(taskId);
    if (it != m_allTasks.end() && !it->completed) {
        it->completed = true;
        it->errorMessage = "Cancelled by user";
        
        logPreloadEvent("TASK_CANCELLED", taskId);
        return true;
    }
    
    return false;
}

PreloadStatistics CachePreloader::getStatistics() const
{
    return m_stats;
}

void CachePreloader::resetStatistics()
{
    m_stats = PreloadStatistics{};
    qCInfo(cachePreloader) << "Preloader statistics reset";
}

QJsonObject CachePreloader::getMetrics() const
{
    QJsonObject metrics;
    
    metrics["total_tasks"] = m_stats.totalTasks.loadAcquire();
    metrics["completed_tasks"] = m_stats.completedTasks.loadAcquire();
    metrics["failed_tasks"] = m_stats.failedTasks.loadAcquire();
    metrics["pending_tasks"] = m_stats.pendingTasks.loadAcquire();
    metrics["success_rate"] = m_stats.getSuccessRate();
    metrics["average_load_time"] = m_stats.averageLoadTime.loadAcquire();
    metrics["active_tasks"] = m_activeTasks.loadAcquire();
    
    return metrics;
}

void CachePreloader::processTaskQueue()
{
    if (m_paused.loadAcquire() || !checkRateLimit()) {
        return;
    }
    
    int maxConcurrent = m_config.maxConcurrentTasks;
    int activeTasks = m_activeTasks.loadAcquire();
    
    if (activeTasks >= maxConcurrent) {
        return;
    }
    
    PreloadTask task = dequeueNextTask();
    if (!task.id.isEmpty() && shouldExecuteTask(task)) {
        executeTask(task);
    }
}

void CachePreloader::checkScheduledTasks()
{
    QMutexLocker locker(&m_queueMutex);
    
    QQueue<PreloadTask> readyTasks;
    while (!m_scheduledQueue.isEmpty()) {
        PreloadTask task = m_scheduledQueue.head();
        if (task.isReady()) {
            readyTasks.enqueue(m_scheduledQueue.dequeue());
        } else {
            break;
        }
    }
    
    // 将就绪的任务移到立即队列
    while (!readyTasks.isEmpty()) {
        PreloadTask task = readyTasks.dequeue();
        task.type = PreloadTaskType::Immediate;
        getQueueByPriority(task.priority)->enqueue(task);
    }
}

void CachePreloader::performAdaptivePreloading()
{
    if (!m_config.enableAdaptive) {
        return;
    }
    
    analyzeAccessPatterns();
    QStringList adaptiveKeys = generateAdaptiveKeys();
    
    for (const QString& key : adaptiveKeys) {
        // 为自适应键创建预加载任务
        auto loader = [key]() -> QVariant {
            // 这里应该根据键的模式生成数据
            return QVariant(QString("adaptive_data_for_%1").arg(key));
        };
        
        submitTask(key, loader, PreloadPriority::Low, -1, "adaptive");
    }
    
    m_stats.adaptiveTasks.fetchAndAddOrdered(adaptiveKeys.size());
}

void CachePreloader::updateMetrics()
{
    QMutexLocker locker(&m_queueMutex);
    
    int pending = m_immediateQueue.size() + m_scheduledQueue.size() + 
                  m_criticalQueue.size() + m_highQueue.size() + 
                  m_normalQueue.size() + m_lowQueue.size();
    
    m_stats.pendingTasks.storeRelease(pending);
    
    // 更新性能指标
    evaluatePreloadEffectiveness();
}

void CachePreloader::cleanupCompletedTasks()
{
    QMutexLocker locker(&m_queueMutex);
    
    auto it = m_allTasks.begin();
    while (it != m_allTasks.end()) {
        if (it->completed && it->createdAt.addSecs(3600) < QDateTime::currentDateTime()) {
            it = m_allTasks.erase(it);
        } else {
            ++it;
        }
    }
}

QString CachePreloader::generateTaskId()
{
    return QString("preload_%1_%2")
           .arg(m_taskIdCounter.fetchAndAddOrdered(1))
           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}

PreloadTask CachePreloader::createTask(const QString& key, std::function<QVariant()> loader,
                                      PreloadTaskType type, PreloadPriority priority,
                                      int ttlSeconds, const QString& category)
{
    PreloadTask task;
    task.id = generateTaskId();
    task.key = key;
    task.category = category;
    task.type = type;
    task.priority = priority;
    task.loader = loader;
    task.ttlSeconds = ttlSeconds > 0 ? ttlSeconds : m_config.defaultTTL;
    task.maxRetries = m_config.maxRetries;
    
    return task;
}

void CachePreloader::enqueueTask(const PreloadTask& task)
{
    if (isQueueFull()) {
        emit queueOverflow();
        return;
    }
    
    QMutexLocker locker(&m_queueMutex);
    
    m_allTasks[task.id] = task;
    m_stats.totalTasks.fetchAndAddOrdered(1);
    
    if (task.type == PreloadTaskType::Scheduled) {
        m_scheduledQueue.enqueue(task);
    } else {
        getQueueByPriority(task.priority)->enqueue(task);
    }
    
    m_queueCondition.wakeOne();
}

PreloadTask CachePreloader::dequeueNextTask()
{
    QMutexLocker locker(&m_queueMutex);
    
    // 按优先级顺序出队
    if (!m_criticalQueue.isEmpty()) {
        return m_criticalQueue.dequeue();
    }
    if (!m_highQueue.isEmpty()) {
        return m_highQueue.dequeue();
    }
    if (!m_normalQueue.isEmpty()) {
        return m_normalQueue.dequeue();
    }
    if (!m_lowQueue.isEmpty()) {
        return m_lowQueue.dequeue();
    }
    if (!m_immediateQueue.isEmpty()) {
        return m_immediateQueue.dequeue();
    }
    
    return PreloadTask(); // 返回空任务
}

void CachePreloader::executeTask(const PreloadTask& task)
{
    if (!m_threadManager) {
        return;
    }
    
    m_activeTasks.fetchAndAddOrdered(1);
    
    m_threadManager->submitServiceTask([this, task]() {
        QDateTime startTime = QDateTime::currentDateTime();
        
        try {
            if (task.loader) {
                QVariant data = task.loader();
                
                if (m_cache) {
                    bool success = m_cache->set(task.key, data, task.ttlSeconds, task.category);
                    
                    if (success) {
                        m_stats.completedTasks.fetchAndAddOrdered(1);
                        emit taskCompleted(task.id, task.key);
                        logPreloadEvent("TASK_COMPLETED", task.id, task.key);
                    } else {
                        m_stats.failedTasks.fetchAndAddOrdered(1);
                        emit taskFailed(task.id, task.key, "Failed to cache data");
                        logPreloadEvent("TASK_FAILED", task.id, "Cache operation failed");
                    }
                }
            }
        } catch (const std::exception& e) {
            m_stats.failedTasks.fetchAndAddOrdered(1);
            emit taskFailed(task.id, task.key, QString("Exception: %1").arg(e.what()));
            logPreloadEvent("TASK_EXCEPTION", task.id, e.what());
        }
        
        qint64 loadTime = startTime.msecsTo(QDateTime::currentDateTime());
        updatePerformanceMetrics(task, loadTime);
        
        m_activeTasks.fetchAndSubOrdered(1);
    });
}

QQueue<PreloadTask>* CachePreloader::getQueueByPriority(PreloadPriority priority)
{
    switch (priority) {
    case PreloadPriority::Critical: return &m_criticalQueue;
    case PreloadPriority::High: return &m_highQueue;
    case PreloadPriority::Normal: return &m_normalQueue;
    case PreloadPriority::Low: return &m_lowQueue;
    default: return &m_normalQueue;
    }
}

bool CachePreloader::isQueueFull() const
{
    QMutexLocker locker(&m_queueMutex);
    return m_allTasks.size() >= m_config.maxQueueSize;
}

bool CachePreloader::checkRateLimit()
{
    if (!m_config.enableRateLimit) {
        return true;
    }
    
    QMutexLocker locker(&m_rateLimitMutex);
    
    QDateTime now = QDateTime::currentDateTime();
    QDateTime cutoff = now.addMSecs(-m_config.rateLimitWindow);
    
    // 清理过期的时间戳
    while (!m_recentTasks.isEmpty() && m_recentTasks.head() < cutoff) {
        m_recentTasks.dequeue();
    }
    
    if (m_recentTasks.size() >= m_config.maxTasksPerSecond) {
        emit rateLimitExceeded();
        return false;
    }
    
    m_recentTasks.enqueue(now);
    return true;
}

void CachePreloader::analyzeAccessPatterns()
{
    // 分析访问模式的简化实现
    // 实际实现应该分析缓存访问历史
}

QStringList CachePreloader::generateAdaptiveKeys()
{
    QStringList keys;
    
    QMutexLocker locker(&m_adaptiveMutex);
    for (auto it = m_adaptivePatterns.begin(); it != m_adaptivePatterns.end(); ++it) {
        try {
            QStringList patternKeys = it.value()();
            keys.append(patternKeys);
        } catch (const std::exception& e) {
            qCWarning(cachePreloader) << "Adaptive pattern" << it.key() << "failed:" << e.what();
        }
    }
    
    return keys;
}

void CachePreloader::evaluatePreloadEffectiveness()
{
    // 评估预加载效果的简化实现
    // 实际实现应该分析缓存命中率改善
}

void CachePreloader::updatePerformanceMetrics(const PreloadTask& task, qint64 loadTime)
{
    // 更新平均加载时间
    int currentAvg = m_stats.averageLoadTime.loadAcquire();
    int newAvg = (currentAvg + loadTime) / 2;
    m_stats.averageLoadTime.storeRelease(newAvg);
    
    // 更新最大加载时间
    int currentMax = m_stats.maxLoadTime.loadAcquire();
    if (loadTime > currentMax) {
        m_stats.maxLoadTime.testAndSetOrdered(currentMax, loadTime);
    }
}

bool CachePreloader::shouldExecuteTask(const PreloadTask& task) const
{
    if (task.completed) {
        return false;
    }
    
    if (task.type == PreloadTaskType::Conditional && task.condition) {
        return task.condition();
    }
    
    return true;
}

void CachePreloader::logPreloadEvent(const QString& event, const QString& taskId, const QString& details) const
{
    if (details.isEmpty()) {
        qCDebug(cachePreloader) << event << "taskId:" << taskId;
    } else {
        qCDebug(cachePreloader) << event << "taskId:" << taskId << "details:" << details;
    }
}
