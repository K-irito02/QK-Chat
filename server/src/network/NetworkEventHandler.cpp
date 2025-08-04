#include "NetworkEventHandler.h"
#include <QDebug>
#include <QCoreApplication>
#include <algorithm>
#include <QPointer>

Q_LOGGING_CATEGORY(networkEventHandler, "qkchat.server.networkeventhandler")

NetworkEventHandler::NetworkEventHandler(ConnectionManager* connectionManager, QObject *parent)
    : QObject(parent)
    , m_connectionManager(connectionManager)
    , m_threadManager(ThreadManager::instance())
    , m_processingTimer(new QTimer(this))
    , m_performanceTimer(new QTimer(this))
    , m_lastProcessingTime(QDateTime::currentDateTime())
{
    // 设置处理定时器
    m_processingTimer->setInterval(m_processingInterval.loadAcquire());
    connect(m_processingTimer, &QTimer::timeout, this, &NetworkEventHandler::processEventBatch);
    
    // 设置性能监控定时器
    m_performanceTimer->setInterval(5000); // 5秒检查一次
    connect(m_performanceTimer, &QTimer::timeout, this, &NetworkEventHandler::checkPerformance);
    
    qCInfo(networkEventHandler) << "NetworkEventHandler created";
}

NetworkEventHandler::~NetworkEventHandler()
{
    shutdown();
    qCInfo(networkEventHandler) << "NetworkEventHandler destroyed";
}

bool NetworkEventHandler::initialize()
{
    qCInfo(networkEventHandler) << "Initializing NetworkEventHandler...";
    
    if (!m_connectionManager) {
        qCCritical(networkEventHandler) << "ConnectionManager is null";
        return false;
    }
    
    if (!m_threadManager) {
        qCCritical(networkEventHandler) << "ThreadManager is null";
        return false;
    }
    
    // 启动定时器
    m_processingTimer->start();
    m_performanceTimer->start();
    
    qCInfo(networkEventHandler) << "NetworkEventHandler initialized successfully";
    return true;
}

void NetworkEventHandler::shutdown()
{
    qCInfo(networkEventHandler) << "Shutting down NetworkEventHandler...";
    
    m_processingTimer->stop();
    m_performanceTimer->stop();
    
    // 处理剩余事件
    while (!m_eventQueue.empty()) {
        processEventBatch();
        QCoreApplication::processEvents();
    }
    
    qCInfo(networkEventHandler) << "NetworkEventHandler shutdown complete";
}

void NetworkEventHandler::submitEvent(const NetworkEvent& event)
{
    if (!isEventValid(event)) {
        qCWarning(networkEventHandler) << "Invalid event submitted";
        updateEventStats(event.type, false);
        return;
    }
    
    // 检查队列大小
    if (m_eventQueue.size() >= m_maxQueueSize.loadAcquire()) {
        qCWarning(networkEventHandler) << "Event queue overflow, dropping event";
        updateEventStats(event.type, false);
        m_stats.droppedEvents.fetchAndAddOrdered(1);
        emit eventDropped(event.type);
        emit queueOverflow();
        return;
    }
    
    m_eventQueue.enqueue(event);
    m_stats.totalEvents.fetchAndAddOrdered(1);
    updateEventStats(event.type, true);
    
    qCDebug(networkEventHandler) << "Event submitted:" << static_cast<int>(event.type);
}

void NetworkEventHandler::submitNewConnection(QSslSocket* socket)
{
    submitEvent(NetworkEvent(NetworkEventType::NewConnection, socket));
}

void NetworkEventHandler::submitDataReceived(QSslSocket* socket, const QByteArray& data)
{
    submitEvent(NetworkEvent(NetworkEventType::DataReceived, socket, data));
}

void NetworkEventHandler::submitConnectionClosed(QSslSocket* socket)
{
    submitEvent(NetworkEvent(NetworkEventType::ConnectionClosed, socket));
}

void NetworkEventHandler::submitSslError(QSslSocket* socket, const QString& error)
{
    submitEvent(NetworkEvent(NetworkEventType::SslError, socket, error));
}

void NetworkEventHandler::submitSocketError(QSslSocket* socket, const QString& error)
{
    submitEvent(NetworkEvent(NetworkEventType::SocketError, socket, error));
}

void NetworkEventHandler::submitHeartbeat(QSslSocket* socket)
{
    submitEvent(NetworkEvent(NetworkEventType::Heartbeat, socket));
}

void NetworkEventHandler::submitEvents(const QList<NetworkEvent>& events)
{
    for (const auto& event : events) {
        submitEvent(event);
    }
}

void NetworkEventHandler::setMaxQueueSize(int maxSize)
{
    m_maxQueueSize.storeRelease(maxSize);
    qCInfo(networkEventHandler) << "Max queue size set to" << maxSize;
}

void NetworkEventHandler::setBatchSize(int batchSize)
{
    m_batchSize.storeRelease(batchSize);
    qCInfo(networkEventHandler) << "Batch size set to" << batchSize;
}

void NetworkEventHandler::setProcessingInterval(int milliseconds)
{
    m_processingInterval.storeRelease(milliseconds);
    m_processingTimer->setInterval(milliseconds);
    qCInfo(networkEventHandler) << "Processing interval set to" << milliseconds << "ms";
}

void NetworkEventHandler::setMaxProcessingTime(int milliseconds)
{
    m_maxProcessingTime.storeRelease(milliseconds);
    qCInfo(networkEventHandler) << "Max processing time set to" << milliseconds << "ms";
}

NetworkEventHandler::EventStats NetworkEventHandler::getStats() const
{
    return m_stats;
}

void NetworkEventHandler::resetStats()
{
    m_stats.totalEvents.storeRelease(0);
    m_stats.processedEvents.storeRelease(0);
    m_stats.droppedEvents.storeRelease(0);
    m_stats.queuedEvents.storeRelease(0);
    m_stats.processingErrors.storeRelease(0);
    
    m_stats.newConnections.storeRelease(0);
    m_stats.dataReceived.storeRelease(0);
    m_stats.connectionsClosed.storeRelease(0);
    m_stats.sslErrors.storeRelease(0);
    m_stats.socketErrors.storeRelease(0);
    m_stats.heartbeats.storeRelease(0);
    
    qCInfo(networkEventHandler) << "Event stats reset";
}

int NetworkEventHandler::getQueueSize() const
{
    return m_eventQueue.size();
}

bool NetworkEventHandler::isOverloaded() const
{
    int queueSize = getQueueSize();
    int maxSize = m_maxQueueSize.loadAcquire();
    return queueSize > (maxSize * 0.8); // 80%阈值
}

void NetworkEventHandler::enableBatchProcessing(bool enabled)
{
    m_batchProcessingEnabled.storeRelease(enabled ? 1 : 0);
    qCInfo(networkEventHandler) << "Batch processing" << (enabled ? "enabled" : "disabled");
}

void NetworkEventHandler::setLoadBalancing(bool enabled)
{
    m_loadBalancingEnabled.storeRelease(enabled ? 1 : 0);
    qCInfo(networkEventHandler) << "Load balancing" << (enabled ? "enabled" : "disabled");
}

void NetworkEventHandler::setPriorityProcessing(bool enabled)
{
    m_priorityProcessingEnabled.storeRelease(enabled ? 1 : 0);
    qCInfo(networkEventHandler) << "Priority processing" << (enabled ? "enabled" : "disabled");
}

void NetworkEventHandler::processEventBatch()
{
    QDateTime startTime = QDateTime::currentDateTime();
    int maxProcessingTime = m_maxProcessingTime.loadAcquire();
    int batchSize = m_batchSize.loadAcquire();
    
    if (m_batchProcessingEnabled.loadAcquire()) {
        // 批量处理
        auto events = dequeueEvents(batchSize);
        if (!events.isEmpty()) {
            processBatch(events);
        }
    } else {
        // 单个处理
        NetworkEvent event;
        int processed = 0;
        
        while (m_eventQueue.dequeue(event) && processed < batchSize) {
            processEvent(event);
            processed++;
            
            // 检查处理时间
            if (startTime.msecsTo(QDateTime::currentDateTime()) > maxProcessingTime) {
                break;
            }
        }
    }
    
    // 更新性能指标
    m_lastProcessingTime = QDateTime::currentDateTime();
    m_processingLatency.storeRelease(startTime.msecsTo(m_lastProcessingTime));
}

void NetworkEventHandler::checkPerformance()
{
    updatePerformanceMetrics();
    checkQueueHealth();
    
    // 检查处理延迟
    int latency = m_processingLatency.loadAcquire();
    if (latency > 100) { // 100ms阈值
        logPerformanceWarning(QString("High processing latency: %1ms").arg(latency));
    }
    
    // 检查队列健康状态
    if (isOverloaded()) {
        emit performanceAlert("Event queue is overloaded");
    }
}

void NetworkEventHandler::processEvent(const NetworkEvent& event)
{
    try {
        switch (event.type) {
        case NetworkEventType::NewConnection:
            processNewConnection(event.socket);
            break;
        case NetworkEventType::DataReceived:
            processDataReceived(event.socket, event.data);
            break;
        case NetworkEventType::ConnectionClosed:
            processConnectionClosed(event.socket);
            break;
        case NetworkEventType::SslError:
            processSslError(event.socket, event.errorMessage);
            break;
        case NetworkEventType::SocketError:
            processSocketError(event.socket, event.errorMessage);
            break;
        case NetworkEventType::Heartbeat:
            processHeartbeat(event.socket);
            break;
        default:
            qCWarning(networkEventHandler) << "Unknown event type:" << static_cast<int>(event.type);
            return;
        }
        
        m_stats.processedEvents.fetchAndAddOrdered(1);
        emit eventProcessed(event.type);
        
    } catch (const std::exception& e) {
        handleProcessingError(event, QString("Exception: %1").arg(e.what()));
    } catch (...) {
        handleProcessingError(event, "Unknown exception");
    }
}

void NetworkEventHandler::processNewConnection(QSslSocket* socket)
{
    if (!socket) {
        qCWarning(networkEventHandler) << "Null socket in new connection event";
        return;
    }

    // 使用智能指针管理socket生命周期
    QPointer<QSslSocket> socketPtr(socket);
    auto weakConnectionManager = QPointer<ConnectionManager>(m_connectionManager);
    auto weakSelf = QPointer<NetworkEventHandler>(this);

    if (m_threadManager) {
        m_threadManager->submitNetworkTask([weakSelf, weakConnectionManager, socketPtr]() {
            // 检查所有对象是否仍然有效
            if (auto self = weakSelf.data()) {
                if (auto connectionManager = weakConnectionManager.data()) {
                    if (auto socket = socketPtr.data()) {
                        try {
                            bool success = connectionManager->addConnection(socket);
                            if (success) {
                                qCDebug(networkEventHandler) << "New connection processed successfully";
                            } else {
                                qCWarning(networkEventHandler) << "Failed to add new connection";
                            }
                        } catch (const std::exception& e) {
                            qCCritical(networkEventHandler) << "Exception in connection processing:" << e.what();
                        } catch (...) {
                            qCCritical(networkEventHandler) << "Unknown exception in connection processing";
                        }
                    }
                }
            }
        }, ThreadPool::HighPriority);
    }
}

void NetworkEventHandler::processDataReceived(QSslSocket* socket, const QByteArray& data)
{
    if (!socket || data.isEmpty()) {
        qCWarning(networkEventHandler) << "Invalid data received event";
        return;
    }

    // 使用智能指针管理对象生命周期
    QPointer<QSslSocket> socketPtr(socket);
    auto weakConnectionManager = QPointer<ConnectionManager>(m_connectionManager);
    auto weakSelf = QPointer<NetworkEventHandler>(this);

    if (m_threadManager) {
        m_threadManager->submitMessageTask([weakSelf, weakConnectionManager, socketPtr, data]() {
            if (auto self = weakSelf.data()) {
                if (auto connectionManager = weakConnectionManager.data()) {
                    if (auto socket = socketPtr.data()) {
                        try {
                            auto connection = connectionManager->getConnection(socket);
                            if (connection) {
                                connection->updateActivity();
                                connection->incrementMessagesReceived();
                                connection->addBytesTransferred(data.size());

                                // 这里可以添加具体的消息处理逻辑
                                qCDebug(networkEventHandler) << "Data processed for connection:" << connection->getUserId();
                            }
                        } catch (const std::exception& e) {
                            qCCritical(networkEventHandler) << "Exception in data processing:" << e.what();
                        } catch (...) {
                            qCCritical(networkEventHandler) << "Unknown exception in data processing";
                        }
                    }
                }
            }
        });
    }
}

void NetworkEventHandler::processConnectionClosed(QSslSocket* socket)
{
    if (!socket) {
        qCWarning(networkEventHandler) << "Null socket in connection closed event";
        return;
    }

    // 使用智能指针管理对象生命周期
    QPointer<QSslSocket> socketPtr(socket);
    auto weakConnectionManager = QPointer<ConnectionManager>(m_connectionManager);
    auto weakSelf = QPointer<NetworkEventHandler>(this);

    if (m_threadManager) {
        m_threadManager->submitNetworkTask([weakSelf, weakConnectionManager, socketPtr]() {
            if (auto self = weakSelf.data()) {
                if (auto connectionManager = weakConnectionManager.data()) {
                    if (auto socket = socketPtr.data()) {
                        try {
                            bool success = connectionManager->removeConnection(socket);
                            if (success) {
                                qCDebug(networkEventHandler) << "Connection closed processed successfully";
                            } else {
                                qCWarning(networkEventHandler) << "Failed to remove closed connection";
                            }
                        } catch (const std::exception& e) {
                            qCCritical(networkEventHandler) << "Exception in connection close processing:" << e.what();
                        } catch (...) {
                            qCCritical(networkEventHandler) << "Unknown exception in connection close processing";
                        }
                    }
                }
            }
        });
    }
}

void NetworkEventHandler::processSslError(QSslSocket* socket, const QString& error)
{
    qCWarning(networkEventHandler) << "SSL error for socket:" << socket << "Error:" << error;
    
    // 可以根据错误类型决定是否关闭连接
    if (socket && m_connectionManager) {
        m_connectionManager->setConnectionState(socket, ConnectionState::Error);
    }
}

void NetworkEventHandler::processSocketError(QSslSocket* socket, const QString& error)
{
    qCWarning(networkEventHandler) << "Socket error for socket:" << socket << "Error:" << error;
    
    // 处理socket错误，通常需要关闭连接
    if (socket && m_connectionManager) {
        m_connectionManager->setConnectionState(socket, ConnectionState::Error);
        m_connectionManager->removeConnection(socket);
    }
}

void NetworkEventHandler::processHeartbeat(QSslSocket* socket)
{
    if (!socket) {
        return;
    }
    
    // 更新连接活动时间
    if (m_connectionManager) {
        m_connectionManager->updateConnectionActivity(socket);
    }
    
    qCDebug(networkEventHandler) << "Heartbeat processed for socket:" << socket;
}

QList<NetworkEvent> NetworkEventHandler::dequeueEvents(int maxCount)
{
    QList<NetworkEvent> events;
    NetworkEvent event;
    
    for (int i = 0; i < maxCount && m_eventQueue.dequeue(event); ++i) {
        events.append(event);
    }
    
    return events;
}

void NetworkEventHandler::processBatch(const QList<NetworkEvent>& events)
{
    if (events.isEmpty()) {
        return;
    }
    
    QList<NetworkEvent> sortedEvents = events;
    
    // 如果启用优先级处理，对事件排序
    if (m_priorityProcessingEnabled.loadAcquire()) {
        sortEventsByPriority(sortedEvents);
    }
    
    // 如果启用负载均衡，分发到不同线程
    if (m_loadBalancingEnabled.loadAcquire()) {
        distributeLoad(sortedEvents);
    } else {
        // 顺序处理
        for (const auto& event : sortedEvents) {
            processEvent(event);
        }
    }
}

int NetworkEventHandler::getEventPriority(NetworkEventType type) const
{
    switch (type) {
    case NetworkEventType::NewConnection: return 3;
    case NetworkEventType::ConnectionClosed: return 3;
    case NetworkEventType::SslError: return 2;
    case NetworkEventType::SocketError: return 2;
    case NetworkEventType::DataReceived: return 1;
    case NetworkEventType::Heartbeat: return 0;
    default: return 0;
    }
}

void NetworkEventHandler::sortEventsByPriority(QList<NetworkEvent>& events) const
{
    std::sort(events.begin(), events.end(), [this](const NetworkEvent& a, const NetworkEvent& b) {
        return getEventPriority(a.type) > getEventPriority(b.type);
    });
}

void NetworkEventHandler::distributeLoad(const QList<NetworkEvent>& events)
{
    // 简单的负载分发策略：按事件类型分发到不同线程池
    for (const auto& event : events) {
        switch (event.type) {
        case NetworkEventType::NewConnection:
        case NetworkEventType::ConnectionClosed:
            m_threadManager->submitNetworkTask([this, event]() {
                processEvent(event);
            });
            break;
        case NetworkEventType::DataReceived:
            m_threadManager->submitMessageTask([this, event]() {
                processEvent(event);
            });
            break;
        default:
            processEvent(event); // 在当前线程处理
            break;
        }
    }
}

void NetworkEventHandler::updatePerformanceMetrics()
{
    m_stats.queuedEvents.storeRelease(getQueueSize());
}

void NetworkEventHandler::checkQueueHealth()
{
    int queueSize = getQueueSize();
    int maxSize = m_maxQueueSize.loadAcquire();
    
    if (queueSize > maxSize * 0.9) {
        logPerformanceWarning(QString("Queue nearly full: %1/%2").arg(queueSize).arg(maxSize));
    }
}

void NetworkEventHandler::logPerformanceWarning(const QString& message)
{
    qCWarning(networkEventHandler) << "Performance warning:" << message;
    emit performanceAlert(message);
}

void NetworkEventHandler::updateEventStats(NetworkEventType type, bool processed)
{
    if (!processed) {
        return;
    }
    
    switch (type) {
    case NetworkEventType::NewConnection:
        m_stats.newConnections.fetchAndAddOrdered(1);
        break;
    case NetworkEventType::DataReceived:
        m_stats.dataReceived.fetchAndAddOrdered(1);
        break;
    case NetworkEventType::ConnectionClosed:
        m_stats.connectionsClosed.fetchAndAddOrdered(1);
        break;
    case NetworkEventType::SslError:
        m_stats.sslErrors.fetchAndAddOrdered(1);
        break;
    case NetworkEventType::SocketError:
        m_stats.socketErrors.fetchAndAddOrdered(1);
        break;
    case NetworkEventType::Heartbeat:
        m_stats.heartbeats.fetchAndAddOrdered(1);
        break;
    }
}

void NetworkEventHandler::incrementCounter(QAtomicInt& counter)
{
    counter.fetchAndAddOrdered(1);
}

void NetworkEventHandler::handleProcessingError(const NetworkEvent& event, const QString& error)
{
    qCCritical(networkEventHandler) << "Error processing event type" 
                                   << static_cast<int>(event.type) << ":" << error;
    
    m_stats.processingErrors.fetchAndAddOrdered(1);
    emit processingError(error);
}

bool NetworkEventHandler::isEventValid(const NetworkEvent& event) const
{
    // 基本验证
    if (event.type < NetworkEventType::NewConnection || event.type > NetworkEventType::Heartbeat) {
        return false;
    }
    
    // 对于需要socket的事件，检查socket是否有效
    switch (event.type) {
    case NetworkEventType::NewConnection:
    case NetworkEventType::DataReceived:
    case NetworkEventType::ConnectionClosed:
    case NetworkEventType::SslError:
    case NetworkEventType::SocketError:
    case NetworkEventType::Heartbeat:
        return event.socket != nullptr;
    default:
        return true;
    }
}
