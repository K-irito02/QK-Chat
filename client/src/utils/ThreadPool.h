#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <QObject>
#include <QThreadPool>
#include <QRunnable>
#include <functional>

class ThreadPool : public QObject
{
    Q_OBJECT
public:
    explicit ThreadPool(int maxThreadCount = QThread::idealThreadCount(), QObject *parent = nullptr);
    ~ThreadPool();

    // 简化的enqueue方法，只接受void函数
    void enqueue(std::function<void()> func);

    void shutdown();

private:
    QThreadPool *_threadPool;
};

// Template implementation must be in the header file

class TaskRunnable : public QRunnable
{
public:
    TaskRunnable(std::function<void()> func) : m_func(func) {}
    void run() override { m_func(); }
private:
    std::function<void()> m_func;
};

inline void ThreadPool::enqueue(std::function<void()> func)
{
    auto runnable = new TaskRunnable(func);
    runnable->setAutoDelete(true);
    _threadPool->start(runnable);
}

#endif // THREADPOOL_H