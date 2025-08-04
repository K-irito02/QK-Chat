#include "MessageEngine.h"
#include <QDebug>
#include <QUuid>
#include <QCoreApplication>
#include <algorithm>
#include <QPointer>

Q_LOGGING_CATEGORY(messageEngine, "qkchat.server.messageengine")

MessageEngine::MessageEngine(ConnectionManager* connectionManager, QObject *parent)
    : QObject(parent)
    , m_connectionManager(connectionManager)
    , m_threadManager(ThreadManager::instance())
    , m_processingTimer(new QTimer(this))
    , m_cleanupTimer(new QTimer(this))
    , m_metricsTimer(new QTimer(this))
    , m_lastMetricsUpdate(QDateTime::currentDateTime())
{
    // 设置处理定时器
    m_processingTimer->setInterval(m_processingInterval.loadAcquire());
    connect(m_processingTimer, &QTimer::timeout, this, &MessageEngine::processMessageBatch);
    
    // 设置清理定时器 - 每分钟清理一次过期消息
    m_cleanupTimer->setInterval(60000);
    connect(m_cleanupTimer, &QTimer::timeout, this, &MessageEngine::cleanupExpiredMessages);
    
    // 设置性能监控定时器 - 每秒更新一次
    m_metricsTimer->setInterval(1000);
    connect(m_metricsTimer, &QTimer::timeout, this, &MessageEngine::updatePerformanceMetrics);
    
    qCInfo(messageEngine) << "MessageEngine created";
}

MessageEngine::~MessageEngine()
{
    shutdown();
    qCInfo(messageEngine) << "MessageEngine destroyed";
}

bool MessageEngine::initialize()
{
    qCInfo(messageEngine) << "Initializing MessageEngine...";
    
    if (!m_connectionManager) {
        qCCritical(messageEngine) << "ConnectionManager is null";
        return false;
    }
    
    if (!m_threadManager) {
        qCCritical(messageEngine) << "ThreadManager is null";
        return false;
    }
    
    // 加载持久化消息
    if (m_messagePersistenceEnabled.loadAcquire()) {
        loadPersistedMessages();
    }
    
    // 启动定时器
    m_processingTimer->start();
    m_cleanupTimer->start();
    m_metricsTimer->start();
    
    qCInfo(messageEngine) << "MessageEngine initialized successfully";
    return true;
}

void MessageEngine::shutdown()
{
    qCInfo(messageEngine) << "Shutting down MessageEngine...";
    
    m_processingTimer->stop();
    m_cleanupTimer->stop();
    m_metricsTimer->stop();
    
    // 处理剩余消息
    while (getTotalQueueSize() > 0) {
        processMessageBatch();
        QCoreApplication::processEvents();
    }
    
    qCInfo(messageEngine) << "MessageEngine shutdown complete";
}

bool MessageEngine::submitMessage(const Message& message)
{
    if (!validateMessage(message)) {
        qCWarning(messageEngine) << "Invalid message submitted:" << message.id;
        return false;
    }
    
    // 检查队列大小
    if (getTotalQueueSize() >= m_maxQueueSize.loadAcquire()) {
        qCWarning(messageEngine) << "Message queue overflow, dropping message:" << message.id;
        m_stats.failedMessages.fetchAndAddOrdered(1);
        emit queueOverflow();
        return false;
    }
    
    enqueueMessage(message);
    m_stats.totalMessages.fetchAndAddOrdered(1);
    updateMessageStats(message.type, true);
    
    logMessageEvent("MESSAGE_SUBMITTED", message);
    return true;
}

bool MessageEngine::submitMessage(MessageType type, qint64 fromUserId, qint64 toUserId, 
                                 const QJsonObject& data, MessagePriority priority)
{
    Message message;
    message.id = generateMessageId();
    message.type = type;
    message.priority = priority;
    message.fromUserId = fromUserId;
    message.toUserId = toUserId;
    message.data = data;
    message.timestamp = QDateTime::currentDateTime();
    
    // 设置过期时间
    if (m_messageTTL.loadAcquire() > 0) {
        message.expiresAt = message.timestamp.addSecs(m_messageTTL.loadAcquire());
    }
    
    return submitMessage(message);
}

bool MessageEngine::submitMessages(const QList<Message>& messages)
{
    bool allSuccess = true;
    for (const auto& message : messages) {
        if (!submitMessage(message)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

void MessageEngine::registerHandler(std::shared_ptr<MessageHandler> handler)
{
    if (!handler) {
        qCWarning(messageEngine) << "Cannot register null handler";
        return;
    }
    
    QString handlerName = handler->handlerName();
    m_handlers.insert(handlerName, handler);
    
    // 更新类型处理器映射
    for (int i = static_cast<int>(MessageType::Unknown); i <= static_cast<int>(MessageType::EmailVerification); ++i) {
        MessageType type = static_cast<MessageType>(i);
        if (handler->canHandle(type)) {
            QStringList handlers = m_typeHandlers.value(type);
            if (!handlers.contains(handlerName)) {
                handlers.append(handlerName);
                m_typeHandlers.insert(type, handlers);
            }
        }
    }
    
    qCInfo(messageEngine) << "Handler registered:" << handlerName;
}

void MessageEngine::unregisterHandler(const QString& handlerName)
{
    if (m_handlers.remove(handlerName)) {
        // 从类型处理器映射中移除
        m_typeHandlers.forEach([handlerName](MessageType type, QStringList handlers) {
            handlers.removeAll(handlerName);
            // 注意：这里需要重新插入更新后的列表
        });
        
        qCInfo(messageEngine) << "Handler unregistered:" << handlerName;
    }
}

QStringList MessageEngine::getRegisteredHandlers() const
{
    return m_handlers.keys();
}

void MessageEngine::setMaxQueueSize(int maxSize)
{
    m_maxQueueSize.storeRelease(maxSize);
    qCInfo(messageEngine) << "Max queue size set to" << maxSize;
}

void MessageEngine::setBatchSize(int batchSize)
{
    m_batchSize.storeRelease(batchSize);
    qCInfo(messageEngine) << "Batch size set to" << batchSize;
}

void MessageEngine::setProcessingInterval(int milliseconds)
{
    m_processingInterval.storeRelease(milliseconds);
    m_processingTimer->setInterval(milliseconds);
    qCInfo(messageEngine) << "Processing interval set to" << milliseconds << "ms";
}

void MessageEngine::setMaxRetryCount(int maxRetries)
{
    m_maxRetryCount.storeRelease(maxRetries);
    qCInfo(messageEngine) << "Max retry count set to" << maxRetries;
}

void MessageEngine::setMessageTTL(int seconds)
{
    m_messageTTL.storeRelease(seconds);
    qCInfo(messageEngine) << "Message TTL set to" << seconds << "seconds";
}

MessageEngine::EngineStats MessageEngine::getStats() const
{
    return m_stats;
}

void MessageEngine::resetStats()
{
    m_stats.totalMessages.storeRelease(0);
    m_stats.processedMessages.storeRelease(0);
    m_stats.failedMessages.storeRelease(0);
    m_stats.queuedMessages.storeRelease(0);
    m_stats.expiredMessages.storeRelease(0);
    m_stats.retriedMessages.storeRelease(0);
    
    m_stats.loginMessages.storeRelease(0);
    m_stats.chatMessages.storeRelease(0);
    m_stats.groupChatMessages.storeRelease(0);
    m_stats.heartbeatMessages.storeRelease(0);
    m_stats.systemMessages.storeRelease(0);
    
    m_stats.averageProcessingTime.storeRelease(0);
    m_stats.maxProcessingTime.storeRelease(0);
    m_stats.throughputPerSecond.storeRelease(0);
    
    m_messagesProcessedLastSecond.storeRelease(0);
    
    qCInfo(messageEngine) << "Message engine stats reset";
}

int MessageEngine::getQueueSize() const
{
    return getTotalQueueSize();
}

bool MessageEngine::isOverloaded() const
{
    int queueSize = getTotalQueueSize();
    int maxSize = m_maxQueueSize.loadAcquire();
    return queueSize > (maxSize * 0.8); // 80%阈值
}

void MessageEngine::enableBatchProcessing(bool enabled)
{
    m_batchProcessingEnabled.storeRelease(enabled ? 1 : 0);
    qCInfo(messageEngine) << "Batch processing" << (enabled ? "enabled" : "disabled");
}

void MessageEngine::enablePriorityProcessing(bool enabled)
{
    m_priorityProcessingEnabled.storeRelease(enabled ? 1 : 0);
    qCInfo(messageEngine) << "Priority processing" << (enabled ? "enabled" : "disabled");
}

void MessageEngine::enableRetryMechanism(bool enabled)
{
    m_retryMechanismEnabled.storeRelease(enabled ? 1 : 0);
    qCInfo(messageEngine) << "Retry mechanism" << (enabled ? "enabled" : "disabled");
}

void MessageEngine::enableMessagePersistence(bool enabled)
{
    m_messagePersistenceEnabled.storeRelease(enabled ? 1 : 0);
    qCInfo(messageEngine) << "Message persistence" << (enabled ? "enabled" : "disabled");
}

void MessageEngine::processMessageBatch()
{
    QDateTime startTime = QDateTime::currentDateTime();
    int batchSize = m_batchSize.loadAcquire();
    
    if (m_batchProcessingEnabled.loadAcquire()) {
        // 批量处理
        auto messages = dequeueMessages(batchSize);
        if (!messages.isEmpty()) {
            processBatch(messages);
        }
    } else {
        // 单个处理
        int processed = 0;
        while (processed < batchSize) {
            Message message = dequeueNextMessage();
            if (!message.isValid()) {
                break; // 队列为空
            }
            
            processMessage(message);
            processed++;
        }
    }
    
    // 处理重试队列
    if (m_retryMechanismEnabled.loadAcquire()) {
        processRetryQueue();
    }
    
    // 更新性能指标
    int processingTime = startTime.msecsTo(QDateTime::currentDateTime());
    updatePerformanceStats(processingTime);
}

void MessageEngine::cleanupExpiredMessages()
{
    int expiredCount = 0;
    QDateTime now = QDateTime::currentDateTime();
    
    // 这里需要实现过期消息清理逻辑
    // 由于使用无锁队列，清理过期消息比较复杂
    // 可以考虑在处理消息时检查过期时间
    
    if (expiredCount > 0) {
        m_stats.expiredMessages.fetchAndAddOrdered(expiredCount);
        qCInfo(messageEngine) << "Cleaned up" << expiredCount << "expired messages";
    }
}

void MessageEngine::updatePerformanceMetrics()
{
    QDateTime now = QDateTime::currentDateTime();
    
    // 更新吞吐量
    int messagesThisSecond = m_messagesProcessedLastSecond.fetchAndStoreOrdered(0);
    m_stats.throughputPerSecond.storeRelease(messagesThisSecond);
    
    // 更新队列大小
    m_stats.queuedMessages.storeRelease(getTotalQueueSize());
    
    // 检查是否过载
    if (isOverloaded()) {
        emit engineOverloaded();
        emit performanceAlert("Message engine is overloaded");
    }
    
    m_lastMetricsUpdate = now;
}

bool MessageEngine::processMessage(const Message& message)
{
    QDateTime startTime = QDateTime::currentDateTime();
    
    try {
        // 检查消息是否过期
        if (message.isExpired()) {
            qCDebug(messageEngine) << "Message expired:" << message.id;
            m_stats.expiredMessages.fetchAndAddOrdered(1);
            return false;
        }
        
        // 路由消息到处理器
        bool success = routeMessage(message);
        
        if (success) {
            m_stats.processedMessages.fetchAndAddOrdered(1);
            m_messagesProcessedLastSecond.fetchAndAddOrdered(1);
            emit messageProcessed(message.id, message.type);
            logMessageEvent("MESSAGE_PROCESSED", message);
        } else {
            // 如果启用重试机制，将消息加入重试队列
            if (m_retryMechanismEnabled.loadAcquire() && 
                message.retryCount < m_maxRetryCount.loadAcquire()) {
                scheduleRetry(message);
            } else {
                m_stats.failedMessages.fetchAndAddOrdered(1);
                emit messageFailed(message.id, "Processing failed");
                logMessageEvent("MESSAGE_FAILED", message);
            }
        }
        
        // 更新处理时间统计
        int processingTime = startTime.msecsTo(QDateTime::currentDateTime());
        updatePerformanceStats(processingTime);
        
        return success;
        
    } catch (const std::exception& e) {
        handleProcessingError(message, QString("Exception: %1").arg(e.what()));
        return false;
    } catch (...) {
        handleProcessingError(message, "Unknown exception");
        return false;
    }
}

QList<Message> MessageEngine::dequeueMessages(int maxCount)
{
    QList<Message> messages;
    
    if (m_priorityProcessingEnabled.loadAcquire()) {
        // 按优先级顺序处理
        int remaining = maxCount;
        
        // 关键优先级
        while (remaining > 0) {
            Message msg;
            if (!m_criticalQueue.dequeue(msg)) break;
            messages.append(msg);
            remaining--;
        }
        
        // 高优先级
        while (remaining > 0) {
            Message msg;
            if (!m_highQueue.dequeue(msg)) break;
            messages.append(msg);
            remaining--;
        }
        
        // 普通优先级
        while (remaining > 0) {
            Message msg;
            if (!m_normalQueue.dequeue(msg)) break;
            messages.append(msg);
            remaining--;
        }
        
        // 低优先级
        while (remaining > 0) {
            Message msg;
            if (!m_lowQueue.dequeue(msg)) break;
            messages.append(msg);
            remaining--;
        }
    } else {
        // 简单的轮询处理
        for (int i = 0; i < maxCount; ++i) {
            Message msg = dequeueNextMessage();
            if (!msg.isValid()) break;
            messages.append(msg);
        }
    }
    
    return messages;
}

void MessageEngine::processBatch(const QList<Message>& messages)
{
    if (messages.isEmpty()) {
        return;
    }

    // 使用weak_ptr避免循环引用和悬空指针
    auto weakSelf = QPointer<MessageEngine>(this);

    // 批量提交减少线程切换开销
    auto batchProcessor = [weakSelf, messages]() {
        if (auto self = weakSelf.data()) {
            for (const auto& message : messages) {
                if (!weakSelf.data()) break; // 检查对象是否仍然有效
                try {
                    self->processMessage(message);
                } catch (const std::exception& e) {
                    qCCritical(messageEngine) << "Exception in message processing:" << e.what();
                } catch (...) {
                    qCCritical(messageEngine) << "Unknown exception in message processing";
                }
            }
        }
    };

    if (m_threadManager) {
        m_threadManager->submitMessageTask(std::move(batchProcessor));
    }
}

void MessageEngine::enqueueMessage(const Message& message)
{
    LockFreeQueue<Message>* queue = getQueueByPriority(message.priority);
    if (queue) {
        queue->enqueue(message);
    } else {
        m_normalQueue.enqueue(message); // 默认队列
    }
}

Message MessageEngine::dequeueNextMessage()
{
    Message message;
    
    // 按优先级顺序尝试出队
    if (m_criticalQueue.dequeue(message)) return message;
    if (m_highQueue.dequeue(message)) return message;
    if (m_normalQueue.dequeue(message)) return message;
    if (m_lowQueue.dequeue(message)) return message;
    
    return Message(); // 返回无效消息
}

int MessageEngine::getTotalQueueSize() const
{
    return m_criticalQueue.size() + m_highQueue.size() + 
           m_normalQueue.size() + m_lowQueue.size() + m_retryQueue.size();
}

bool MessageEngine::routeMessage(const Message& message)
{
    QStringList handlers = findHandlers(message.type);
    if (handlers.isEmpty()) {
        qCWarning(messageEngine) << "No handlers found for message type:" << static_cast<int>(message.type);
        return false;
    }
    
    bool success = false;
    for (const QString& handlerName : handlers) {
        auto handler = m_handlers.value(handlerName);
        if (handler && handler->canHandle(message.type)) {
            try {
                if (handler->handleMessage(message)) {
                    success = true;
                    break; // 第一个成功处理的处理器
                }
            } catch (const std::exception& e) {
                qCWarning(messageEngine) << "Handler" << handlerName << "failed:" << e.what();
            }
        }
    }
    
    return success;
}

QStringList MessageEngine::findHandlers(MessageType type) const
{
    return m_typeHandlers.value(type);
}

void MessageEngine::scheduleRetry(const Message& message)
{
    Message retryMessage = message;
    retryMessage.retryCount++;
    retryMessage.timestamp = QDateTime::currentDateTime();
    
    m_retryQueue.enqueue(retryMessage);
    m_stats.retriedMessages.fetchAndAddOrdered(1);
    
    qCDebug(messageEngine) << "Message scheduled for retry:" << message.id 
                          << "Attempt:" << retryMessage.retryCount;
}

void MessageEngine::processRetryQueue()
{
    Message message;
    int processed = 0;
    int maxRetries = 10; // 每次最多处理10个重试消息
    
    while (processed < maxRetries && m_retryQueue.dequeue(message)) {
        processMessage(message);
        processed++;
    }
}

bool MessageEngine::validateMessage(const Message& message) const
{
    if (!message.isValid()) {
        return false;
    }
    
    if (message.type == MessageType::Unknown) {
        return false;
    }
    
    // 可以添加更多验证逻辑
    return true;
}

MessagePriority MessageEngine::getDefaultPriority(MessageType type) const
{
    switch (type) {
    case MessageType::Login:
    case MessageType::Logout:
        return MessagePriority::High;
    case MessageType::SystemNotification:
        return MessagePriority::Critical;
    case MessageType::Heartbeat:
        return MessagePriority::Low;
    default:
        return MessagePriority::Normal;
    }
}

void MessageEngine::updateMessageStats(MessageType type, bool processed)
{
    if (!processed) return;
    
    switch (type) {
    case MessageType::Login:
    case MessageType::Logout:
        m_stats.loginMessages.fetchAndAddOrdered(1);
        break;
    case MessageType::Chat:
        m_stats.chatMessages.fetchAndAddOrdered(1);
        break;
    case MessageType::GroupChat:
        m_stats.groupChatMessages.fetchAndAddOrdered(1);
        break;
    case MessageType::Heartbeat:
        m_stats.heartbeatMessages.fetchAndAddOrdered(1);
        break;
    case MessageType::SystemNotification:
        m_stats.systemMessages.fetchAndAddOrdered(1);
        break;
    default:
        break;
    }
}

void MessageEngine::updatePerformanceStats(int processingTime)
{
    // 更新最大处理时间
    int currentMax = m_stats.maxProcessingTime.loadAcquire();
    while (processingTime > currentMax) {
        if (m_stats.maxProcessingTime.testAndSetOrdered(currentMax, processingTime)) {
            break;
        }
        currentMax = m_stats.maxProcessingTime.loadAcquire();
    }
    
    // 更新平均处理时间（简单移动平均）
    int currentAvg = m_stats.averageProcessingTime.loadAcquire();
    int newAvg = (currentAvg + processingTime) / 2;
    m_stats.averageProcessingTime.storeRelease(newAvg);
}

void MessageEngine::handleProcessingError(const Message& message, const QString& error)
{
    qCCritical(messageEngine) << "Error processing message" << message.id << ":" << error;
    
    m_stats.failedMessages.fetchAndAddOrdered(1);
    emit messageFailed(message.id, error);
    logMessageEvent("MESSAGE_ERROR", message);
}

void MessageEngine::logMessageEvent(const QString& event, const Message& message) const
{
    qCDebug(messageEngine) << event << "ID:" << message.id 
                          << "Type:" << static_cast<int>(message.type)
                          << "From:" << message.fromUserId 
                          << "To:" << message.toUserId;
}

bool MessageEngine::persistMessage(const Message& message)
{
    // 这里可以实现消息持久化逻辑
    // 例如保存到数据库或文件
    Q_UNUSED(message)
    return true;
}

bool MessageEngine::loadPersistedMessages()
{
    // 这里可以实现从持久化存储加载消息的逻辑
    return true;
}

QString MessageEngine::generateMessageId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

LockFreeQueue<Message>* MessageEngine::getQueueByPriority(MessagePriority priority)
{
    switch (priority) {
    case MessagePriority::Critical:
        return &m_criticalQueue;
    case MessagePriority::High:
        return &m_highQueue;
    case MessagePriority::Normal:
        return &m_normalQueue;
    case MessagePriority::Low:
        return &m_lowQueue;
    default:
        return &m_normalQueue;
    }
}
