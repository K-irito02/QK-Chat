#ifndef MESSAGEENGINE_H
#define MESSAGEENGINE_H

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QTimer>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <functional>
#include <memory>
#include "../utils/LockFreeStructures.h"
#include "../core/ConnectionManager.h"
#include "../core/ThreadManager.h"

Q_DECLARE_LOGGING_CATEGORY(messageEngine)

/**
 * @brief 消息类型枚举
 */
enum class MessageType {
    Unknown = 0,
    Login = 1,
    Logout = 2,
    Chat = 3,
    GroupChat = 4,
    Heartbeat = 5,
    Register = 6,
    UserStatus = 7,
    FileTransfer = 8,
    SystemNotification = 9,
    EmailVerification = 10,
    EmailValidation = 11,
    UsernameValidation = 12,
    EmailAvailability = 13,
};

/**
 * @brief 消息优先级
 */
enum class MessagePriority {
    Low = 0,
    Normal = 1,
    High = 2,
    Critical = 3
};

/**
 * @brief 消息结构
 */
struct Message {
    QString id{};
    MessageType type{MessageType::Unknown};
    MessagePriority priority{MessagePriority::Normal};
    qint64 fromUserId{0};
    qint64 toUserId{0};
    QJsonObject data{};
    QDateTime timestamp{};
    QDateTime expiresAt{};
    QSslSocket* sourceSocket{nullptr};
    int retryCount{0};
    bool requiresResponse{false};
    
    Message() 
        : type(MessageType::Unknown)
        , priority(MessagePriority::Normal)
        , fromUserId(0)
        , toUserId(0)
        , timestamp(QDateTime::currentDateTime())
        , sourceSocket(nullptr)
        , retryCount(0)
        , requiresResponse(false)
    {}
    
    bool isExpired() const {
        return expiresAt.isValid() && QDateTime::currentDateTime() > expiresAt;
    }
    
    bool isValid() const {
        return type != MessageType::Unknown && !id.isEmpty();
    }
    
    QByteArray serialize() const {
        QJsonObject json;
        json["id"] = id;
        json["type"] = static_cast<int>(type);
        json["priority"] = static_cast<int>(priority);
        json["fromUserId"] = fromUserId;
        json["toUserId"] = toUserId;
        json["data"] = data;
        json["timestamp"] = timestamp.toString(Qt::ISODate);
        json["requiresResponse"] = requiresResponse;
        
        return QJsonDocument(json).toJson(QJsonDocument::Compact);
    }
    
    static Message deserialize(const QByteArray& data) {
        Message msg;
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            return msg; // 返回无效消息
        }
        
        QJsonObject json = doc.object();
        msg.id = json["id"].toString();
        msg.type = static_cast<MessageType>(json["type"].toInt());
        msg.priority = static_cast<MessagePriority>(json["priority"].toInt());
        msg.fromUserId = json["fromUserId"].toVariant().toLongLong();
        msg.toUserId = json["toUserId"].toVariant().toLongLong();
        msg.data = json["data"].toObject();
        msg.timestamp = QDateTime::fromString(json["timestamp"].toString(), Qt::ISODate);
        msg.requiresResponse = json["requiresResponse"].toBool();
        
        return msg;
    }
};

/**
 * @brief 消息处理器接口
 */
class MessageHandler
{
public:
    virtual ~MessageHandler() = default;
    virtual bool canHandle(MessageType type) const = 0;
    virtual bool handleMessage(const Message& message) = 0;
    virtual QString handlerName() const = 0;
};

/**
 * @brief 高性能消息处理引擎
 * 
 * 特性：
 * - 优先级队列处理
 * - 批量消息处理
 * - 消息路由和分发
 * - 自动重试机制
 * - 消息持久化
 */
class MessageEngine : public QObject
{
    Q_OBJECT

public:
    struct EngineStats {
        QAtomicInt totalMessages{0};
        QAtomicInt processedMessages{0};
        QAtomicInt failedMessages{0};
        QAtomicInt queuedMessages{0};
        QAtomicInt expiredMessages{0};
        QAtomicInt retriedMessages{0};
        
        // 按类型统计
        QAtomicInt loginMessages{0};
        QAtomicInt chatMessages{0};
        QAtomicInt groupChatMessages{0};
        QAtomicInt heartbeatMessages{0};
        QAtomicInt systemMessages{0};
        
        // 性能指标
        QAtomicInt averageProcessingTime{0};
        QAtomicInt maxProcessingTime{0};
        QAtomicInt throughputPerSecond{0};
    };

    explicit MessageEngine(ConnectionManager* connectionManager, QObject *parent = nullptr);
    ~MessageEngine();

    // 初始化和配置
    bool initialize();
    void shutdown();
    
    // 消息提交
    bool submitMessage(const Message& message);
    bool submitMessage(MessageType type, qint64 fromUserId, qint64 toUserId, 
                      const QJsonObject& data, MessagePriority priority = MessagePriority::Normal);
    
    // 批量消息处理
    bool submitMessages(const QList<Message>& messages);
    
    // 消息处理器管理
    void registerHandler(std::shared_ptr<MessageHandler> handler);
    void unregisterHandler(const QString& handlerName);
    QStringList getRegisteredHandlers() const;
    
    // 配置参数
    void setMaxQueueSize(int maxSize);
    void setBatchSize(int batchSize);
    void setProcessingInterval(int milliseconds);
    void setMaxRetryCount(int maxRetries);
    void setMessageTTL(int seconds);
    
    // 统计信息
    EngineStats getStats() const;
    void resetStats();
    int getQueueSize() const;
    bool isOverloaded() const;
    
    // 性能调优
    void enableBatchProcessing(bool enabled);
    void enablePriorityProcessing(bool enabled);
    void enableRetryMechanism(bool enabled);
    void enableMessagePersistence(bool enabled);

signals:
    void messageProcessed(const QString& messageId, MessageType type);
    void messageFailed(const QString& messageId, const QString& error);
    void queueOverflow();
    void engineOverloaded();
    void performanceAlert(const QString& message);

private slots:
    void processMessageBatch();
    void cleanupExpiredMessages();
    void updatePerformanceMetrics();

private:
    // 优先级队列 - 使用多个队列实现优先级
    LockFreeQueue<Message> m_criticalQueue;
    LockFreeQueue<Message> m_highQueue;
    LockFreeQueue<Message> m_normalQueue;
    LockFreeQueue<Message> m_lowQueue;
    
    // 重试队列
    LockFreeQueue<Message> m_retryQueue;
    
    // 组件引用
    ConnectionManager* m_connectionManager;
    ThreadManager* m_threadManager;
    
    // 消息处理器
    ConcurrentMap<QString, std::shared_ptr<MessageHandler>> m_handlers;
    ConcurrentMap<MessageType, QStringList> m_typeHandlers;
    
    // 配置参数
    QAtomicInt m_maxQueueSize{10000};
    QAtomicInt m_batchSize{100};
    QAtomicInt m_processingInterval{10};
    QAtomicInt m_maxRetryCount{3};
    QAtomicInt m_messageTTL{300}; // 5分钟
    
    // 处理选项
    QAtomicInt m_batchProcessingEnabled{1};
    QAtomicInt m_priorityProcessingEnabled{1};
    QAtomicInt m_retryMechanismEnabled{1};
    QAtomicInt m_messagePersistenceEnabled{0};
    
    // 统计信息
    EngineStats m_stats;
    
    // 定时器
    QTimer* m_processingTimer;
    QTimer* m_cleanupTimer;
    QTimer* m_metricsTimer;
    
    // 性能监控
    QDateTime m_lastMetricsUpdate;
    QAtomicInt m_messagesProcessedLastSecond{0};
    
    // 内部处理方法
    bool processMessage(const Message& message);
    QList<Message> dequeueMessages(int maxCount);
    void processBatch(const QList<Message>& messages);
    
    // 队列管理
    void enqueueMessage(const Message& message);
    Message dequeueNextMessage();
    int getTotalQueueSize() const;
    
    // 消息路由
    bool routeMessage(const Message& message);
    QStringList findHandlers(MessageType type) const;
    
    // 重试机制
    void scheduleRetry(const Message& message);
    void processRetryQueue();
    
    // 消息验证
    bool validateMessage(const Message& message) const;
    MessagePriority getDefaultPriority(MessageType type) const;
    
    // 统计更新
    void updateMessageStats(MessageType type, bool processed = true);
    void updatePerformanceStats(int processingTime);
    
    // 错误处理
    void handleProcessingError(const Message& message, const QString& error);
    void logMessageEvent(const QString& event, const Message& message) const;

    // 消息持久化
    bool persistMessage(const Message& message);
    bool loadPersistedMessages();

    // 工具方法
    QString generateMessageId() const;
    LockFreeQueue<Message>* getQueueByPriority(MessagePriority priority);
};

#endif // MESSAGEENGINE_H
