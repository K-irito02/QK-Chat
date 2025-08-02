#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <QObject>
#include <QRunnable>
#include <QThreadPool>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <functional>

class ThreadPool : public QObject
{
    Q_OBJECT

public:
    explicit ThreadPool(int maxThreads = QThread::idealThreadCount(), QObject *parent = nullptr);
    ~ThreadPool();

    template<typename F>
    void enqueue(F &&f);

    void shutdown();

private:
    class Task : public QRunnable
    {
    public:
        Task(std::function<void()> &&func) : m_func(std::move(func)) {
            setAutoDelete(true);
        }
        void run() override { m_func(); }

    private:
        std::function<void()> m_func;
    };

    QThreadPool *m_pool;
    bool m_shutdown;
};

template<typename F>
void ThreadPool::enqueue(F &&f)
{
    if (m_shutdown) {
        return;
    }
    m_pool->start(new Task(std::forward<F>(f)));
}

#endif // THREADPOOL_H