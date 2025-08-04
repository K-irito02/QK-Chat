#ifndef STACK_TRACE_COLLECTOR_H
#define STACK_TRACE_COLLECTOR_H

#include <QObject>
#include <QMutex>
#include <QHash>
#include <QQueue>
#include <QTimer>
#include <QDateTime>
#include <QLoggingCategory>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QElapsedTimer>
#include <memory>
#include <functional>
#include <execinfo.h>
#include <cxxabi.h>
#include <signal.h>
#include <csignal>

Q_DECLARE_LOGGING_CATEGORY(stackTrace)

/**
 * @brief 堆栈帧信息
 */
struct StackFrame {
    QString function;       // 函数名
    QString file;          // 文件名
    int line{0};           // 行号
    QString address;       // 内存地址
    QString module;        // 模块名
    QString demangledName; // 解码后的函数名
};

/**
 * @brief 堆栈跟踪信息
 */
struct StackTrace {
    QString traceId;                    // 跟踪ID
    QDateTime timestamp;                // 时间戳
    QThread* thread;                    // 线程
    QString component;                  // 组件名
    QString operation;                  // 操作名
    QString errorMessage;               // 错误消息
    QList<StackFrame> frames;          // 堆栈帧
    QJsonObject context;               // 上下文信息
    int severity{0};                   // 严重程度
};

/**
 * @brief 异常类型枚举
 */
enum class ExceptionType {
    ThreadPoolException,     // 线程池异常
    DatabaseException,       // 数据库异常
    NetworkException,        // 网络异常
    SSLException,           // SSL异常
    MessageException,       // 消息异常
    MemoryException,        // 内存异常
    TimeoutException,       // 超时异常
    UnknownException        // 未知异常
};

/**
 * @brief 异常信息
 */
struct ExceptionInfo {
    ExceptionType type;
    QString component;
    QString operation;
    QString message;
    QDateTime timestamp;
    QThread* thread;
    StackTrace stackTrace;
    QJsonObject metadata;
    int occurrenceCount{1};
};

/**
 * @brief 堆栈追踪收集器
 */
class StackTraceCollector : public QObject
{
    Q_OBJECT

public:
    static StackTraceCollector* instance();
    
    // 堆栈收集
    StackTrace captureStackTrace(const QString& component, const QString& operation,
                                const QString& errorMessage = QString(),
                                const QJsonObject& context = QJsonObject());
    
    // 异常记录
    void recordException(ExceptionType type, const QString& component, 
                        const QString& operation, const QString& message,
                        const QJsonObject& metadata = QJsonObject());
    
    void recordThreadPoolException(const QString& poolName, const QString& operation,
                                  const QString& error, const QJsonObject& context = QJsonObject());
    
    void recordDatabaseException(const QString& connectionName, const QString& sql,
                                const QString& error, int executionTime = -1);
    
    void recordNetworkException(const QString& endpoint, const QString& operation,
                               const QString& error, const QJsonObject& sslInfo = QJsonObject());
    
    // 查询接口
    QList<ExceptionInfo> getExceptions(ExceptionType type = ExceptionType::UnknownException,
                                      const QDateTime& since = QDateTime()) const;
    
    QList<StackTrace> getStackTraces(const QString& component = QString(),
                                    const QDateTime& since = QDateTime()) const;
    
    // 统计信息
    QJsonObject getExceptionStatistics() const;
    QJsonObject getComponentStatistics() const;
    QJsonObject getThreadStatistics() const;
    
    // 配置
    void setMaxTraces(int maxTraces) { m_maxTraces = maxTraces; }
    void setMaxExceptions(int maxExceptions) { m_maxExceptions = maxExceptions; }
    void enableSymbolResolution(bool enabled) { m_symbolResolution = enabled; }
    void setTraceDepth(int depth) { m_traceDepth = depth; }

signals:
    void exceptionRecorded(const ExceptionInfo& exception);
    void criticalExceptionDetected(const ExceptionInfo& exception);
    void repeatedExceptionDetected(const QString& component, int count);

private slots:
    void performCleanup();
    void analyzeExceptionPatterns();

private:
    explicit StackTraceCollector(QObject *parent = nullptr);
    
    static StackTraceCollector* s_instance;
    static QMutex s_instanceMutex;
    
    // 存储
    QQueue<StackTrace> m_stackTraces;
    QQueue<ExceptionInfo> m_exceptions;
    QHash<QString, ExceptionInfo> m_exceptionIndex; // 用于去重和计数
    mutable QMutex m_dataMutex;
    
    // 配置
    int m_maxTraces{1000};
    int m_maxExceptions{500};
    bool m_symbolResolution{true};
    int m_traceDepth{20};
    
    // 定时器
    QTimer* m_cleanupTimer;
    QTimer* m_analysisTimer;
    
    // 内部方法
    QList<StackFrame> captureStackFrames() const;
    StackFrame parseStackFrame(void* address) const;
    QString demangleSymbol(const QString& mangledName) const;
    QString generateTraceId() const;
    QString getComponentFromStackTrace(const QList<StackFrame>& frames) const;
    void cleanupOldData();
    void updateExceptionIndex(const ExceptionInfo& exception);
};

/**
 * @brief 线程池异常追踪器
 */
class ThreadPoolTracker : public QObject
{
    Q_OBJECT

public:
    struct TaskInfo {
        QString taskId;
        QString poolName;
        QDateTime startTime;
        QDateTime endTime;
        QThread* thread;
        bool completed{false};
        bool failed{false};
        QString errorMessage;
        StackTrace executionTrace;
    };

    explicit ThreadPoolTracker(QObject *parent = nullptr);
    
    // 任务跟踪
    QString startTaskTracking(const QString& poolName, const QString& taskDescription);
    void endTaskTracking(const QString& taskId, bool success = true, const QString& error = QString());
    void recordTaskException(const QString& taskId, const QString& error);
    
    // 线程状态跟踪
    void recordThreadStart(QThread* thread, const QString& poolName);
    void recordThreadEnd(QThread* thread);
    void recordThreadException(QThread* thread, const QString& error);
    
    // 查询接口
    QList<TaskInfo> getFailedTasks(const QString& poolName = QString(),
                                  const QDateTime& since = QDateTime()) const;
    QJsonObject getPoolStatistics(const QString& poolName) const;
    QJsonObject getAllPoolStatistics() const;

signals:
    void taskFailed(const TaskInfo& task);
    void threadCrashed(QThread* thread, const QString& error);
    void poolOverloaded(const QString& poolName);

private:
    QHash<QString, TaskInfo> m_activeTasks;
    QQueue<TaskInfo> m_taskHistory;
    QHash<QThread*, QString> m_threadPools;
    mutable QMutex m_dataMutex;
    
    StackTraceCollector* m_stackCollector;
    int m_maxTaskHistory{1000};
    
    void cleanupTaskHistory();
};

/**
 * @brief 数据库操作追踪器
 */
class DatabaseTracker : public QObject
{
    Q_OBJECT

public:
    struct QueryInfo {
        QString queryId;
        QString connectionName;
        QString sql;
        QVariantList parameters;
        QDateTime startTime;
        QDateTime endTime;
        int executionTime{0};
        bool success{true};
        QString errorMessage;
        int affectedRows{0};
        StackTrace callTrace;
    };

    struct ConnectionInfo {
        QString connectionName;
        QDateTime createdTime;
        QDateTime lastUsed;
        int queryCount{0};
        int errorCount{0};
        bool isValid{true};
        QString lastError;
        StackTrace creationTrace;
    };

    explicit DatabaseTracker(QObject *parent = nullptr);
    
    // 查询跟踪
    QString startQueryTracking(const QString& connectionName, const QString& sql,
                              const QVariantList& parameters = QVariantList());
    void endQueryTracking(const QString& queryId, bool success, int executionTime,
                         int affectedRows = 0, const QString& error = QString());
    
    // 连接跟踪
    void recordConnectionCreated(const QString& connectionName);
    void recordConnectionDestroyed(const QString& connectionName);
    void recordConnectionError(const QString& connectionName, const QString& error);
    
    // 事务跟踪
    void recordTransactionStart(const QString& connectionName);
    void recordTransactionCommit(const QString& connectionName);
    void recordTransactionRollback(const QString& connectionName, const QString& reason);
    
    // 查询接口
    QList<QueryInfo> getFailedQueries(const QString& connectionName = QString(),
                                     const QDateTime& since = QDateTime()) const;
    QList<QueryInfo> getSlowQueries(int thresholdMs = 1000,
                                   const QDateTime& since = QDateTime()) const;
    QJsonObject getConnectionStatistics() const;
    QJsonObject getQueryStatistics() const;

signals:
    void queryFailed(const QueryInfo& query);
    void slowQueryDetected(const QueryInfo& query);
    void connectionLost(const QString& connectionName);
    void transactionRollback(const QString& connectionName, const QString& reason);

private:
    QHash<QString, QueryInfo> m_activeQueries;
    QQueue<QueryInfo> m_queryHistory;
    QHash<QString, ConnectionInfo> m_connections;
    mutable QMutex m_dataMutex;
    
    StackTraceCollector* m_stackCollector;
    int m_maxQueryHistory{2000};
    int m_slowQueryThreshold{1000}; // 毫秒
    
    void cleanupQueryHistory();
    void analyzeQueryPatterns();
};

/**
 * @brief 网络事件追踪器
 */
class NetworkTracker : public QObject
{
    Q_OBJECT

public:
    struct ConnectionEvent {
        QString eventId;
        QString endpoint;
        QString eventType; // connect, disconnect, ssl_handshake, data_transfer
        QDateTime timestamp;
        bool success{true};
        QString errorMessage;
        QJsonObject metadata;
        StackTrace eventTrace;
    };

    struct SSLInfo {
        QString protocol;
        QString cipher;
        QString certificate;
        QDateTime handshakeTime;
        int handshakeDuration{0};
        bool sessionReused{false};
    };

    explicit NetworkTracker(QObject *parent = nullptr);
    
    // 连接事件跟踪
    void recordConnectionEvent(const QString& endpoint, const QString& eventType,
                              bool success, const QString& error = QString(),
                              const QJsonObject& metadata = QJsonObject());
    
    // SSL事件跟踪
    void recordSSLHandshakeStart(const QString& endpoint);
    void recordSSLHandshakeEnd(const QString& endpoint, bool success,
                              const SSLInfo& sslInfo = SSLInfo(),
                              const QString& error = QString());
    
    // 数据传输跟踪
    void recordDataTransfer(const QString& endpoint, qint64 bytesTransferred,
                           const QString& direction = "send"); // send/receive
    
    // 查询接口
    QList<ConnectionEvent> getConnectionEvents(const QString& endpoint = QString(),
                                              const QDateTime& since = QDateTime()) const;
    QList<ConnectionEvent> getFailedEvents(const QString& eventType = QString(),
                                          const QDateTime& since = QDateTime()) const;
    QJsonObject getNetworkStatistics() const;
    QJsonObject getSSLStatistics() const;

signals:
    void connectionFailed(const ConnectionEvent& event);
    void sslHandshakeFailed(const ConnectionEvent& event);
    void networkTimeout(const QString& endpoint);
    void highLatencyDetected(const QString& endpoint, int latency);

private:
    QQueue<ConnectionEvent> m_connectionEvents;
    QHash<QString, SSLInfo> m_sslSessions;
    QHash<QString, QElapsedTimer> m_activeHandshakes;
    mutable QMutex m_dataMutex;
    
    StackTraceCollector* m_stackCollector;
    int m_maxEvents{1500};
    
    void cleanupEvents();
    void analyzeNetworkPatterns();
};

/**
 * @brief 信号处理器 - 处理系统信号并收集堆栈信息
 */
class SignalHandler : public QObject
{
    Q_OBJECT

public:
    static SignalHandler* instance();
    
    void installSignalHandlers();
    void uninstallSignalHandlers();
    
    // 手动触发堆栈收集
    void dumpStackTrace(const QString& reason = "Manual dump");

signals:
    void segmentationFault(const StackTrace& trace);
    void abortSignal(const StackTrace& trace);
    void floatingPointException(const StackTrace& trace);

private:
    explicit SignalHandler(QObject *parent = nullptr);
    
    static SignalHandler* s_instance;
    static void signalHandler(int signal);
    static void handleCrash(int signal);
    
    StackTraceCollector* m_stackCollector;
    bool m_handlersInstalled{false};
};

/**
 * @brief 异常模式分析器
 */
class ExceptionPatternAnalyzer : public QObject
{
    Q_OBJECT

public:
    struct ExceptionPattern {
        QString patternId;
        ExceptionType type;
        QString component;
        int occurrenceCount{0};
        QDateTime firstOccurrence;
        QDateTime lastOccurrence;
        double frequency{0.0}; // 每小时发生次数
        QStringList commonStackFrames;
        QString suggestedAction;
    };

    explicit ExceptionPatternAnalyzer(QObject *parent = nullptr);
    
    void analyzeExceptions(const QList<ExceptionInfo>& exceptions);
    QList<ExceptionPattern> getPatterns() const;
    QList<ExceptionPattern> getCriticalPatterns() const;
    
    void setFrequencyThreshold(double threshold) { m_frequencyThreshold = threshold; }

signals:
    void criticalPatternDetected(const ExceptionPattern& pattern);
    void newPatternDetected(const ExceptionPattern& pattern);

private:
    QList<ExceptionPattern> m_patterns;
    double m_frequencyThreshold{5.0}; // 每小时5次
    mutable QMutex m_dataMutex;
    
    QString calculatePatternId(const ExceptionInfo& exception) const;
    QStringList extractCommonFrames(const QList<StackTrace>& traces) const;
    QString suggestAction(const ExceptionPattern& pattern) const;
};

#endif // STACK_TRACE_COLLECTOR_H