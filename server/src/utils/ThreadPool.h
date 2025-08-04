#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <QObject>
#include <QRunnable>
#include <QThreadPool>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <QTimer>
#include <functional>
#include <memory>

/**
 * @brief 高性能线程池实现
 *
 * 特性：
 * - 支持任务优先级
 * - 动态线程数调整
 * - 任务统计和监控
 * - 优雅关闭机制
 */
class ThreadPool : public QObject
{
    Q_OBJECT

public:
    enum TaskPriority {
        LowPriority = 0,
        NormalPriority = 1,
        HighPriority = 2,
        CriticalPriority = 3
    };

    struct TaskStats {
        QAtomicInt totalTasks{0};
        QAtomicInt completedTasks{0};
        QAtomicInt failedTasks{0};
        QAtomicInt activeTasks{0};
        QAtomicInt queuedTasks{0};
    };

    explicit ThreadPool(int maxThreads = QThread::idealThreadCount(), QObject *parent = nullptr);
    ~ThreadPool();

    // 基本任务提交
    template<typename F>
    void enqueue(F &&f, TaskPriority priority = NormalPriority);

    // 带返回值的任务提交
    template<typename F, typename R = std::invoke_result_t<F>>
    std::future<R> submit(F &&f, TaskPriority priority = NormalPriority);

    // 线程池控制
    void shutdown();
    void setMaxThreadCount(int maxThreads);
    int maxThreadCount() const;
    int activeThreadCount() const;

    // 统计信息
    TaskStats getStats() const;
    void resetStats();

    // 动态调整
    void setAutoResize(bool enabled);
    void setLoadThreshold(double threshold);

signals:
    void taskCompleted();
    void taskFailed();
    void poolOverloaded();

private slots:
    void adjustThreadCount();

private:
    class Task : public QRunnable
    {
    public:
        Task(std::function<void()> &&func, TaskPriority priority, ThreadPool *pool);
        void run() override;
        TaskPriority priority() const { return m_priority; }

    private:
        std::function<void()> m_func;
        TaskPriority m_priority;
        ThreadPool *m_pool;
    };

    QThreadPool *m_pool;
    QAtomicInt m_shutdown{0};
    TaskStats m_stats;

    // 动态调整相关
    QTimer *m_adjustTimer;
    bool m_autoResize;
    double m_loadThreshold;
    int m_minThreads;
    int m_maxThreads;

    void updateStats(bool completed, bool failed = false);
    friend class Task;
};

template<typename F>
void ThreadPool::enqueue(F &&f, TaskPriority priority)
{
    if (m_shutdown.loadAcquire()) {
        return;
    }

    m_stats.totalTasks.fetchAndAddOrdered(1);
    m_stats.queuedTasks.fetchAndAddOrdered(1);

    auto task = new Task(std::forward<F>(f), priority, this);
    m_pool->start(task, static_cast<int>(priority));
}

template<typename F, typename R>
std::future<R> ThreadPool::submit(F &&f, TaskPriority priority)
{
    auto promise = std::make_shared<std::promise<R>>();
    auto future = promise->get_future();

    enqueue([promise, f = std::forward<F>(f)]() mutable {
        try {
            if constexpr (std::is_void_v<R>) {
                f();
                promise->set_value();
            } else {
                promise->set_value(f());
            }
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    }, priority);

    return future;
}

#endif // THREADPOOL_H