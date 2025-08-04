#include "LogManager.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

Q_LOGGING_CATEGORY(logManager, "qkchat.server.logmanager")

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
    , _logLevel("INFO")
    , _consoleOutputEnabled(true) // 默认启用控制台输出
{
    // 设置日志目录
    QString appDir = QCoreApplication::applicationDirPath();
    _logDirectory = QDir(appDir).absoluteFilePath("../../../../logs/server");
    
    // 确保日志目录存在
    QDir logDir(_logDirectory);
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
    
    // 初始化所有日志类型为启用状态
    _enabledTypes[Connection] = true;
    _enabledTypes[SSL] = true;
    _enabledTypes[Error] = true;
    _enabledTypes[Heartbeat] = true;
    _enabledTypes[Performance] = true;
    _enabledTypes[Debug] = true;
    _enabledTypes[Database] = true;
    _enabledTypes[Authentication] = true;
    _enabledTypes[Message] = true;
    _enabledTypes[System] = true;
    
    // 初始化日志文件
    initializeLogFiles();
    
    // 设置日志轮转定时器
    _rotationTimer = new QTimer(this);
    connect(_rotationTimer, &QTimer::timeout, this, &LogManager::checkLogRotation);
    _rotationTimer->start(ROTATION_CHECK_INTERVAL);
    
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

    QList<LogType> types = {Connection, SSL, Error, Heartbeat, Performance, Debug,
                           Database, Authentication, Message, System};
    
    for (LogType type : types) {
        QString fileName = getLogFileName(type);
        QString filePath = QDir(_logDirectory).absoluteFilePath(fileName);
        
        QFile *file = new QFile(filePath, this);
        
        // 检查文件是否存在且为空
        bool isNewFile = !QFile::exists(filePath) || QFileInfo(filePath).size() == 0;
        
        // 如果是新文件或空文件，先删除再创建
        if (isNewFile && QFile::exists(filePath)) {
            QFile::remove(filePath);
        }
        
        // 统一使用WriteOnly模式，确保文件干净
        if (file->open(QIODevice::WriteOnly)) {
            _logFiles[type] = file;
            _logStreams[type] = new QTextStream(file);
            
            // 设置UTF-8编码
            _logStreams[type]->setEncoding(QStringConverter::Utf8);
            
            qCDebug(logManager) << "Initialized log file:" << filePath;
        } else {
            qCWarning(logManager) << "Failed to open log file:" << filePath << file->errorString();
            delete file;
        }
    }
}

void LogManager::writeLog(LogType type, const QString &message, const QString &source)
{
    if (!isLogTypeEnabled(type)) {
        return;
    }

    QString formattedMessage = formatLogMessage(message, source);
    writeToFile(type, formattedMessage);

    // 如果启用控制台输出，同时输出到控制台
    if (_consoleOutputEnabled) {
        // 根据日志类型选择合适的Qt日志级别
        switch (type) {
        case Error:
            qCCritical(logManager).noquote() << formattedMessage;
            break;
        case SSL:
        case Connection:
        case Heartbeat:
        case Performance:
            qCInfo(logManager).noquote() << formattedMessage;
            break;
        case Debug:
        default:
            qCDebug(logManager).noquote() << formattedMessage;
            break;
        }
    }

    emit logWritten(type, formattedMessage);
}

void LogManager::writeConnectionLog(const QString &clientId, const QString &action, const QString &details)
{
    QString message = QString("[CLIENT:%1] %2").arg(clientId, action);
    if (!details.isEmpty()) {
        message += QString(" - %1").arg(details);
    }
    writeLog(Connection, message, "ConnectionManager");
}

void LogManager::writeSslLog(const QString &clientId, const QString &event, const QString &details)
{
    QString message = QString("[CLIENT:%1] SSL_%2").arg(clientId, event);
    if (!details.isEmpty()) {
        message += QString(" - %1").arg(details);
    }
    writeLog(SSL, message, "SSLManager");
}

void LogManager::writeErrorLog(const QString &error, const QString &source, const QString &stackTrace)
{
    QString message = QString("ERROR: %1").arg(error);
    if (!stackTrace.isEmpty()) {
        message += QString("\nStack Trace:\n%1").arg(stackTrace);
    }
    writeLog(Error, message, source);
}

void LogManager::writeHeartbeatLog(const QString &clientId, const QString &status, qint64 latency)
{
    QString message = QString("[CLIENT:%1] HEARTBEAT_%2").arg(clientId, status);
    if (latency >= 0) {
        message += QString(" - Latency: %1ms").arg(latency);
    }
    writeLog(Heartbeat, message, "HeartbeatManager");
}

void LogManager::writePerformanceLog(const QString &metric, double value, const QString &unit)
{
    QString message = QString("METRIC: %1 = %2 %3").arg(metric).arg(value).arg(unit);
    writeLog(Performance, message, "PerformanceMonitor");
}

void LogManager::writeDatabaseLog(const QString &operation, const QString &details, const QString &source)
{
    QString message = QString("DB_OP: %1").arg(operation);
    if (!details.isEmpty()) {
        message += QString(" - %1").arg(details);
    }
    writeLog(Database, message, source.isEmpty() ? "DatabaseManager" : source);
}

void LogManager::writeAuthenticationLog(const QString &user, const QString &action, const QString &result, const QString &details)
{
    QString message = QString("AUTH: [USER:%1] %2 -> %3").arg(user, action, result);
    if (!details.isEmpty()) {
        message += QString(" - %1").arg(details);
    }
    writeLog(Authentication, message, "AuthenticationManager");
}

void LogManager::writeMessageLog(const QString &fromUser, const QString &toUser, const QString &action, const QString &details)
{
    QString message = QString("MSG: [FROM:%1] [TO:%2] %3").arg(fromUser, toUser, action);
    if (!details.isEmpty()) {
        message += QString(" - %1").arg(details);
    }
    writeLog(Message, message, "MessageManager");
}

void LogManager::writeSystemLog(const QString &component, const QString &event, const QString &details)
{
    QString message = QString("SYS: [%1] %2").arg(component, event);
    if (!details.isEmpty()) {
        message += QString(" - %1").arg(details);
    }
    writeLog(System, message, component);
}

void LogManager::writeDebugLog(const QString &message, const QString &source, const QString &details)
{
    QString logMessage = QString("DEBUG: %1").arg(message);
    if (!details.isEmpty()) {
        logMessage += QString(" - %1").arg(details);
    }
    writeLog(Debug, logMessage, source.isEmpty() ? "DebugManager" : source);
}

void LogManager::writeToFile(LogType type, const QString &message)
{
    QMutexLocker locker(&_logMutex);
    
    if (_logStreams.contains(type) && _logStreams[type]) {
        // 确保消息是纯文本格式
        QString cleanMessage = message;
        // 移除可能的控制字符
        cleanMessage.remove(QRegularExpression(QStringLiteral("[\\x00-\\x1F\\x7F]")));
        
        *_logStreams[type] << cleanMessage << Qt::endl;
        _logStreams[type]->flush();
        
        // 检查是否需要轮转
        if (shouldRotateLog(type)) {
            rotateLogFile(type);
        }
    }
}

QString LogManager::getLogFileName(LogType type) const
{
    switch (type) {
    case Connection:
        return "connection.log";
    case SSL:
        return "ssl.log";
    case Error:
        return "error.log";
    case Heartbeat:
        return "heartbeat.log";
    case Performance:
        return "performance.log";
    case Debug:
        return "debug.log";
    case Database:
        return "database.log";
    case Authentication:
        return "authentication.log";
    case Message:
        return "message.log";
    case System:
        return "system.log";
    default:
        return "general.log";
    }
}

QString LogManager::getLogTypeString(LogType type) const
{
    switch (type) {
    case Connection:
        return "CONN";
    case SSL:
        return "SSL";
    case Error:
        return "ERROR";
    case Heartbeat:
        return "HEART";
    case Performance:
        return "PERF";
    case Debug:
        return "DEBUG";
    case Database:
        return "DB";
    case Authentication:
        return "AUTH";
    case Message:
        return "MSG";
    case System:
        return "SYS";
    default:
        return "GENERAL";
    }
}

QString LogManager::formatLogMessage(const QString &message, const QString &source) const
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString sourceStr = source.isEmpty() ? "Unknown" : source;
    return QString("[%1] [%2] %3").arg(timestamp, sourceStr, message);
}

bool LogManager::shouldRotateLog(LogType type) const
{
    if (!_logFiles.contains(type) || !_logFiles[type]) {
        return false;
    }
    
    return _logFiles[type]->size() >= _maxFileSize;
}

void LogManager::rotateLogFile(LogType type)
{
    if (!_logFiles.contains(type) || !_logFiles[type]) {
        return;
    }
    
    QString currentFileName = _logFiles[type]->fileName();
    QString baseName = QFileInfo(currentFileName).baseName();
    QString extension = QFileInfo(currentFileName).suffix();
    QString dirPath = QFileInfo(currentFileName).absolutePath();
    
    // 关闭当前文件
    _logFiles[type]->close();
    delete _logStreams[type];
    delete _logFiles[type];
    
    // 重命名文件
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString rotatedFileName = QString("%1_%2.%3").arg(baseName, timestamp, extension);
    QString rotatedFilePath = QDir(dirPath).absoluteFilePath(rotatedFileName);
    
    QFile::rename(currentFileName, rotatedFilePath);
    
    // 创建新文件
    QFile *newFile = new QFile(currentFileName, this);
    if (newFile->open(QIODevice::WriteOnly)) {
        _logFiles[type] = newFile;
        _logStreams[type] = new QTextStream(newFile);
        _logStreams[type]->setEncoding(QStringConverter::Utf8);
        
        emit logRotated(type, rotatedFilePath, currentFileName);
        qCInfo(logManager) << "Log rotated:" << rotatedFilePath;
    } else {
        qCWarning(logManager) << "Failed to create new log file:" << currentFileName;
        delete newFile;
    }
    
    // 清理旧文件
    cleanupOldLogFiles(type);
}

void LogManager::cleanupOldLogFiles(LogType type)
{
    QString fileName = getLogFileName(type);
    QString baseName = QFileInfo(fileName).baseName();
    QString extension = QFileInfo(fileName).suffix();
    
    QDir logDir(_logDirectory);
    QStringList filters;
    filters << QString("%1_*.%2").arg(baseName, extension);
    
    QFileInfoList files = logDir.entryInfoList(filters, QDir::Files, QDir::Time | QDir::Reversed);
    
    // 保留最新的 _maxFiles - 1 个文件（当前文件不算在内）
    while (files.size() >= _maxFiles) {
        QFileInfo oldestFile = files.takeLast();
        if (QFile::remove(oldestFile.absoluteFilePath())) {
            qCInfo(logManager) << "Removed old log file:" << oldestFile.fileName();
        }
    }
}

void LogManager::checkLogRotation()
{
    QList<LogType> types = {Connection, SSL, Error, Heartbeat, Performance, Debug,
                           Database, Authentication, Message, System};

    for (LogType type : types) {
        if (shouldRotateLog(type)) {
            rotateLogFile(type);
        }
    }
}

void LogManager::clearLogs()
{
    QMutexLocker locker(&_logMutex);

    for (auto it = _logFiles.begin(); it != _logFiles.end(); ++it) {
        if (it.value()) {
            it.value()->resize(0);
        }
    }

    qCInfo(logManager) << "All logs cleared";
}

void LogManager::clearLogsOnStartup()
{
    QMutexLocker locker(&_logMutex);

    // 清空所有.log文件
    QDir logDir(_logDirectory);
    QStringList filters;
    filters << "*.log";

    QFileInfoList logFiles = logDir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo &fileInfo : logFiles) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            file.close();
            qCInfo(logManager) << "Cleared log file on startup:" << fileInfo.fileName();
        } else {
            qCWarning(logManager) << "Failed to clear log file:" << fileInfo.fileName();
        }
    }

    qCInfo(logManager) << "All log files cleared on startup";
}

void LogManager::setMaxFileSize(qint64 maxSize)
{
    _maxFileSize = maxSize;
}

void LogManager::setMaxFiles(int maxFiles)
{
    _maxFiles = maxFiles;
}

void LogManager::setLogLevel(const QString &level)
{
    _logLevel = level.toUpper();
}

void LogManager::enableLogType(LogType type, bool enabled)
{
    _enabledTypes[type] = enabled;
}

bool LogManager::isLogTypeEnabled(LogType type) const
{
    return _enabledTypes.value(type, true);
}

void LogManager::setConsoleOutput(bool enabled)
{
    _consoleOutputEnabled = enabled;
    qCInfo(logManager) << "Console output" << (enabled ? "enabled" : "disabled");
}

bool LogManager::isConsoleOutputEnabled() const
{
    return _consoleOutputEnabled;
}



void LogManager::rotateLogs()
{
    QList<LogType> types = {Connection, SSL, Error, Heartbeat, Performance, Debug,
                           Database, Authentication, Message, System};

    for (LogType type : types) {
        rotateLogFile(type);
    }
}
