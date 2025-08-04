#include "LogManager.h"
#include "../config/DevelopmentConfig.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QThread>
#include <QDebug>
#include <QProcess>
#include <QSysInfo>

Q_LOGGING_CATEGORY(logManager, "qkchat.client.logmanager")

LogManager* LogManager::_instance = nullptr;
QMutex LogManager::_instanceMutex;

LogManager* LogManager::instance()
{
    QMutexLocker locker(&_instanceMutex);
    if (!_instance) {
        _instance = new LogManager();
    }
    return _instance;
}

LogManager::LogManager(QObject *parent)
    : QObject(parent)
    , _maxFileSize(DEFAULT_MAX_FILE_SIZE)
    , _maxFiles(DEFAULT_MAX_FILES)
    , _logLevel(InfoLevel)
    , _consoleOutput(true)
{
    // 设置日志目录
    QString appDir = QCoreApplication::applicationDirPath();
    _logDirectory = QDir(appDir).absoluteFilePath("../../../../logs/client");
    
    // 确保日志目录存在
    QDir logDir(_logDirectory);
    if (!logDir.exists()) {
        logDir.mkpath(".");
        qDebug() << "Created log directory:" << _logDirectory;
    }
    
    // 初始化所有日志类型为启用状态
    _enabledTypes[Connection] = true;
    _enabledTypes[SSL] = true;
    _enabledTypes[ErrorLog] = true;
    _enabledTypes[Heartbeat] = true;
    _enabledTypes[UI] = true;
    _enabledTypes[Performance] = true;
    _enabledTypes[Diagnostic] = true;
    _enabledTypes[DebugLog] = true;
    
    // 初始化日志类型计数器
    for (auto it = _enabledTypes.begin(); it != _enabledTypes.end(); ++it) {
        _logTypeCounts[it.key()] = 0;
    }
    
    // 初始化日志文件
    initializeLogFiles();

    // 设置日志轮转定时器
    _rotationTimer = new QTimer(this);
    connect(_rotationTimer, &QTimer::timeout, this, &LogManager::checkLogRotation);
    _rotationTimer->start(ROTATION_CHECK_INTERVAL);
    
    // 设置清理定时器
    _cleanupTimer = new QTimer(this);
    connect(_cleanupTimer, &QTimer::timeout, this, &LogManager::cleanupOldLogs);
    _cleanupTimer->start(CLEANUP_INTERVAL);
    
    // 设置监控定时器
    _metricsTimer = new QTimer(this);
    connect(_metricsTimer, &QTimer::timeout, this, &LogManager::updateMetrics);
    _metricsTimer->start(METRICS_UPDATE_INTERVAL);

    // 连接开发环境配置变化
    connect(DevelopmentConfig::instance(), &DevelopmentConfig::debugConfigurationChanged,
            this, [this]() {
        updateFromDevelopmentConfig();
    });

    // 应用开发环境配置
    updateFromDevelopmentConfig();

    qCInfo(logManager) << "LogManager initialized with directory:" << _logDirectory;
}

LogManager::~LogManager()
{
    QMutexLocker locker(&_logMutex);
    
    // 关闭所有日志文件
    for (auto it = _logFiles.begin(); it != _logFiles.end(); ++it) {
        if (it.value()) {
            it.value()->close();
            delete it.value();
        }
    }
    
    for (auto it = _logStreams.begin(); it != _logStreams.end(); ++it) {
        delete it.value();
    }
    
    _logFiles.clear();
    _logStreams.clear();
}

void LogManager::initializeLogFiles()
{
    QMutexLocker locker(&_logMutex);
    
    QList<LogType> types = {Connection, SSL, ErrorLog, Heartbeat, UI, Performance, Diagnostic, DebugLog};
    
    for (LogType type : types) {
        QString fileName = getLogFileName(type);
        QString filePath = QDir(_logDirectory).absoluteFilePath(fileName);
        
        QFile *file = new QFile(filePath, this);
        if (file->open(QIODevice::WriteOnly | QIODevice::Append)) {
            _logFiles[type] = file;
            _logStreams[type] = new QTextStream(file);
            qCDebug(logManager) << "Initialized log file:" << filePath;
        } else {
            qCWarning(logManager) << "Failed to open log file:" << filePath << file->errorString();
        }
    }
}

void LogManager::writeLog(LogType type, LogLevel level, const QString &message, const QString &source)
{
    if (!_enabledTypes.value(type, false) || level < _logLevel) {
        return;
    }
    
    QMutexLocker locker(&_logMutex);
    
    QString formattedMessage = formatLogMessage(type, level, message, source);
    
    // 写入文件
    writeToFile(type, level, formattedMessage);
    
    // 输出到控制台
    if (_consoleOutput) {
        writeToConsole(type, level, formattedMessage);
    }
    
    // 更新统计
    _logTypeCounts[type]++;
    
    emit logWritten(type, level, formattedMessage);
}

void LogManager::writeConnectionLog(const QString &action, const QString &details, LogLevel level)
{
    QString message = QString("Connection: %1").arg(action);
    if (!details.isEmpty()) {
        message += QString(" - %1").arg(details);
    }
    writeLog(Connection, level, message, "NetworkClient");
}

void LogManager::writeSslLog(const QString &event, const QString &details, LogLevel level)
{
    QString message = QString("SSL: %1").arg(event);
    if (!details.isEmpty()) {
        message += QString(" - %1").arg(details);
    }
    writeLog(SSL, level, message, "SSLManager");
}

void LogManager::writeErrorLog(const QString &error, const QString &source, const QString &stackTrace)
{
    QString message = QString("Error: %1").arg(error);
    if (!stackTrace.isEmpty()) {
        message += QString("\nStackTrace: %1").arg(stackTrace);
    }
    writeLog(ErrorLog, ErrorLevel, message, source);
    
    // 记录错误统计
    _errorCounts[source]++;
    emit errorRecorded(error, source);
}

void LogManager::writeHeartbeatLog(const QString &status, qint64 latency)
{
    QString message = QString("Heartbeat: %1").arg(status);
    if (latency >= 0) {
        message += QString(" (Latency: %1ms)").arg(latency);
    }
    writeLog(Heartbeat, InfoLevel, message, "HeartbeatManager");
}

void LogManager::writeUILog(const QString &action, const QString &details)
{
    QString message = QString("UI: %1").arg(action);
    if (!details.isEmpty()) {
        message += QString(" - %1").arg(details);
    }
    writeLog(UI, InfoLevel, message, "UI");
}

void LogManager::writePerformanceLog(const QString &metric, double value, const QString &unit)
{
    QString message = QString("Performance: %1 = %2 %3").arg(metric).arg(value).arg(unit);
    writeLog(Performance, InfoLevel, message, "PerformanceMonitor");
    
    // 记录指标
    recordMetric(metric, value, unit);
}

void LogManager::writeDiagnosticLog(const QString &component, const QString &status, const QString &details)
{
    QString message = QString("Diagnostic: %1 - %2").arg(component).arg(status);
    if (!details.isEmpty()) {
        message += QString(" (%1)").arg(details);
    }
    writeLog(Diagnostic, InfoLevel, message, "DiagnosticManager");
}

void LogManager::recordMetric(const QString &name, double value, const QString &unit)
{
    QMutexLocker locker(&_logMutex);
    
    if (!_metrics.contains(name)) {
        _metrics[name] = QQueue<double>();
    }
    
    QQueue<double> &queue = _metrics[name];
    queue.enqueue(value);
    
    // 保持历史记录在限制范围内
    while (queue.size() > MAX_METRICS_HISTORY) {
        queue.dequeue();
    }
    
    emit metricRecorded(name, value, unit);
}

void LogManager::recordEvent(const QString &event, const QString &category)
{
    QMutexLocker locker(&_logMutex);
    QString key = category.isEmpty() ? event : QString("%1.%2").arg(category).arg(event);
    _eventCounts[key]++;
}

void LogManager::recordError(const QString &error, const QString &component)
{
    QMutexLocker locker(&_logMutex);
    _errorCounts[component]++;
    emit errorRecorded(error, component);
}

void LogManager::startDiagnosticSession(const QString &sessionId)
{
    QMutexLocker locker(&_logMutex);
    _diagnosticSessions[sessionId] = QHash<QString, QString>();
    writeDiagnosticLog("Session", "Started", sessionId);
}

void LogManager::endDiagnosticSession(const QString &sessionId)
{
    QMutexLocker locker(&_logMutex);
    if (_diagnosticSessions.contains(sessionId)) {
        writeDiagnosticLog("Session", "Ended", sessionId);
        _diagnosticSessions.remove(sessionId);
    }
}

void LogManager::addDiagnosticInfo(const QString &sessionId, const QString &key, const QString &value)
{
    QMutexLocker locker(&_logMutex);
    if (_diagnosticSessions.contains(sessionId)) {
        _diagnosticSessions[sessionId][key] = value;
        writeDiagnosticLog("Info", QString("%1: %2").arg(key).arg(value), sessionId);
    }
}

QString LogManager::generateDiagnosticReport(const QString &sessionId)
{
    QMutexLocker locker(&_logMutex);
    
    if (!_diagnosticSessions.contains(sessionId)) {
        return "Session not found";
    }
    
    QString report = QString("=== Diagnostic Report for Session: %1 ===\n").arg(sessionId);
    report += QString("Generated: %1\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    report += QString("System: %1 %2\n").arg(QSysInfo::prettyProductName()).arg(QSysInfo::currentCpuArchitecture());
    
    const QHash<QString, QString> &session = _diagnosticSessions[sessionId];
    for (auto it = session.begin(); it != session.end(); ++it) {
        report += QString("%1: %2\n").arg(it.key()).arg(it.value());
    }
    
    return report;
}

void LogManager::clearLogs()
{
    QMutexLocker locker(&_logMutex);
    
    // 关闭现有文件
    for (auto it = _logFiles.begin(); it != _logFiles.end(); ++it) {
        if (it.value()) {
            it.value()->close();
        }
    }
    
    // 清空目录中的所有.log文件
    QDir logDir(_logDirectory);
    QStringList filters;
    filters << "*.log";
    QFileInfoList logFiles = logDir.entryInfoList(filters, QDir::Files);
    
    for (const QFileInfo &fileInfo : logFiles) {
        QFile::remove(fileInfo.absoluteFilePath());
    }
    
    // 重新初始化文件
    initializeLogFiles();
    
    qCInfo(logManager) << "All logs cleared";
}

void LogManager::writeToFile(LogType type, LogLevel level, const QString &message)
{
    if (!_logFiles.contains(type) || !_logStreams.contains(type)) {
        return;
    }
    
    QTextStream *stream = _logStreams[type];
    if (stream) {
        *stream << message << "\n";
        stream->flush();
    }
}

QString LogManager::getLogFileName(LogType type) const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString typeStr = getLogTypeString(type).toLower();
    return QString("%1_%2.log").arg(typeStr).arg(timestamp);
}

QString LogManager::getLogTypeString(LogType type) const
{
    switch (type) {
        case Connection: return "Connection";
        case SSL: return "SSL";
        case ErrorLog: return "Error";
        case Heartbeat: return "Heartbeat";
        case UI: return "UI";
        case Performance: return "Performance";
        case Diagnostic: return "Diagnostic";
        case DebugLog: return "Debug";
        default: return "Unknown";
    }
}

QString LogManager::getLogLevelString(LogLevel level) const
{
    switch (level) {
        case DebugLevel: return "DEBUG";
        case InfoLevel: return "INFO";
        case WarningLevel: return "WARN";
        case ErrorLevel: return "ERROR";
        case CriticalLevel: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

QString LogManager::formatLogMessage(LogType type, LogLevel level, const QString &message, const QString &source) const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString levelStr = getLogLevelString(level);
    QString typeStr = getLogTypeString(type);
    QString sourceStr = source.isEmpty() ? "Unknown" : source;
    
    return QString("[%1] [%2] [%3] [%4] %5")
           .arg(timestamp)
           .arg(levelStr)
           .arg(typeStr)
           .arg(sourceStr)
           .arg(message);
}

void LogManager::writeToConsole(LogType type, LogLevel level, const QString &message)
{
    QString levelStr = getLogLevelString(level);
    QString typeStr = getLogTypeString(type);
    
    // 根据级别使用不同的颜色输出
    switch (level) {
        case DebugLevel:
            qDebug() << QString("[%1] [%2] %3").arg(levelStr).arg(typeStr).arg(message);
            break;
        case InfoLevel:
            qInfo() << QString("[%1] [%2] %3").arg(levelStr).arg(typeStr).arg(message);
            break;
        case WarningLevel:
            qWarning() << QString("[%1] [%2] %3").arg(levelStr).arg(typeStr).arg(message);
            break;
        case ErrorLevel:
        case CriticalLevel:
            qCritical() << QString("[%1] [%2] %3").arg(levelStr).arg(typeStr).arg(message);
            break;
    }
}

void LogManager::checkLogRotation()
{
    QMutexLocker locker(&_logMutex);
    
    for (auto it = _logFiles.begin(); it != _logFiles.end(); ++it) {
        if (shouldRotateLog(it.key())) {
            rotateLogFile(it.key());
        }
    }
}

bool LogManager::shouldRotateLog(LogType type) const
{
    if (!_logFiles.contains(type)) {
        return false;
    }
    
    QFile *file = _logFiles[type];
    if (!file) {
        return false;
    }
    
    return file->size() > _maxFileSize;
}

void LogManager::rotateLogFile(LogType type)
{
    if (!_logFiles.contains(type)) {
        return;
    }
    
    QFile *file = _logFiles[type];
    if (!file) {
        return;
    }
    
    QString currentFileName = file->fileName();
    QString rotatedFilePath = currentFileName + QString(".%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    
    // 关闭当前文件
    file->close();
    
    // 重命名文件
    if (QFile::rename(currentFileName, rotatedFilePath)) {
        // 创建新文件
        if (file->open(QIODevice::WriteOnly | QIODevice::Append)) {
            qCInfo(logManager) << "Log rotated:" << rotatedFilePath;
            emit logRotated(type, rotatedFilePath, currentFileName);
        } else {
            qCWarning(logManager) << "Failed to create new log file:" << currentFileName;
        }
    }
    
    cleanupOldLogFiles(type);
}

void LogManager::cleanupOldLogFiles(LogType type)
{
    QString typeStr = getLogTypeString(type).toLower();
    QDir logDir(_logDirectory);
    QStringList filters;
    filters << QString("%1_*.log*").arg(typeStr);
    
    QFileInfoList logFiles = logDir.entryInfoList(filters, QDir::Files);
    
    // 按修改时间排序
    std::sort(logFiles.begin(), logFiles.end(), 
              [](const QFileInfo &a, const QFileInfo &b) {
                  return a.lastModified() > b.lastModified();
              });
    
    // 删除超出最大文件数的旧文件
    while (logFiles.size() > _maxFiles) {
        QFileInfo oldestFile = logFiles.takeLast();
        QFile::remove(oldestFile.absoluteFilePath());
        qCInfo(logManager) << "Removed old log file:" << oldestFile.fileName();
    }
}

void LogManager::cleanupOldLogs()
{
    QMutexLocker locker(&_logMutex);
    
    // 清理超过7天的日志文件
    QDir logDir(_logDirectory);
    QStringList filters;
    filters << "*.log*";
    QFileInfoList allLogFiles = logDir.entryInfoList(filters, QDir::Files);
    
    QDateTime cutoffDate = QDateTime::currentDateTime().addDays(-7);
    
    for (const QFileInfo &fileInfo : allLogFiles) {
        if (fileInfo.lastModified() < cutoffDate) {
            QFile::remove(fileInfo.absoluteFilePath());
            qCInfo(logManager) << "Removed old log file:" << fileInfo.fileName();
        }
    }
}

void LogManager::updateMetrics()
{
    // 记录系统性能指标
    recordMetric("memory_usage", QProcess::systemEnvironment().size(), "env_vars");
    recordMetric("log_file_count", getLogFileCount(), "files");
    recordMetric("total_log_size", getTotalLogSize(), "bytes");
    
    saveMetrics();
}

void LogManager::saveMetrics()
{
    QString metricsFile = QDir(_logDirectory).absoluteFilePath("metrics.json");
    QFile file(metricsFile);
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject metrics;
        metrics["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        metrics["log_file_count"] = getLogFileCount();
        metrics["total_log_size"] = getTotalLogSize();
        
        QJsonObject typeStats;
        QHash<LogType, int> stats = getLogTypeStatistics();
        for (auto it = stats.begin(); it != stats.end(); ++it) {
            typeStats[getLogTypeString(it.key())] = it.value();
        }
        metrics["log_type_statistics"] = typeStats;
        
        QJsonDocument doc(metrics);
        file.write(doc.toJson());
    }
}

void LogManager::setLogLevel(LogLevel level)
{
    _logLevel = level;
}

void LogManager::setConsoleOutput(bool enabled)
{
    _consoleOutput = enabled;
}

void LogManager::enableLogType(LogType type, bool enabled)
{
    _enabledTypes[type] = enabled;
}

bool LogManager::isLogTypeEnabled(LogType type) const
{
    return _enabledTypes.value(type, false);
}

void LogManager::setLogDirectory(const QString &directory)
{
    _logDirectory = directory;
    QDir logDir(_logDirectory);
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
}

int LogManager::getLogFileCount() const
{
    QDir logDir(_logDirectory);
    QStringList filters;
    filters << "*.log*";
    return logDir.entryInfoList(filters, QDir::Files).size();
}

qint64 LogManager::getTotalLogSize() const
{
    QDir logDir(_logDirectory);
    QStringList filters;
    filters << "*.log*";
    QFileInfoList logFiles = logDir.entryInfoList(filters, QDir::Files);
    
    qint64 totalSize = 0;
    for (const QFileInfo &fileInfo : logFiles) {
        totalSize += fileInfo.size();
    }
    
    return totalSize;
}

QHash<LogManager::LogType, int> LogManager::getLogTypeStatistics() const
{
    return _logTypeCounts;
}

void LogManager::updateFromDevelopmentConfig()
{
    DevelopmentConfig *devConfig = DevelopmentConfig::instance();
    
    _logLevel = devConfig->isVerboseLogging() ? DebugLevel : InfoLevel;
    _consoleOutput = devConfig->isLogToFile();
    
    qCInfo(logManager) << "Log configuration updated from DevelopmentConfig";
    qCInfo(logManager) << "Log level:" << getLogLevelString(_logLevel);
    qCInfo(logManager) << "Verbose logging:" << devConfig->isVerboseLogging();
    qCInfo(logManager) << "Log to file:" << devConfig->isLogToFile();
}
