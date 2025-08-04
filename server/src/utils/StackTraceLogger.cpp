#include "StackTraceLogger.h"
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QFile>
#include <QDate>
#include <QDateTime>
#include <QThread>
#include <QMutexLocker>

StackTraceLogger* StackTraceLogger::m_instance = nullptr;

StackTraceLogger& StackTraceLogger::instance() {
    if (!m_instance) {
        static QMutex mutex;
        QMutexLocker locker(&mutex);
        if (!m_instance) {
            m_instance = new StackTraceLogger();
        }
    }
    return *m_instance;
}

StackTraceLogger::StackTraceLogger() 
    : m_enableStackTraces(true)
    , m_maxDepth(50) {
    // 默认日志目录
    m_logDirectory = "D:/QT_Learn/Projects/QKChatApp/logs/server";
    
    // 确保日志目录存在
    QDir dir(m_logDirectory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

StackTraceLogger::~StackTraceLogger() {
}

void StackTraceLogger::setLogDirectory(const QString& directory) {
    QMutexLocker locker(&m_mutex);
    m_logDirectory = directory;
    
    QDir dir(m_logDirectory);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

void StackTraceLogger::enableStackTraces(bool enable) {
    QMutexLocker locker(&m_mutex);
    m_enableStackTraces = enable;
}

void StackTraceLogger::setMaxDepth(int depth) {
    QMutexLocker locker(&m_mutex);
    m_maxDepth = depth;
}

QString StackTraceLogger::getCurrentTimestamp() const {
    return QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
}

QString StackTraceLogger::getThreadInfo() const {
    return QString("[Thread ID: %1 | Name: %2]")
        .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()))
        .arg(QThread::currentThread() ? QThread::currentThread()->objectName() : "Unnamed");
}

void StackTraceLogger::logStackTrace(const QString& context, const QString& threadInfo) {
    if (!m_enableStackTraces) return;
    
    QMutexLocker locker(&m_mutex);
    
    QString message;
    QTextStream stream(&message);
    
    stream << "[" << getCurrentTimestamp() << "] "
            << "=== STACK TRACE === " << context << " ===\n"
            << getThreadInfo() << "\n"
            << generateStackTrace() << "\n"
            << "===============================\n\n";
    
    writeToLogFile(message);
}

void StackTraceLogger::logMethodEntry(const QString& methodName, const QString& className) {
    QMutexLocker locker(&m_mutex);
    
    QString fullMethod = className.isEmpty() ? methodName : (className + "::" + methodName);
    m_methodStack.push(fullMethod);
    
    QString message;
    QTextStream stream(&message);
    stream << "[" << getCurrentTimestamp() << "] "
            << "ENTER: " << fullMethod << " - "
            << getThreadInfo() << "\n";
    
    writeToLogFile(message);
}

void StackTraceLogger::logMethodExit(const QString& methodName, const QString& className) {
    QMutexLocker locker(&m_mutex);
    
    QString fullMethod = className.isEmpty() ? methodName : (className + "::" + methodName);
    if (!m_methodStack.isEmpty()) {
        m_methodStack.pop();
    }
    
    QString message;
    QTextStream stream(&message);
    stream << "[" << getCurrentTimestamp() << "] "
            << "EXIT: " << fullMethod << " - "
            << getThreadInfo() << "\n";
    
    writeToLogFile(message);
}

void StackTraceLogger::logThreadBlock(const QString& operation, qint64 timeoutMs) {
    QMutexLocker locker(&m_mutex);
    
    QString message;
    QTextStream stream(&message);
    stream << "[" << getCurrentTimestamp() << "] "
            << "WARNING: Thread blocked in " << operation << " for " << timeoutMs << "ms - "
            << getThreadInfo() << "\n"
            << "Current call stack:\n";
    
    // 输出当前方法栈
    for (int i = 0; i < m_methodStack.size(); ++i) {
        stream << "  [" << i << "] " << m_methodStack[i] << "\n";
    }
    
    writeToLogFile(message);
    
    // 同时输出堆栈跟踪
    logStackTrace("BLOCK_DETECTED");
}

void StackTraceLogger::logCriticalSection(const QString& sectionName, const QString& operation) {
    QMutexLocker locker(&m_mutex);
    
    QString message;
    QTextStream stream(&message);
    stream << "[" << getCurrentTimestamp() << "] "
            << "CRITICAL SECTION: " << sectionName << " - " << operation << " - "
            << getThreadInfo() << "\n";
    
    writeToLogFile(message);
}

QString StackTraceLogger::generateStackTrace() const {
    QString trace;
    QTextStream stream(&trace);
    
#ifdef Q_OS_WIN
    // 简化的堆栈跟踪实现
    stream << "  Stack trace generation simplified for compatibility\n";
    stream << "  Current thread ID: " << QThread::currentThreadId() << "\n";
    stream << "  Process ID: " << GetCurrentProcessId() << "\n";
    
    // 输出当前时间戳
    stream << "  Timestamp: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz") << "\n";
    
    // 输出方法栈信息
    stream << "  Method stack depth: " << m_methodStack.size() << "\n";
    for (int i = 0; i < m_methodStack.size(); ++i) {
        stream << "    [" << i << "] " << m_methodStack[i] << "\n";
    }
    
#else
    stream << "  Stack traces not available on this platform\n";
#endif
    
    return trace;
}

QString StackTraceLogger::getLogFilePath() const {
    QString dateStr = QDate::currentDate().toString("yyyy-MM-dd");
    return QString("%1/stacktrace_%2.log").arg(m_logDirectory).arg(dateStr);
}

void StackTraceLogger::writeToLogFile(const QString& message) {
    QString filePath = getLogFilePath();
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream << message;
        file.close();
    } else {
        qWarning() << "Failed to open stack trace log file:" << filePath;
    }
}

// 辅助函数生成崩溃报告
void generateCrashReport(const QString& reason) {
    StackTraceLogger::instance().logStackTrace("CRASH_REPORT: " + reason);
}