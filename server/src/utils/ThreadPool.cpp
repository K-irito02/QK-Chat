#include "ThreadPool.h"
#include <QThread>

ThreadPool::ThreadPool(int maxThreads, QObject *parent)
    : QObject(parent)
    , m_shutdown(false)
{
    m_pool = new QThreadPool(this);
    m_pool->setMaxThreadCount(maxThreads);
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

void ThreadPool::shutdown()
{
    if (m_shutdown) {
        return;
    }
    m_shutdown = true;
    m_pool->clear();
    m_pool->waitForDone();
}