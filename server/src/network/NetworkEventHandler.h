#ifndef NETWORKEVENTHANDLER_H
#define NETWORKEVENTHANDLER_H

#include <QObject>
#include <QSslSocket>
#include <QTimer>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <memory>
#include "../utils/LockFreeStructures.h"
#include "../core/ConnectionManager.h"
#include "../core/ThreadManager.h"

Q_DECLARE_LOGGING_CATEGORY(networkEventHandler)

/**
 * @brief 网络事件类型
 */
enum class NetworkEventType {
    NewConnection = 0,
    DataReceived = 1,
    ConnectionClosed = 2,
    SslError = 3,
    SocketError = 4,
    Heartbeat = 5
};

/**
 * @brief 网络事件结构
 */
struct NetworkEvent {
    NetworkEventType type;
    QSslSocket* socket;
    QByteArray data;
    QString errorMessage;
    QDateTime timestamp;
    
    NetworkEvent() : type(NetworkEventType::NewConnection), socket(nullptr), timestamp(QDateTime::currentDateTime()) {}
    
    explicit NetworkEvent(NetworkEventType t, QSslSocket* s = nullptr)
        : type(t), socket(s), timestamp(QDateTime::currentDateTime()) {}
        
    NetworkEvent(NetworkEventType t, QSslSocket* s, const QByteArray& d)
        : type(t), socket(s), data(d), timestamp(QDateTime::currentDateTime()) {}
        
    NetworkEvent(NetworkEventType t, QSslSocket* s, const QString& error)
        : type(t), socket(s), errorMessage(error), timestamp(QDateTime::currentDateTime()) {}
};

/**
 * @brief 高性能网络事件处理器
 * 
 * 特性：
 * - 异步事件处理
 * - 无锁事件队列
 * - 批量处理优化
 * - 自动负载均衡
 */
class NetworkEventHandler : public QObject
{
    Q_OBJECT

public:
    struct EventStats {
        QAtomicInt totalEvents{0};
        QAtomicInt processedEvents{0};
        QAtomicInt droppedEvents{0};
        QAtomicInt queuedEvents{0};
        QAtomicInt processingErrors{0};
        
        // 按类型统计
        QAtomicInt newConnections{0};
        QAtomicInt dataReceived{0};
        QAtomicInt connectionsClosed{0};
        QAtomicInt sslErrors{0};
        QAtomicInt socketErrors{0};
        QAtomicInt heartbeats{0};
    };

    explicit NetworkEventHandler(ConnectionManager* connectionManager, QObject *parent = nullptr);
    ~NetworkEventHandler();

    // 初始化和配置
    bool initialize();
    void shutdown();
    
    // 事件提交接口
    void submitEvent(const NetworkEvent& event);
    void submitNewConnection(QSslSocket* socket);
    void submitDataReceived(QSslSocket* socket, const QByteArray& data);
    void submitConnectionClosed(QSslSocket* socket);
    void submitSslError(QSslSocket* socket, const QString& error);
    void submitSocketError(QSslSocket* socket, const QString& error);
    void submitHeartbeat(QSslSocket* socket);
    
    // 批量事件处理
    void submitEvents(const QList<NetworkEvent>& events);
    
    // 配置参数
    void setMaxQueueSize(int maxSize);
    void setBatchSize(int batchSize);
    void setProcessingInterval(int milliseconds);
    void setMaxProcessingTime(int milliseconds);
    
    // 统计信息
    EventStats getStats() const;
    void resetStats();
    int getQueueSize() const;
    bool isOverloaded() const;
    
    // 性能调优
    void enableBatchProcessing(bool enabled);
    void setLoadBalancing(bool enabled);
    void setPriorityProcessing(bool enabled);

signals:
    void eventProcessed(NetworkEventType type);
    void eventDropped(NetworkEventType type);
    void queueOverflow();
    void processingError(const QString& error);
    void performanceAlert(const QString& message);

private slots:
    void processEventBatch();
    void checkPerformance();

private:
    // 事件队列
    LockFreeQueue<NetworkEvent> m_eventQueue;
    
    // 组件引用
    ConnectionManager* m_connectionManager;
    ThreadManager* m_threadManager;
    
    // 配置参数
    QAtomicInt m_maxQueueSize{10000};
    QAtomicInt m_batchSize{50};
    QAtomicInt m_processingInterval{10};
    QAtomicInt m_maxProcessingTime{100};
    
    // 处理选项
    QAtomicInt m_batchProcessingEnabled{1};
    QAtomicInt m_loadBalancingEnabled{1};
    QAtomicInt m_priorityProcessingEnabled{1};
    
    // 统计信息
    EventStats m_stats;
    
    // 定时器
    QTimer* m_processingTimer;
    QTimer* m_performanceTimer;
    
    // 性能监控
    QDateTime m_lastProcessingTime;
    QAtomicInt m_processingLatency{0};
    
    // 内部处理方法
    void processEvent(const NetworkEvent& event);
    void processNewConnection(QSslSocket* socket);
    void processDataReceived(QSslSocket* socket, const QByteArray& data);
    void processConnectionClosed(QSslSocket* socket);
    void processSslError(QSslSocket* socket, const QString& error);
    void processSocketError(QSslSocket* socket, const QString& error);
    void processHeartbeat(QSslSocket* socket);
    
    // 批量处理
    QList<NetworkEvent> dequeueEvents(int maxCount);
    void processBatch(const QList<NetworkEvent>& events);
    
    // 优先级处理
    int getEventPriority(NetworkEventType type) const;
    void sortEventsByPriority(QList<NetworkEvent>& events) const;
    
    // 负载均衡
    void distributeLoad(const QList<NetworkEvent>& events);
    
    // 性能监控
    void updatePerformanceMetrics();
    void checkQueueHealth();
    void logPerformanceWarning(const QString& message);
    
    // 统计更新
    void updateEventStats(NetworkEventType type, bool processed = true);
    void incrementCounter(QAtomicInt& counter);

    // 错误处理
    void handleProcessingError(const NetworkEvent& event, const QString& error);
    bool isEventValid(const NetworkEvent& event) const;
};

#endif // NETWORKEVENTHANDLER_H
