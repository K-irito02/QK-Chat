#include "ThreadPool.h"

ThreadPool::ThreadPool(int maxThreadCount, QObject *parent)
    : QObject(parent)
{
    _threadPool = new QThreadPool(this);
    _threadPool->setMaxThreadCount(maxThreadCount);
}

ThreadPool::~ThreadPool()
{
    shutdown();
}

void ThreadPool::shutdown()
{
    _threadPool->clear();
    _threadPool->waitForDone();
}