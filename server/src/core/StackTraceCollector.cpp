#include "StackTraceCollector.h"
#include <QDebug>
#include <QCoreApplication>
#include <QUuid>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <dlfcn.h>

Q_LOGGING_CATEGORY(stackTrace, "qkchat.server.stacktrace")

// ============================================================================
// StackTraceCollector 实现
// ============================================================================

StackTraceCollector* StackTraceCollector::s_instance = nullptr;
QMutex StackTraceCollector::s_instanceMutex;

StackTraceCollector* StackTraceCollector::instance()
{
    if (!s_instance) {
        QMutexLocker locker(&s_instanceMutex);
        if (!s_instance) {
            s_instance = new StackTraceCollector();
        }
    }
    return s_instance;
}

StackTraceCollector::StackTraceCollector(QObject *parent)
    : QObject(parent)
    , m_cleanupTimer(new QTimer(this))
    , m_analysisTimer(new QTimer(this))
{
    connect(m_cleanupTimer, &QTimer::timeout, this, &StackTraceCollector::performCleanup);
    connect(m_analysisTimer, &QTimer::timeout, this, &StackTraceCollector::analyzeExceptionPatterns);
    
    m_cleanupTimer->start(300000);  // 每5分钟清理一次
    m_analysisTimer->start(600000); // 每10分钟分析一次
    
    qCInfo(stackTrace) << "StackTraceCollector initialized";
}

StackTrace StackTraceCollector::captureStackTrace(const QString& component, const QString& operation,
                                                 const QString& errorMessage, const QJsonObject& context)
{
    StackTrace trace;
    trace.traceId = generateTraceId();
    trace.timestamp = QDateTime::currentDateTime();
    trace.thread = QThread::currentThread();
    trace.component = component;
    trace.operation = operation;
    trace.errorMessage = errorMessage;
    trace.context = context;
    trace.frames = captureStackFrames();
    trace.severity = errorMessage.isEmpty() ? 1 : 3; // 有错误信息的认为是较严重的
    
    // 如果没有指定组件，从堆栈中推断
    if (trace.component.isEmpty()) {
        trace.component = getComponentFromStackTrace(trace.frames);
    }
    
    QMutexLocker locker(&m_dataMutex);
    m_stackTraces.enqueue(trace);
    
    // 限制堆栈跟踪数量
    while (m_stackTraces.size() > m_maxTraces) {
        m_stackTraces.dequeue();
    }
    
    qCDebug(stackTrace) << "Stack trace captured:" << trace.traceId 
                        << "component:" << trace.component
                        << "frames:" << trace.frames.size();
    
    return trace;
}

void StackTraceCollector::recordException(ExceptionType type, const QString& component, 
                                        const QString& operation, const QString& message,
                                        const QJsonObject& metadata)
{
    ExceptionInfo exception;
    exception.type = type;
    exception.component = component;
    exception.operation = operation;
    exception.message = message;
    exception.timestamp = QDateTime::currentDateTime();
    exception.thread = QThread::currentThread();
    exception.metadata = metadata;
    exception.stackTrace = captureStackTrace(component, operation, message, metadata);
    
    updateExceptionIndex(exception);
    
    QMutexLocker locker(&m_dataMutex);
    m_exceptions.enqueue(exception);
    
    // 限制异常数量
    while (m_exceptions.size() > m_maxExceptions) {
        m_exceptions.dequeue();
    }
    
    emit exceptionRecorded(exception);
    
    // 检查是否是关键异常
    if (exception.occurrenceCount >= 5 || type == ExceptionType::ThreadPoolException) {
        emit criticalExceptionDetected(exception);
    }
    
    qCWarning(stackTrace) << "Exception recorded:" << static_cast<int>(type) 
                          << component << operation << message;
}

void StackTraceCollector::recordThreadPoolException(const QString& poolName, const QString& operation,
                                                   const QString& error, const QJsonObject& context)
{
    QJsonObject metadata = context;
    metadata["poolName"] = poolName;
    metadata["threadId"] = QString::number(reinterpret_cast<quintptr>(QThread::currentThread()));
    
    recordException(ExceptionType::ThreadPoolException, "ThreadPool", operation, error, metadata);
}

void StackTraceCollector::recordDatabaseException(const QString& connectionName, const QString& sql,
                                                 const QString& error, int executionTime)
{
    QJsonObject metadata;
    metadata["connectionName"] = connectionName;
    metadata["sql"] = sql;
    if (executionTime >= 0) {
        metadata["executionTime"] = executionTime;
    }
    metadata["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    recordException(ExceptionType::DatabaseException, "Database", "Query", error, metadata);
}

void StackTraceCollector::recordNetworkException(const QString& endpoint, const QString& operation,
                                                const QString& error, const QJsonObject& sslInfo)
{
    QJsonObject metadata = sslInfo;
    metadata["endpoint"] = endpoint;
    metadata["localPort"] = QCoreApplication::applicationPid(); // 简化处理
    
    ExceptionType type = sslInfo.isEmpty() ? ExceptionType::NetworkException : ExceptionType::SSLException;
    recordException(type, "Network", operation, error, metadata);
}

QList<ExceptionInfo> StackTraceCollector::getExceptions(ExceptionType type, const QDateTime& since) const
{
    QMutexLocker locker(&m_dataMutex);
    
    QList<ExceptionInfo> result;
    for (const ExceptionInfo& exception : m_exceptions) {
        if ((type == ExceptionType::UnknownException || exception.type == type) &&
            (since.isNull() || exception.timestamp >= since)) {
            result.append(exception);
        }
    }
    
    return result;
}

QList<StackTrace> StackTraceCollector::getStackTraces(const QString& component, const QDateTime& since) const
{
    QMutexLocker locker(&m_dataMutex);
    
    QList<StackTrace> result;
    for (const StackTrace& trace : m_stackTraces) {
        if ((component.isEmpty() || trace.component == component) &&
            (since.isNull() || trace.timestamp >= since)) {
            result.append(trace);
        }
    }
    
    return result;
}

QJsonObject StackTraceCollector::getExceptionStatistics() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QJsonObject stats;
    QHash<ExceptionType, int> typeCounts;
    QHash<QString, int> componentCounts;
    
    for (const ExceptionInfo& exception : m_exceptions) {
        typeCounts[exception.type]++;
        componentCounts[exception.component]++;
    }
    
    QJsonObject typeStats;
    for (auto it = typeCounts.begin(); it != typeCounts.end(); ++it) {
        typeStats[QString::number(static_cast<int>(it.key()))] = it.value();
    }
    
    QJsonObject componentStats;
    for (auto it = componentCounts.begin(); it != componentCounts.end(); ++it) {
        componentStats[it.key()] = it.value();
    }
    
    stats["totalExceptions"] = m_exceptions.size();
    stats["totalStackTraces"] = m_stackTraces.size();
    stats["exceptionsByType"] = typeStats;
    stats["exceptionsByComponent"] = componentStats;
    stats["lastUpdate"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    return stats;
}

QJsonObject StackTraceCollector::getComponentStatistics() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QJsonObject stats;
    QHash<QString, QJsonObject> componentStats;
    
    for (const ExceptionInfo& exception : m_exceptions) {
        QJsonObject& compStat = componentStats[exception.component];
        compStat["count"] = compStat["count"].toInt() + 1;
        
        if (!compStat.contains("firstSeen") || 
            exception.timestamp < QDateTime::fromString(compStat["firstSeen"].toString(), Qt::ISODate)) {
            compStat["firstSeen"] = exception.timestamp.toString(Qt::ISODate);
        }
        
        if (!compStat.contains("lastSeen") || 
            exception.timestamp > QDateTime::fromString(compStat["lastSeen"].toString(), Qt::ISODate)) {
            compStat["lastSeen"] = exception.timestamp.toString(Qt::ISODate);
        }
    }
    
    for (auto it = componentStats.begin(); it != componentStats.end(); ++it) {
        stats[it.key()] = it.value();
    }
    
    return stats;
}

QJsonObject StackTraceCollector::getThreadStatistics() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QJsonObject stats;
    QHash<quintptr, QJsonObject> threadStats;
    
    for (const ExceptionInfo& exception : m_exceptions) {
        quintptr threadId = reinterpret_cast<quintptr>(exception.thread);
        QJsonObject& threadStat = threadStats[threadId];
        threadStat["count"] = threadStat["count"].toInt() + 1;
        threadStat["threadId"] = QString::number(threadId);
    }
    
    for (auto it = threadStats.begin(); it != threadStats.end(); ++it) {
        stats[QString::number(it.key())] = it.value();
    }
    
    return stats;
}

void StackTraceCollector::performCleanup()
{
    cleanupOldData();
}

void StackTraceCollector::analyzeExceptionPatterns()
{
    QMutexLocker locker(&m_dataMutex);
    
    // 简化的模式分析：检查重复异常
    QHash<QString, int> exceptionSignatures;
    
    for (const ExceptionInfo& exception : m_exceptions) {
        QString signature = QString("%1:%2:%3")
                              .arg(static_cast<int>(exception.type))
                              .arg(exception.component)
                              .arg(exception.operation);
        
        exceptionSignatures[signature]++;
        
        if (exceptionSignatures[signature] >= 10) {
            emit repeatedExceptionDetected(exception.component, exceptionSignatures[signature]);
        }
    }
    
    qCDebug(stackTrace) << "Exception pattern analysis completed, found" 
                        << exceptionSignatures.size() << "unique patterns";
}

QList<StackFrame> StackTraceCollector::captureStackFrames() const
{
    QList<StackFrame> frames;
    
#ifdef Q_OS_LINUX
    void* buffer[m_traceDepth];
    int nptrs = backtrace(buffer, m_traceDepth);
    
    if (nptrs > 0) {
        for (int i = 0; i < nptrs; ++i) {
            StackFrame frame = parseStackFrame(buffer[i]);
            if (!frame.function.isEmpty()) {
                frames.append(frame);
            }
        }
    }
#else
    // 其他平台的实现
    StackFrame frame;
    frame.function = "unavailable";
    frame.address = "0x0";
    frames.append(frame);
#endif
    
    return frames;
}

StackFrame StackTraceCollector::parseStackFrame(void* address) const
{
    StackFrame frame;
    frame.address = QString("0x%1").arg(reinterpret_cast<quintptr>(address), 0, 16);
    
#ifdef Q_OS_LINUX
    if (m_symbolResolution) {
        Dl_info dlInfo;
        if (dladdr(address, &dlInfo)) {
            if (dlInfo.dli_sname) {
                frame.function = QString::fromLocal8Bit(dlInfo.dli_sname);
                frame.demangledName = demangleSymbol(frame.function);
            }
            
            if (dlInfo.dli_fname) {
                frame.module = QString::fromLocal8Bit(dlInfo.dli_fname);
                frame.file = QFileInfo(frame.module).baseName();
            }
        }
    }
#endif
    
    return frame;
}

QString StackTraceCollector::demangleSymbol(const QString& mangledName) const
{
#ifdef Q_OS_LINUX
    if (mangledName.startsWith("_Z")) {
        int status = 0;
        char* demangled = abi::__cxa_demangle(mangledName.toLocal8Bit().constData(), 
                                            nullptr, nullptr, &status);
        if (status == 0 && demangled) {
            QString result = QString::fromLocal8Bit(demangled);
            free(demangled);
            return result;
        }
    }
#endif
    return mangledName;
}

QString StackTraceCollector::generateTraceId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString StackTraceCollector::getComponentFromStackTrace(const QList<StackFrame>& frames) const
{
    for (const StackFrame& frame : frames) {
        if (frame.function.contains("ChatServer")) return "ChatServer";
        if (frame.function.contains("Database")) return "Database";
        if (frame.function.contains("Network")) return "Network";
        if (frame.function.contains("ThreadPool")) return "ThreadPool";
        if (frame.function.contains("SSL")) return "SSL";
    }
    return "Unknown";
}

void StackTraceCollector::cleanupOldData()
{
    QMutexLocker locker(&m_dataMutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-1); // 保留1天的数据
    
    // 清理旧的堆栈跟踪
    while (!m_stackTraces.isEmpty() && m_stackTraces.head().timestamp < cutoff) {
        m_stackTraces.dequeue();
    }
    
    // 清理旧的异常信息
    while (!m_exceptions.isEmpty() && m_exceptions.head().timestamp < cutoff) {
        m_exceptions.dequeue();
    }
    
    // 清理异常索引中的旧条目
    auto it = m_exceptionIndex.begin();
    while (it != m_exceptionIndex.end()) {
        if (it.value().timestamp < cutoff) {
            it = m_exceptionIndex.erase(it);
        } else {
            ++it;
        }
    }
    
    qCDebug(stackTrace) << "Cleanup completed - stack traces:" << m_stackTraces.size()
                        << "exceptions:" << m_exceptions.size();
}

void StackTraceCollector::updateExceptionIndex(const ExceptionInfo& exception)
{
    QString key = QString("%1:%2:%3:%4")
                    .arg(static_cast<int>(exception.type))
                    .arg(exception.component)
                    .arg(exception.operation)
                    .arg(QCryptographicHash::hash(exception.message.toUtf8(), QCryptographicHash::Md5).toHex());
    
    if (m_exceptionIndex.contains(key)) {
        m_exceptionIndex[key].occurrenceCount++;
    } else {
        ExceptionInfo indexedInfo = exception;
        indexedInfo.occurrenceCount = 1;
        m_exceptionIndex[key] = indexedInfo;
    }
}

// ============================================================================
// ThreadPoolTracker 实现
// ============================================================================

ThreadPoolTracker::ThreadPoolTracker(QObject *parent)
    : QObject(parent)
    , m_stackCollector(StackTraceCollector::instance())
{
    qCInfo(stackTrace) << "ThreadPoolTracker initialized";
}

QString ThreadPoolTracker::startTaskTracking(const QString& poolName, const QString& taskDescription)
{
    QString taskId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    TaskInfo task;
    task.taskId = taskId;
    task.poolName = poolName;
    task.startTime = QDateTime::currentDateTime();
    task.thread = QThread::currentThread();
    task.executionTrace = m_stackCollector->captureStackTrace("ThreadPool", taskDescription);
    
    QMutexLocker locker(&m_dataMutex);
    m_activeTasks[taskId] = task;
    
    qCDebug(stackTrace) << "Task tracking started:" << taskId << "pool:" << poolName;
    return taskId;
}

void ThreadPoolTracker::endTaskTracking(const QString& taskId, bool success, const QString& error)
{
    QMutexLocker locker(&m_dataMutex);
    
    auto it = m_activeTasks.find(taskId);
    if (it != m_activeTasks.end()) {
        TaskInfo task = it.value();
        task.endTime = QDateTime::currentDateTime();
        task.completed = true;
        task.failed = !success;
        task.errorMessage = error;
        
        m_activeTasks.erase(it);
        m_taskHistory.enqueue(task);
        
        // 限制历史记录大小
        while (m_taskHistory.size() > m_maxTaskHistory) {
            m_taskHistory.dequeue();
        }
        
        if (!success) {
            emit taskFailed(task);
            
            // 记录异常
            m_stackCollector->recordThreadPoolException(task.poolName, "TaskExecution", error);
        }
        
        qCDebug(stackTrace) << "Task tracking ended:" << taskId << "success:" << success;
    }
}

void ThreadPoolTracker::recordTaskException(const QString& taskId, const QString& error)
{
    QMutexLocker locker(&m_dataMutex);
    
    auto it = m_activeTasks.find(taskId);
    if (it != m_activeTasks.end()) {
        it.value().errorMessage = error;
        it.value().failed = true;
    }
    
    // 记录异常到堆栈收集器
    m_stackCollector->recordThreadPoolException("Unknown", "TaskException", error);
}

void ThreadPoolTracker::recordThreadStart(QThread* thread, const QString& poolName)
{
    QMutexLocker locker(&m_dataMutex);
    m_threadPools[thread] = poolName;
    
    qCDebug(stackTrace) << "Thread started:" << thread << "pool:" << poolName;
}

void ThreadPoolTracker::recordThreadEnd(QThread* thread)
{
    QMutexLocker locker(&m_dataMutex);
    m_threadPools.remove(thread);
    
    qCDebug(stackTrace) << "Thread ended:" << thread;
}

void ThreadPoolTracker::recordThreadException(QThread* thread, const QString& error)
{
    QString poolName;
    {
        QMutexLocker locker(&m_dataMutex);
        poolName = m_threadPools.value(thread, "Unknown");
    }
    
    emit threadCrashed(thread, error);
    
    // 记录异常
    m_stackCollector->recordThreadPoolException(poolName, "ThreadCrash", error);
    
    qCCritical(stackTrace) << "Thread crashed:" << thread << "pool:" << poolName << "error:" << error;
}

QList<ThreadPoolTracker::TaskInfo> ThreadPoolTracker::getFailedTasks(const QString& poolName, const QDateTime& since) const
{
    QMutexLocker locker(&m_dataMutex);
    
    QList<TaskInfo> result;
    for (const TaskInfo& task : m_taskHistory) {
        if (task.failed && 
            (poolName.isEmpty() || task.poolName == poolName) &&
            (since.isNull() || task.startTime >= since)) {
            result.append(task);
        }
    }
    
    return result;
}

QJsonObject ThreadPoolTracker::getPoolStatistics(const QString& poolName) const
{
    QMutexLocker locker(&m_dataMutex);
    
    QJsonObject stats;
    int totalTasks = 0;
    int failedTasks = 0;
    int activeTasks = 0;
    
    // 统计历史任务
    for (const TaskInfo& task : m_taskHistory) {
        if (poolName.isEmpty() || task.poolName == poolName) {
            totalTasks++;
            if (task.failed) {
                failedTasks++;
            }
        }
    }
    
    // 统计活跃任务
    for (const TaskInfo& task : m_activeTasks) {
        if (poolName.isEmpty() || task.poolName == poolName) {
            activeTasks++;
        }
    }
    
    stats["totalTasks"] = totalTasks;
    stats["failedTasks"] = failedTasks;
    stats["activeTasks"] = activeTasks;
    stats["successRate"] = totalTasks > 0 ? (double)(totalTasks - failedTasks) / totalTasks : 1.0;
    
    return stats;
}

QJsonObject ThreadPoolTracker::getAllPoolStatistics() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QJsonObject allStats;
    QSet<QString> poolNames;
    
    // 收集所有池名称
    for (const TaskInfo& task : m_taskHistory) {
        poolNames.insert(task.poolName);
    }
    for (const TaskInfo& task : m_activeTasks) {
        poolNames.insert(task.poolName);
    }
    
    // 为每个池生成统计
    for (const QString& poolName : poolNames) {
        allStats[poolName] = getPoolStatistics(poolName);
    }
    
    return allStats;
}

void ThreadPoolTracker::cleanupTaskHistory()
{
    QMutexLocker locker(&m_dataMutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addHours(-24); // 保留24小时
    
    while (!m_taskHistory.isEmpty() && m_taskHistory.head().startTime < cutoff) {
        m_taskHistory.dequeue();
    }
}

// ============================================================================
// SignalHandler 实现
// ============================================================================

SignalHandler* SignalHandler::s_instance = nullptr;

SignalHandler* SignalHandler::instance()
{
    if (!s_instance) {
        s_instance = new SignalHandler();
    }
    return s_instance;
}

SignalHandler::SignalHandler(QObject *parent)
    : QObject(parent)
    , m_stackCollector(StackTraceCollector::instance())
{
    qCInfo(stackTrace) << "SignalHandler initialized";
}

void SignalHandler::installSignalHandlers()
{
    if (!m_handlersInstalled) {
        signal(SIGSEGV, signalHandler);
        signal(SIGABRT, signalHandler);
        signal(SIGFPE, signalHandler);
        signal(SIGILL, signalHandler);
        
        m_handlersInstalled = true;
        qCInfo(stackTrace) << "Signal handlers installed";
    }
}

void SignalHandler::uninstallSignalHandlers()
{
    if (m_handlersInstalled) {
        signal(SIGSEGV, SIG_DFL);
        signal(SIGABRT, SIG_DFL);
        signal(SIGFPE, SIG_DFL);
        signal(SIGILL, SIG_DFL);
        
        m_handlersInstalled = false;
        qCInfo(stackTrace) << "Signal handlers uninstalled";
    }
}

void SignalHandler::dumpStackTrace(const QString& reason)
{
    StackTrace trace = m_stackCollector->captureStackTrace("SignalHandler", "ManualDump", reason);
    qCWarning(stackTrace) << "Manual stack trace dump:" << reason << "traceId:" << trace.traceId;
}

void SignalHandler::signalHandler(int signal)
{
    handleCrash(signal);
}

void SignalHandler::handleCrash(int signal)
{
    SignalHandler* handler = instance();
    
    QString signalName;
    switch (signal) {
    case SIGSEGV: signalName = "SIGSEGV (Segmentation fault)"; break;
    case SIGABRT: signalName = "SIGABRT (Abort)"; break;
    case SIGFPE:  signalName = "SIGFPE (Floating point exception)"; break;
    case SIGILL:  signalName = "SIGILL (Illegal instruction)"; break;
    default:      signalName = QString("Signal %1").arg(signal); break;
    }
    
    StackTrace trace = handler->m_stackCollector->captureStackTrace(
        "SystemCrash", 
        "SignalHandler", 
        QString("Process crashed with %1").arg(signalName)
    );
    
    // 根据信号类型发射不同的信号
    switch (signal) {
    case SIGSEGV:
        emit handler->segmentationFault(trace);
        break;
    case SIGABRT:
        emit handler->abortSignal(trace);
        break;
    case SIGFPE:
        emit handler->floatingPointException(trace);
        break;
    }
    
    // 记录崩溃信息
    handler->m_stackCollector->recordException(ExceptionType::UnknownException, 
                                               "System", "Crash", signalName);
    
    // 恢复默认信号处理
    signal(signal, SIG_DFL);
    raise(signal);
}

#include "StackTraceCollector.moc"