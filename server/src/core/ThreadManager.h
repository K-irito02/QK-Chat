#ifndef THREADMANAGER_H
#define THREADMANAGER_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QAtomicInt>
#include <QLoggingCategory>
#include <memory>
#include "../utils/ThreadPool.h"

Q_DECLARE_LOGGING_CATEGORY(threadManager)

/**
 * @brief 线程管理器 - 统一管理所有线程池
 * 
 * 职责分离：
 * - 网络I/O线程池：处理网络事件和连接管理
 * - 消息处理线程池：处理客户端消息解析和路由
 * - 数据库操作线程池：处理所有数据库读写操作
 * - 文件传输线程池：处理文件上传下载
 * - 后台服务线程池：处理心跳检测、缓存同步等
 */
class ThreadManager : public QObject
{
    Q_OBJECT

public:
    enum PoolType {
        NetworkPool = 0,    // 网络I/O处理
        MessagePool = 1,    // 消息处理
        DatabasePool = 2,   // 数据库操作
        FilePool = 3,       // 文件传输
        ServicePool = 4     // 后台服务
    };

    struct PoolConfig {
        int minThreads = 2;
        int maxThreads = 8;
        bool autoResize = true;
        double loadThreshold = 0.8;
        QString name{};
    };

    struct SystemStats {
        QAtomicInt totalTasks{0};
        QAtomicInt completedTasks{0};
        QAtomicInt failedTasks{0};
        QAtomicInt activeTasks{0};
        QAtomicInt queuedTasks{0};
        
        // 各线程池统计
        ThreadPool::TaskStats networkStats;
        ThreadPool::TaskStats messageStats;
        ThreadPool::TaskStats databaseStats;
        ThreadPool::TaskStats fileStats;
        ThreadPool::TaskStats serviceStats;
    };

    static ThreadManager* instance();
    
    // 初始化和配置
    bool initialize();
    void shutdown();
    
    // 线程池配置
    void configurePool(PoolType type, const PoolConfig &config);
    PoolConfig getPoolConfig(PoolType type) const;
    
    // 任务提交接口
    template<typename F>
    void submitNetworkTask(F &&f, ThreadPool::TaskPriority priority = ThreadPool::NormalPriority);
    
    template<typename F>
    void submitMessageTask(F &&f, ThreadPool::TaskPriority priority = ThreadPool::NormalPriority);
    
    template<typename F>
    void submitDatabaseTask(F &&f, ThreadPool::TaskPriority priority = ThreadPool::NormalPriority);
    
    template<typename F>
    void submitFileTask(F &&f, ThreadPool::TaskPriority priority = ThreadPool::NormalPriority);
    
    template<typename F>
    void submitServiceTask(F &&f, ThreadPool::TaskPriority priority = ThreadPool::NormalPriority);
    
    // 带返回值的任务提交
    template<typename F, typename R = std::invoke_result_t<F>>
    std::future<R> submitDatabaseTaskWithResult(F &&f, ThreadPool::TaskPriority priority = ThreadPool::NormalPriority);
    
    // 统计信息
    SystemStats getSystemStats() const;
    ThreadPool::TaskStats getPoolStats(PoolType type) const;
    void resetAllStats();
    
    // 监控和诊断
    bool isHealthy() const;
    QString getHealthReport() const;
    void enableMonitoring(bool enabled);

signals:
    void poolOverloaded(PoolType type);
    void systemOverloaded();
    void taskCompleted(PoolType type);
    void taskFailed(PoolType type);
    void healthStatusChanged(bool healthy);

private slots:
    void onPoolOverloaded();
    void onTaskCompleted();
    void onTaskFailed();
    void checkSystemHealth();

private:
    explicit ThreadManager(QObject *parent = nullptr);
    ~ThreadManager();
    
    static ThreadManager* s_instance;
    static QMutex s_instanceMutex;
    
    // 线程池实例
    std::unique_ptr<ThreadPool> m_networkPool;
    std::unique_ptr<ThreadPool> m_messagePool;
    std::unique_ptr<ThreadPool> m_databasePool;
    std::unique_ptr<ThreadPool> m_filePool;
    std::unique_ptr<ThreadPool> m_servicePool;
    
    // 配置信息
    QHash<PoolType, PoolConfig> m_poolConfigs;
    
    // 监控相关
    QTimer *m_healthTimer;
    bool m_monitoringEnabled;
    bool m_systemHealthy;
    QAtomicInt m_overloadCount{0};
    
    // 内部方法
    ThreadPool* getPool(PoolType type) const;
    void setupPool(PoolType type, ThreadPool* pool);
    void updateSystemStats();
    QString poolTypeToString(PoolType type) const;
};

// 模板方法实现
template<typename F>
void ThreadManager::submitNetworkTask(F &&f, ThreadPool::TaskPriority priority)
{
    if (m_networkPool) {
        m_networkPool->enqueue(std::forward<F>(f), priority);
    }
}

template<typename F>
void ThreadManager::submitMessageTask(F &&f, ThreadPool::TaskPriority priority)
{
    if (m_messagePool) {
        m_messagePool->enqueue(std::forward<F>(f), priority);
    }
}

template<typename F>
void ThreadManager::submitDatabaseTask(F &&f, ThreadPool::TaskPriority priority)
{
    if (m_databasePool) {
        m_databasePool->enqueue(std::forward<F>(f), priority);
    }
}

template<typename F>
void ThreadManager::submitFileTask(F &&f, ThreadPool::TaskPriority priority)
{
    if (m_filePool) {
        m_filePool->enqueue(std::forward<F>(f), priority);
    }
}

template<typename F>
void ThreadManager::submitServiceTask(F &&f, ThreadPool::TaskPriority priority)
{
    if (m_servicePool) {
        m_servicePool->enqueue(std::forward<F>(f), priority);
    }
}

template<typename F, typename R>
std::future<R> ThreadManager::submitDatabaseTaskWithResult(F &&f, ThreadPool::TaskPriority priority)
{
    if (m_databasePool) {
        return m_databasePool->submit<F, R>(std::forward<F>(f), priority);
    }
    
    // 返回一个已经设置异常的future
    std::promise<R> promise;
    promise.set_exception(std::make_exception_ptr(std::runtime_error("Database pool not available")));
    return promise.get_future();
}

#endif // THREADMANAGER_H
