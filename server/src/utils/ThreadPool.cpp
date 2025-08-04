#include "ThreadPool.h"
#include <QThread>
#include <QLoggingCategory>
#include <QDebug>

Q_LOGGING_CATEGORY(threadPool, "qkchat.server.threadpool")

ThreadPool::Task::Task(std::function<void()> &&func, TaskPriority priority, ThreadPool *pool)
    : m_func(std::move(func))
    , m_priority(priority)
    , m_pool(pool)
{
    setAutoDelete(true);
}

void ThreadPool::Task::run()
{
    if (!m_pool || m_pool->m_shutdown.loadAcquire()) {
        return;
    }

    // 使用RAII保护统计更新
    class StatsGuard {
    public:
        StatsGuard(ThreadPool* pool) : m_pool(pool), m_completed(false) {
            if (m_pool) {
                m_pool->m_stats.activeTasks.fetchAndAddOrdered(1);
                m_pool->m_stats.queuedTasks.fetchAndSubOrdered(1);
            }
        }

        ~StatsGuard() {
            if (m_pool) {
                m_pool->m_stats.activeTasks.fetchAndSubOrdered(1);
                if (!m_completed) {
                    // 如果任务没有正常完成，标记为失败
                    m_pool->updateStats(true, true);
                    try {
                        emit m_pool->taskFailed();
                    } catch (...) {
                        // 忽略信号发射异常
                    }
                }
            }
        }

        void markCompleted(bool success) {
            if (m_pool && !m_completed) {
                m_completed = true;
                m_pool->updateStats(true, !success);
                try {
                    if (success) {
                        emit m_pool->taskCompleted();
                    } else {
                        emit m_pool->taskFailed();
                    }
                } catch (...) {
                    // 忽略信号发射异常
                }
            }
        }

    private:
        ThreadPool* m_pool;
        bool m_completed;
    };

    StatsGuard guard(m_pool);

    try {
        m_func();
        guard.markCompleted(true);
    } catch (const std::exception &e) {
        qCWarning(threadPool) << "Task execution failed:" << e.what();
        guard.markCompleted(false);
    } catch (...) {
        qCWarning(threadPool) << "Task execution failed with unknown exception";
        guard.markCompleted(false);
    }
}

ThreadPool::ThreadPool(int maxThreads, QObject *parent)
    : QObject(parent)
    , m_pool(new QThreadPool(this))
    , m_adjustTimer(new QTimer(this))
    , m_autoResize(false)
    , m_loadThreshold(0.8)
    , m_minThreads(2)
    , m_maxThreads(maxThreads)
{
    m_pool->setMaxThreadCount(maxThreads);

    // 设置动态调整定时器
    m_adjustTimer->setInterval(5000); // 5秒检查一次
    connect(m_adjustTimer, &QTimer::timeout, this, &ThreadPool::adjustThreadCount);

    qCInfo(threadPool) << "ThreadPool created with" << maxThreads << "max threads";
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

void ThreadPool::shutdown()
{
    if (m_shutdown.testAndSetOrdered(0, 1)) {
        qCInfo(threadPool) << "ThreadPool shutting down...";

        m_adjustTimer->stop();
        m_pool->clear();
        m_pool->waitForDone(30000); // 最多等待30秒

        qCInfo(threadPool) << "ThreadPool shutdown complete";
    }
}

void ThreadPool::setMaxThreadCount(int maxThreads)
{
    m_maxThreads = maxThreads;
    m_pool->setMaxThreadCount(maxThreads);
    qCInfo(threadPool) << "Max thread count set to" << maxThreads;
}

int ThreadPool::maxThreadCount() const
{
    return m_pool->maxThreadCount();
}

int ThreadPool::activeThreadCount() const
{
    return m_pool->activeThreadCount();
}

ThreadPool::TaskStats ThreadPool::getStats() const
{
    return m_stats;
}

void ThreadPool::resetStats()
{
    m_stats.totalTasks.storeRelease(0);
    m_stats.completedTasks.storeRelease(0);
    m_stats.failedTasks.storeRelease(0);
    m_stats.activeTasks.storeRelease(0);
    m_stats.queuedTasks.storeRelease(0);
    qCInfo(threadPool) << "ThreadPool stats reset";
}

void ThreadPool::setAutoResize(bool enabled)
{
    m_autoResize = enabled;
    if (enabled) {
        m_adjustTimer->start();
        qCInfo(threadPool) << "Auto-resize enabled";
    } else {
        m_adjustTimer->stop();
        qCInfo(threadPool) << "Auto-resize disabled";
    }
}

void ThreadPool::setLoadThreshold(double threshold)
{
    m_loadThreshold = qBound(0.1, threshold, 1.0);
    qCInfo(threadPool) << "Load threshold set to" << m_loadThreshold;
}

void ThreadPool::adjustThreadCount()
{
    if (!m_autoResize || m_shutdown.loadAcquire()) {
        return;
    }

    int currentThreads = m_pool->maxThreadCount();
    int activeThreads = m_pool->activeThreadCount();
    int queuedTasks = m_stats.queuedTasks.loadAcquire();

    double loadRatio = static_cast<double>(activeThreads) / currentThreads;

    // 如果负载过高且有排队任务，增加线程
    if (loadRatio > m_loadThreshold && queuedTasks > 0 && currentThreads < m_maxThreads) {
        int newThreadCount = qMin(currentThreads + 2, m_maxThreads);
        m_pool->setMaxThreadCount(newThreadCount);
        qCInfo(threadPool) << "Increased thread count to" << newThreadCount
                          << "(load:" << loadRatio << ", queued:" << queuedTasks << ")";
        emit poolOverloaded();
    }
    // 如果负载较低，减少线程
    else if (loadRatio < 0.3 && currentThreads > m_minThreads) {
        int newThreadCount = qMax(currentThreads - 1, m_minThreads);
        m_pool->setMaxThreadCount(newThreadCount);
        qCInfo(threadPool) << "Decreased thread count to" << newThreadCount
                          << "(load:" << loadRatio << ")";
    }
}

void ThreadPool::updateStats(bool completed, bool failed)
{
    if (completed) {
        m_stats.completedTasks.fetchAndAddOrdered(1);
    }
    if (failed) {
        m_stats.failedTasks.fetchAndAddOrdered(1);
    }
}