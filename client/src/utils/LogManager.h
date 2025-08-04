#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMutex>
#include <QTimer>
#include <QDir>
#include <QLoggingCategory>
#include <QHash>
#include <QQueue>

Q_DECLARE_LOGGING_CATEGORY(logManager)

/**
 * @brief 客户端日志管理器类
 * 
 * 负责管理不同类型的日志文件，包括：
 * - 连接日志
 * - SSL日志
 * - 错误日志
 * - 心跳日志
 * - UI操作日志
 * - 性能监控日志
 * - 诊断日志
 */
class LogManager : public QObject
{
    Q_OBJECT
    
public:
    enum LogType {
        Connection,     // 连接日志
        SSL,           // SSL相关日志
        ErrorLog,      // 错误日志
        Heartbeat,     // 心跳日志
        UI,            // UI操作日志
        Performance,   // 性能监控日志
        Diagnostic,    // 诊断日志
        DebugLog       // 调试日志
    };
    Q_ENUM(LogType)
    
    enum LogLevel {
        DebugLevel,
        InfoLevel,
        WarningLevel,
        ErrorLevel,
        CriticalLevel
    };
    Q_ENUM(LogLevel)
    
    static LogManager* instance();
    
    // 日志记录方法
    void writeLog(LogType type, LogLevel level, const QString &message, const QString &source = "");
    void writeConnectionLog(const QString &action, const QString &details = "", LogLevel level = InfoLevel);
    void writeSslLog(const QString &event, const QString &details = "", LogLevel level = InfoLevel);
    void writeErrorLog(const QString &error, const QString &source = "", const QString &stackTrace = "");
    void writeHeartbeatLog(const QString &status, qint64 latency = -1);
    void writeUILog(const QString &action, const QString &details = "");
    void writePerformanceLog(const QString &metric, double value, const QString &unit);
    void writeDiagnosticLog(const QString &component, const QString &status, const QString &details = "");
    
    // 监控方法
    void recordMetric(const QString &name, double value, const QString &unit = "");
    void recordEvent(const QString &event, const QString &category = "");
    void recordError(const QString &error, const QString &component = "");
    
    // 诊断方法
    void startDiagnosticSession(const QString &sessionId);
    void endDiagnosticSession(const QString &sessionId);
    void addDiagnosticInfo(const QString &sessionId, const QString &key, const QString &value);
    QString generateDiagnosticReport(const QString &sessionId);
    
    // 日志管理
    void clearLogs();
    void rotateLogs();
    void setMaxFileSize(qint64 maxSize);
    void setMaxFiles(int maxFiles);
    void setLogLevel(LogLevel level);
    void setConsoleOutput(bool enabled);
    
    // 配置
    void enableLogType(LogType type, bool enabled = true);
    bool isLogTypeEnabled(LogType type) const;
    void setLogDirectory(const QString &directory);
    
    // 统计信息
    int getLogFileCount() const;
    qint64 getTotalLogSize() const;
    QHash<LogType, int> getLogTypeStatistics() const;
    
signals:
    void logWritten(LogType type, LogLevel level, const QString &message);
    void logRotated(LogType type, const QString &oldFile, const QString &newFile);
    void metricRecorded(const QString &name, double value, const QString &unit);
    void errorRecorded(const QString &error, const QString &component);
    
private slots:
    void checkLogRotation();
    void cleanupOldLogs();
    
private:
    explicit LogManager(QObject *parent = nullptr);
    ~LogManager();
    
    void initializeLogFiles();
    void writeToFile(LogType type, LogLevel level, const QString &message);
    QString getLogFileName(LogType type) const;
    QString getLogTypeString(LogType type) const;
    QString getLogLevelString(LogLevel level) const;
    QString formatLogMessage(LogType type, LogLevel level, const QString &message, const QString &source = "") const;
    void rotateLogFile(LogType type);
    bool shouldRotateLog(LogType type) const;
    void cleanupOldLogFiles(LogType type);
    void updateFromDevelopmentConfig();
    void writeToConsole(LogType type, LogLevel level, const QString &message);
    
    // 监控相关
    void updateMetrics();
    void saveMetrics();
    
    static LogManager* _instance;
    static QMutex _instanceMutex;
    
    QHash<LogType, QFile*> _logFiles;
    QHash<LogType, QTextStream*> _logStreams;
    QHash<LogType, bool> _enabledTypes;
    QHash<LogType, int> _logTypeCounts;
    QMutex _logMutex;
    QTimer *_rotationTimer;
    QTimer *_cleanupTimer;
    QTimer *_metricsTimer;
    
    QString _logDirectory;
    qint64 _maxFileSize;
    int _maxFiles;
    LogLevel _logLevel;
    bool _consoleOutput;
    
    // 监控数据
    QHash<QString, QQueue<double>> _metrics;
    QHash<QString, int> _eventCounts;
    QHash<QString, int> _errorCounts;
    
    // 诊断会话
    QHash<QString, QHash<QString, QString>> _diagnosticSessions;
    
    static const qint64 DEFAULT_MAX_FILE_SIZE = 5 * 1024 * 1024; // 5MB
    static const int DEFAULT_MAX_FILES = 3;
    static const int ROTATION_CHECK_INTERVAL = 60000; // 1分钟
    static const int CLEANUP_INTERVAL = 300000; // 5分钟
    static const int METRICS_UPDATE_INTERVAL = 30000; // 30秒
    static const int MAX_METRICS_HISTORY = 100;
};

#endif // LOGMANAGER_H
