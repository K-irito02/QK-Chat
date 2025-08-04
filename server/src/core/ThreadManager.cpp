#include "ThreadManager.h"
#include <QDebug>
#include <QCoreApplication>
#include <mutex>

Q_LOGGING_CATEGORY(threadManager, "qkchat.server.threadmanager")

ThreadManager* ThreadManager::s_instance = nullptr;
QMutex ThreadManager::s_instanceMutex;

ThreadManager* ThreadManager::instance()
{
    static ThreadManager* instance = nullptr;
    static std::once_flag flag;

    std::call_once(flag, []() {
        instance = new ThreadManager();
        // 注册清理函数
        std::atexit([]() {
            if (instance) {
                instance->shutdown();
                delete instance;
                instance = nullptr;
            }
        });
    });

    return instance;
}

ThreadManager::ThreadManager(QObject *parent)
    : QObject(parent)
    , m_healthTimer(new QTimer(this))
    , m_monitoringEnabled(false)
    , m_systemHealthy(true)
{
    // 设置默认配置
    m_poolConfigs[NetworkPool] = {2, 4, true, 0.8, "Network"};
    m_poolConfigs[MessagePool] = {4, 8, true, 0.8, "Message"};
    m_poolConfigs[DatabasePool] = {2, 6, true, 0.7, "Database"};
    m_poolConfigs[FilePool] = {2, 4, true, 0.8, "File"};
    m_poolConfigs[ServicePool] = {1, 2, false, 0.8, "Service"};
    
    // 设置健康检查定时器
    m_healthTimer->setInterval(10000); // 10秒检查一次
    connect(m_healthTimer, &QTimer::timeout, this, &ThreadManager::checkSystemHealth);
    
    qCInfo(threadManager) << "ThreadManager created";
}

ThreadManager::~ThreadManager()
{
    shutdown();
    qCInfo(threadManager) << "ThreadManager destroyed";
}

bool ThreadManager::initialize()
{
    qCInfo(threadManager) << "Initializing ThreadManager...";
    
    try {
        // 创建各个线程池
        auto networkConfig = m_poolConfigs[NetworkPool];
        m_networkPool = std::make_unique<ThreadPool>(networkConfig.maxThreads, this);
        setupPool(NetworkPool, m_networkPool.get());
        
        auto messageConfig = m_poolConfigs[MessagePool];
        m_messagePool = std::make_unique<ThreadPool>(messageConfig.maxThreads, this);
        setupPool(MessagePool, m_messagePool.get());
        
        auto databaseConfig = m_poolConfigs[DatabasePool];
        m_databasePool = std::make_unique<ThreadPool>(databaseConfig.maxThreads, this);
        setupPool(DatabasePool, m_databasePool.get());
        
        auto fileConfig = m_poolConfigs[FilePool];
        m_filePool = std::make_unique<ThreadPool>(fileConfig.maxThreads, this);
        setupPool(FilePool, m_filePool.get());
        
        auto serviceConfig = m_poolConfigs[ServicePool];
        m_servicePool = std::make_unique<ThreadPool>(serviceConfig.maxThreads, this);
        setupPool(ServicePool, m_servicePool.get());
        
        qCInfo(threadManager) << "All thread pools created successfully";
        
        // 启用监控
        enableMonitoring(true);
        
        qCInfo(threadManager) << "ThreadManager initialized successfully";
        return true;
        
    } catch (const std::exception &e) {
        qCCritical(threadManager) << "Failed to initialize ThreadManager:" << e.what();
        return false;
    }
}

void ThreadManager::shutdown()
{
    qCInfo(threadManager) << "Shutting down ThreadManager...";
    
    enableMonitoring(false);
    
    // 按依赖顺序关闭线程池
    if (m_servicePool) {
        m_servicePool->shutdown();
        m_servicePool.reset();
    }
    
    if (m_filePool) {
        m_filePool->shutdown();
        m_filePool.reset();
    }
    
    if (m_messagePool) {
        m_messagePool->shutdown();
        m_messagePool.reset();
    }
    
    if (m_databasePool) {
        m_databasePool->shutdown();
        m_databasePool.reset();
    }
    
    if (m_networkPool) {
        m_networkPool->shutdown();
        m_networkPool.reset();
    }
    
    qCInfo(threadManager) << "ThreadManager shutdown complete";
}

void ThreadManager::configurePool(PoolType type, const PoolConfig &config)
{
    m_poolConfigs[type] = config;
    
    ThreadPool* pool = getPool(type);
    if (pool) {
        pool->setMaxThreadCount(config.maxThreads);
        pool->setAutoResize(config.autoResize);
        pool->setLoadThreshold(config.loadThreshold);
        
        qCInfo(threadManager) << "Pool" << poolTypeToString(type) 
                             << "reconfigured: max threads =" << config.maxThreads;
    }
}

ThreadManager::PoolConfig ThreadManager::getPoolConfig(PoolType type) const
{
    return m_poolConfigs.value(type);
}

ThreadManager::SystemStats ThreadManager::getSystemStats() const
{
    SystemStats stats;
    
    if (m_networkPool) stats.networkStats = m_networkPool->getStats();
    if (m_messagePool) stats.messageStats = m_messagePool->getStats();
    if (m_databasePool) stats.databaseStats = m_databasePool->getStats();
    if (m_filePool) stats.fileStats = m_filePool->getStats();
    if (m_servicePool) stats.serviceStats = m_servicePool->getStats();
    
    // 汇总统计
    stats.totalTasks = stats.networkStats.totalTasks.loadAcquire() +
                      stats.messageStats.totalTasks.loadAcquire() +
                      stats.databaseStats.totalTasks.loadAcquire() +
                      stats.fileStats.totalTasks.loadAcquire() +
                      stats.serviceStats.totalTasks.loadAcquire();
    
    stats.completedTasks = stats.networkStats.completedTasks.loadAcquire() +
                          stats.messageStats.completedTasks.loadAcquire() +
                          stats.databaseStats.completedTasks.loadAcquire() +
                          stats.fileStats.completedTasks.loadAcquire() +
                          stats.serviceStats.completedTasks.loadAcquire();
    
    stats.failedTasks = stats.networkStats.failedTasks.loadAcquire() +
                       stats.messageStats.failedTasks.loadAcquire() +
                       stats.databaseStats.failedTasks.loadAcquire() +
                       stats.fileStats.failedTasks.loadAcquire() +
                       stats.serviceStats.failedTasks.loadAcquire();
    
    stats.activeTasks = stats.networkStats.activeTasks.loadAcquire() +
                       stats.messageStats.activeTasks.loadAcquire() +
                       stats.databaseStats.activeTasks.loadAcquire() +
                       stats.fileStats.activeTasks.loadAcquire() +
                       stats.serviceStats.activeTasks.loadAcquire();
    
    stats.queuedTasks = stats.networkStats.queuedTasks.loadAcquire() +
                       stats.messageStats.queuedTasks.loadAcquire() +
                       stats.databaseStats.queuedTasks.loadAcquire() +
                       stats.fileStats.queuedTasks.loadAcquire() +
                       stats.serviceStats.queuedTasks.loadAcquire();
    
    return stats;
}

ThreadPool::TaskStats ThreadManager::getPoolStats(PoolType type) const
{
    ThreadPool* pool = getPool(type);
    if (pool) {
        return pool->getStats();
    }
    return ThreadPool::TaskStats{};
}

void ThreadManager::resetAllStats()
{
    if (m_networkPool) m_networkPool->resetStats();
    if (m_messagePool) m_messagePool->resetStats();
    if (m_databasePool) m_databasePool->resetStats();
    if (m_filePool) m_filePool->resetStats();
    if (m_servicePool) m_servicePool->resetStats();
    
    m_overloadCount.storeRelease(0);
    qCInfo(threadManager) << "All thread pool stats reset";
}

bool ThreadManager::isHealthy() const
{
    return m_systemHealthy;
}

QString ThreadManager::getHealthReport() const
{
    SystemStats stats = getSystemStats();
    
    QString report;
    report += QString("System Health: %1\n").arg(m_systemHealthy ? "Healthy" : "Unhealthy");
    report += QString("Total Tasks: %1 (Completed: %2, Failed: %3)\n")
              .arg(stats.totalTasks.loadAcquire())
              .arg(stats.completedTasks.loadAcquire())
              .arg(stats.failedTasks.loadAcquire());
    report += QString("Active Tasks: %1, Queued Tasks: %2\n")
              .arg(stats.activeTasks.loadAcquire())
              .arg(stats.queuedTasks.loadAcquire());
    report += QString("Overload Count: %1\n").arg(m_overloadCount.loadAcquire());
    
    return report;
}

void ThreadManager::enableMonitoring(bool enabled)
{
    m_monitoringEnabled = enabled;
    if (enabled) {
        m_healthTimer->start();
        qCInfo(threadManager) << "Monitoring enabled";
    } else {
        m_healthTimer->stop();
        qCInfo(threadManager) << "Monitoring disabled";
    }
}

void ThreadManager::onPoolOverloaded()
{
    ThreadPool* pool = qobject_cast<ThreadPool*>(sender());
    if (!pool) return;
    
    PoolType type = NetworkPool; // 默认值
    if (pool == m_networkPool.get()) type = NetworkPool;
    else if (pool == m_messagePool.get()) type = MessagePool;
    else if (pool == m_databasePool.get()) type = DatabasePool;
    else if (pool == m_filePool.get()) type = FilePool;
    else if (pool == m_servicePool.get()) type = ServicePool;
    
    m_overloadCount.fetchAndAddOrdered(1);
    emit poolOverloaded(type);
    
    qCWarning(threadManager) << "Pool overloaded:" << poolTypeToString(type);
}

void ThreadManager::onTaskCompleted()
{
    ThreadPool* pool = qobject_cast<ThreadPool*>(sender());
    if (!pool) return;
    
    PoolType type = NetworkPool; // 默认值
    if (pool == m_networkPool.get()) type = NetworkPool;
    else if (pool == m_messagePool.get()) type = MessagePool;
    else if (pool == m_databasePool.get()) type = DatabasePool;
    else if (pool == m_filePool.get()) type = FilePool;
    else if (pool == m_servicePool.get()) type = ServicePool;
    
    emit taskCompleted(type);
}

void ThreadManager::onTaskFailed()
{
    ThreadPool* pool = qobject_cast<ThreadPool*>(sender());
    if (!pool) return;
    
    PoolType type = NetworkPool; // 默认值
    if (pool == m_networkPool.get()) type = NetworkPool;
    else if (pool == m_messagePool.get()) type = MessagePool;
    else if (pool == m_databasePool.get()) type = DatabasePool;
    else if (pool == m_filePool.get()) type = FilePool;
    else if (pool == m_servicePool.get()) type = ServicePool;
    
    emit taskFailed(type);
}

void ThreadManager::checkSystemHealth()
{
    if (!m_monitoringEnabled) return;
    
    SystemStats stats = getSystemStats();
    bool wasHealthy = m_systemHealthy;
    
    // 健康检查逻辑
    int totalActive = stats.activeTasks.loadAcquire();
    int totalQueued = stats.queuedTasks.loadAcquire();
    int totalFailed = stats.failedTasks.loadAcquire();
    int totalCompleted = stats.completedTasks.loadAcquire();
    
    // 计算失败率
    double failureRate = 0.0;
    if (totalCompleted + totalFailed > 0) {
        failureRate = static_cast<double>(totalFailed) / (totalCompleted + totalFailed);
    }
    
    // 判断系统健康状态
    m_systemHealthy = (totalQueued < 1000) &&  // 排队任务不超过1000
                     (failureRate < 0.05) &&   // 失败率低于5%
                     (totalActive < 50);       // 活跃任务不超过50
    
    if (wasHealthy != m_systemHealthy) {
        emit healthStatusChanged(m_systemHealthy);
        qCInfo(threadManager) << "System health changed to:" 
                             << (m_systemHealthy ? "Healthy" : "Unhealthy");
    }
    
    // 如果系统过载，发出信号
    if (!m_systemHealthy && totalQueued > 500) {
        emit systemOverloaded();
    }
}

ThreadPool* ThreadManager::getPool(PoolType type) const
{
    switch (type) {
    case NetworkPool: return m_networkPool.get();
    case MessagePool: return m_messagePool.get();
    case DatabasePool: return m_databasePool.get();
    case FilePool: return m_filePool.get();
    case ServicePool: return m_servicePool.get();
    default: return nullptr;
    }
}

void ThreadManager::setupPool(PoolType type, ThreadPool* pool)
{
    if (!pool) return;
    
    auto config = m_poolConfigs[type];
    pool->setAutoResize(config.autoResize);
    pool->setLoadThreshold(config.loadThreshold);
    
    // 连接信号
    connect(pool, &ThreadPool::poolOverloaded, this, &ThreadManager::onPoolOverloaded);
    connect(pool, &ThreadPool::taskCompleted, this, &ThreadManager::onTaskCompleted);
    connect(pool, &ThreadPool::taskFailed, this, &ThreadManager::onTaskFailed);
    
    qCInfo(threadManager) << "Pool" << poolTypeToString(type) << "configured with"
                         << config.maxThreads << "max threads";
}

QString ThreadManager::poolTypeToString(PoolType type) const
{
    switch (type) {
    case NetworkPool: return "Network";
    case MessagePool: return "Message";
    case DatabasePool: return "Database";
    case FilePool: return "File";
    case ServicePool: return "Service";
    default: return "Unknown";
    }
}
