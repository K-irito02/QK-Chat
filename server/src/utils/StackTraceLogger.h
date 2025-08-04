#ifndef STACKTRACELOGGER_H
#define STACKTRACELOGGER_H

#include <QString>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QMutex>
#include <QDebug>
#include <QThread>
#include <QStack>
#include <QElapsedTimer>

#ifdef Q_OS_WIN
    #include <windows.h>
    #include <dbghelp.h>
    #pragma comment(lib, "dbghelp.lib")
#endif

class StackTraceLogger {
public:
    static StackTraceLogger& instance();
    
    void logStackTrace(const QString& context = "", const QString& threadInfo = "");
    void logMethodEntry(const QString& methodName, const QString& className = "");
    void logMethodExit(const QString& methodName, const QString& className = "");
    void logThreadBlock(const QString& operation, qint64 timeoutMs);
    void logCriticalSection(const QString& sectionName, const QString& operation);
    
    void setLogDirectory(const QString& directory);
    void enableStackTraces(bool enable);
    void setMaxDepth(int depth);

private:
    StackTraceLogger();
    ~StackTraceLogger();
    
    QString getCurrentTimestamp() const;
    QString getThreadInfo() const;
    QString generateStackTrace() const;
    QString getLogFilePath() const;
    
    void writeToLogFile(const QString& message);
    
    QString m_logDirectory;
    QMutex m_mutex;
    bool m_enableStackTraces;
    int m_maxDepth;
    QStack<QString> m_methodStack;
    
    static StackTraceLogger* m_instance;
};

// 便捷的宏定义
#define LOG_METHOD_ENTRY() StackTraceLogger::instance().logMethodEntry(__FUNCTION__, __FILE__)
#define LOG_METHOD_EXIT() StackTraceLogger::instance().logMethodExit(__FUNCTION__, __FILE__)
#define LOG_STACK_TRACE(context) StackTraceLogger::instance().logStackTrace(context)
#define LOG_THREAD_BLOCK(operation, timeout) StackTraceLogger::instance().logThreadBlock(operation, timeout)
#define LOG_CRITICAL_SECTION(section, operation) StackTraceLogger::instance().logCriticalSection(section, operation)

// 方法追踪器类，自动记录进入和退出
class MethodTracer {
public:
    MethodTracer(const QString& methodName, const QString& className = "") 
        : m_method(methodName), m_class(className) {
        StackTraceLogger::instance().logMethodEntry(m_method, m_class);
    }
    
    ~MethodTracer() {
        StackTraceLogger::instance().logMethodExit(m_method, m_class);
    }

private:
    QString m_method;
    QString m_class;
};

#define METHOD_TRACER() MethodTracer tracer(__FUNCTION__, __FILE__)

#endif // STACKTRACELOGGER_H