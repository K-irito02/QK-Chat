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

Q_DECLARE_LOGGING_CATEGORY(logManager)

/**
 * @brief 日志管理器类
 * 
 * 负责管理不同类型的日志文件，包括：
 * - 连接日志
 * - SSL日志
 * - 错误日志
 * - 心跳日志
 * - 性能监控日志
 */
class LogManager : public QObject
{
    Q_OBJECT
    
public:
    enum LogType {
        Connection,     // 连接日志
        SSL,           // SSL相关日志
        Error,         // 错误日志
        Heartbeat,     // 心跳日志
        Performance,   // 性能监控日志
        Debug,         // 调试日志
        Database,      // 数据库操作日志
        Authentication,// 认证相关日志
        Message,       // 消息处理日志
        System         // 系统级日志
    };
    Q_ENUM(LogType)
    
    static LogManager* instance();
    
    // 日志记录方法
    void writeLog(LogType type, const QString &message, const QString &source = "");
    void writeConnectionLog(const QString &clientId, const QString &action, const QString &details = "");
    void writeSslLog(const QString &clientId, const QString &event, const QString &details = "");
    void writeErrorLog(const QString &error, const QString &source = "", const QString &stackTrace = "");
    void writeHeartbeatLog(const QString &clientId, const QString &status, qint64 latency = -1);
    void writePerformanceLog(const QString &metric, double value, const QString &unit = "");
    void writeDatabaseLog(const QString &operation, const QString &details = "", const QString &source = "");
    void writeAuthenticationLog(const QString &user, const QString &action, const QString &result, const QString &details = "");
    void writeMessageLog(const QString &fromUser, const QString &toUser, const QString &action, const QString &details = "");
    void writeSystemLog(const QString &component, const QString &event, const QString &details = "");
    void writeDebugLog(const QString &message, const QString &source = "", const QString &details = "");
    
    // 日志管理
    void clearLogs();
    void clearLogsOnStartup(); // 启动时清空日志
    void rotateLogs();
    void setMaxFileSize(qint64 maxSize);
    void setMaxFiles(int maxFiles);

    // 配置
    void setLogLevel(const QString &level);
    void enableLogType(LogType type, bool enabled = true);
    bool isLogTypeEnabled(LogType type) const;
    void setConsoleOutput(bool enabled); // 控制台输出开关
    bool isConsoleOutputEnabled() const;
    
signals:
    void logWritten(LogType type, const QString &message);
    void logRotated(LogType type, const QString &oldFile, const QString &newFile);
    
private slots:
    void checkLogRotation();
    
private:
    explicit LogManager(QObject *parent = nullptr);
    ~LogManager();
    
    void initializeLogFiles();
    void writeToFile(LogType type, const QString &message);
    QString getLogFileName(LogType type) const;
    QString getLogTypeString(LogType type) const;
    QString formatLogMessage(const QString &message, const QString &source = "") const;
    void rotateLogFile(LogType type);
    bool shouldRotateLog(LogType type) const;
    void cleanupOldLogFiles(LogType type);
    
    static LogManager* _instance;
    static QMutex _instanceMutex;
    
    QHash<LogType, QFile*> _logFiles;
    QHash<LogType, QTextStream*> _logStreams;
    QHash<LogType, bool> _enabledTypes;
    QMutex _logMutex;
    QTimer *_rotationTimer;
    
    QString _logDirectory;
    qint64 _maxFileSize;
    int _maxFiles;
    QString _logLevel;
    bool _consoleOutputEnabled; // 控制台输出开关
    
    static const qint64 DEFAULT_MAX_FILE_SIZE = 10 * 1024 * 1024; // 10MB
    static const int DEFAULT_MAX_FILES = 5;
    static const int ROTATION_CHECK_INTERVAL = 60000; // 1分钟
};

#endif // LOGMANAGER_H
