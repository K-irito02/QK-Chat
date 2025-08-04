#include "MonitorManager.h"
#include "LogManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QSysInfo>
#include <QHostInfo>
#include <QThread>

MonitorManager* MonitorManager::_instance = nullptr;
QMutex MonitorManager::_instanceMutex;

MonitorManager* MonitorManager::instance()
{
    QMutexLocker locker(&_instanceMutex);
    if (!_instance) {
        _instance = new MonitorManager();
    }
    return _instance;
}

MonitorManager::MonitorManager(QObject *parent)
    : QObject(parent)
    , _isMonitoring(false)
    , _monitoringInterval(DEFAULT_MONITORING_INTERVAL)
    , _maxHistorySize(DEFAULT_MAX_HISTORY_SIZE)
    , _cpuUsage(0.0)
    , _memoryUsage(0)
    , _availableMemory(0)
    , _networkConnected(false)
    , _totalLatency(0)
    , _latencyCount(0)
{
    initializeMonitoring();
    setupTimers();
    
    // 启用所有监控类型
    for (int i = System; i <= CPU; ++i) {
        _enabledMetrics[static_cast<MetricType>(i)] = true;
    }
    
    qDebug() << "MonitorManager initialized";
}

MonitorManager::~MonitorManager()
{
    stopMonitoring();
    qDebug() << "MonitorManager destroyed";
}

void MonitorManager::initializeMonitoring()
{
    loadSystemInfo();
    calculateMemoryUsage();
}

void MonitorManager::setupTimers()
{
    _monitoringTimer = new QTimer(this);
    connect(_monitoringTimer, &QTimer::timeout, this, &MonitorManager::updateSystemMetrics);
    
    _saveTimer = new QTimer(this);
    connect(_saveTimer, &QTimer::timeout, this, &MonitorManager::saveMetrics);
    
    _thresholdTimer = new QTimer(this);
    connect(_thresholdTimer, &QTimer::timeout, this, &MonitorManager::checkThresholds);
}

void MonitorManager::loadSystemInfo()
{
    _systemInfo.osName = QSysInfo::prettyProductName();
    _systemInfo.osVersion = QSysInfo::productVersion();
    _systemInfo.cpuArchitecture = QSysInfo::currentCpuArchitecture();
    _systemInfo.cpuCount = QThread::idealThreadCount();
    _systemInfo.hostName = QHostInfo::localHostName();
    _systemInfo.userName = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    
    // 获取内存信息
    #ifdef Q_OS_WINDOWS
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    _systemInfo.totalMemory = memInfo.ullTotalPhys;
    _systemInfo.availableMemory = memInfo.ullAvailPhys;
    #else
    // Linux/Unix 系统
    QFile memInfo("/proc/meminfo");
    if (memInfo.open(QIODevice::ReadOnly)) {
        QTextStream stream(&memInfo);
        QString line;
        while (stream.readLineInto(&line)) {
            if (line.startsWith("MemTotal:")) {
                QString value = line.split(" ").last();
                _systemInfo.totalMemory = value.toLongLong() * 1024;
            } else if (line.startsWith("MemAvailable:")) {
                QString value = line.split(" ").last();
                _systemInfo.availableMemory = value.toLongLong() * 1024;
            }
        }
    }
    #endif
}

void MonitorManager::startMonitoring()
{
    if (_isMonitoring) {
        return;
    }
    
    _isMonitoring = true;
    _monitoringTimer->start(_monitoringInterval);
    _saveTimer->start(DEFAULT_SAVE_INTERVAL);
    _thresholdTimer->start(DEFAULT_THRESHOLD_CHECK_INTERVAL);
    
    LogManager::instance()->writeDiagnosticLog("Monitor", "Started", "Monitoring system started");
    qDebug() << "System monitoring started";
}

void MonitorManager::stopMonitoring()
{
    if (!_isMonitoring) {
        return;
    }
    
    _isMonitoring = false;
    _monitoringTimer->stop();
    _saveTimer->stop();
    _thresholdTimer->stop();
    
    LogManager::instance()->writeDiagnosticLog("Monitor", "Stopped", "Monitoring system stopped");
    qDebug() << "System monitoring stopped";
}

bool MonitorManager::isMonitoring() const
{
    return _isMonitoring;
}

void MonitorManager::recordMetric(const QString &name, double value, const QString &unit, MetricType type)
{
    if (!_enabledMetrics.value(type, false)) {
        return;
    }
    
    QMutexLocker locker(&_dataMutex);
    
    MetricData data;
    data.name = name;
    data.value = value;
    data.unit = unit;
    data.timestamp = QDateTime::currentDateTime();
    data.type = type;
    
    if (!_metrics.contains(name)) {
        _metrics[name] = QQueue<MetricData>();
    }
    
    QQueue<MetricData> &queue = _metrics[name];
    queue.enqueue(data);
    
    // 保持历史记录在限制范围内
    while (queue.size() > _maxHistorySize) {
        queue.dequeue();
    }
    
    emit metricUpdated(name, value, unit);
    
    // 记录到日志
    LogManager::instance()->writePerformanceLog(name, value, unit);
}

void MonitorManager::recordEvent(const QString &event, const QString &category)
{
    QMutexLocker locker(&_dataMutex);
    QString key = category.isEmpty() ? event : QString("%1.%2").arg(category).arg(event);
    _eventCounts[key]++;
}

void MonitorManager::recordError(const QString &error, const QString &component)
{
    QMutexLocker locker(&_dataMutex);
    _errorCounts[component]++;
    
    // 记录到日志
    LogManager::instance()->writeErrorLog(error, component);
}

MonitorManager::SystemInfo MonitorManager::getSystemInfo() const
{
    return _systemInfo;
}

double MonitorManager::getCpuUsage() const
{
    return _cpuUsage;
}

qint64 MonitorManager::getMemoryUsage() const
{
    return _memoryUsage;
}

qint64 MonitorManager::getAvailableMemory() const
{
    return _availableMemory;
}

void MonitorManager::setNetworkStatus(bool connected)
{
    if (_networkConnected != connected) {
        _networkConnected = connected;
        emit networkStatusChanged(connected);
        
        LogManager::instance()->writeConnectionLog(
            connected ? "Connected" : "Disconnected",
            "Network status changed"
        );
    }
}

bool MonitorManager::isNetworkConnected() const
{
    return _networkConnected;
}

void MonitorManager::recordNetworkLatency(qint64 latency)
{
    QMutexLocker locker(&_dataMutex);
    
    _networkLatencies.enqueue(latency);
    _totalLatency += latency;
    _latencyCount++;
    
    // 保持历史记录在限制范围内
    while (_networkLatencies.size() > _maxHistorySize) {
        qint64 oldLatency = _networkLatencies.dequeue();
        _totalLatency -= oldLatency;
    }
    
    recordMetric("network_latency", latency, "ms", Network);
}

qint64 MonitorManager::getAverageLatency() const
{
    QMutexLocker locker(&_dataMutex);
    return _latencyCount > 0 ? _totalLatency / _latencyCount : 0;
}

void MonitorManager::recordResponseTime(const QString &operation, qint64 timeMs)
{
    QMutexLocker locker(&_dataMutex);
    
    if (!_responseTimes.contains(operation)) {
        _responseTimes[operation] = QQueue<qint64>();
        _totalResponseTimes[operation] = 0;
        _responseCounts[operation] = 0;
    }
    
    QQueue<qint64> &queue = _responseTimes[operation];
    queue.enqueue(timeMs);
    _totalResponseTimes[operation] += timeMs;
    _responseCounts[operation]++;
    
    // 保持历史记录在限制范围内
    while (queue.size() > _maxHistorySize) {
        qint64 oldTime = queue.dequeue();
        _totalResponseTimes[operation] -= oldTime;
    }
    
    recordMetric(QString("response_time_%1").arg(operation), timeMs, "ms", Performance);
}

qint64 MonitorManager::getAverageResponseTime(const QString &operation) const
{
    QMutexLocker locker(&_dataMutex);
    int count = _responseCounts.value(operation, 0);
    return count > 0 ? _totalResponseTimes.value(operation, 0) / count : 0;
}

QHash<QString, double> MonitorManager::getMetrics() const
{
    QMutexLocker locker(&_dataMutex);
    QHash<QString, double> result;
    
    for (auto it = _metrics.begin(); it != _metrics.end(); ++it) {
        if (!it.value().isEmpty()) {
            result[it.key()] = it.value().last().value;
        }
    }
    
    return result;
}

QHash<QString, int> MonitorManager::getEventCounts() const
{
    QMutexLocker locker(&_dataMutex);
    return _eventCounts;
}

QHash<QString, int> MonitorManager::getErrorCounts() const
{
    QMutexLocker locker(&_dataMutex);
    return _errorCounts;
}

QString MonitorManager::generateSystemReport() const
{
    QString report = "=== System Report ===\n";
    report += QString("OS: %1 %2\n").arg(_systemInfo.osName).arg(_systemInfo.osVersion);
    report += QString("Architecture: %1\n").arg(_systemInfo.cpuArchitecture);
    report += QString("CPU Cores: %1\n").arg(_systemInfo.cpuCount);
    report += QString("Host Name: %1\n").arg(_systemInfo.hostName);
    report += QString("User: %1\n").arg(_systemInfo.userName);
    report += QString("Total Memory: %1 MB\n").arg(_systemInfo.totalMemory / (1024 * 1024));
    report += QString("Available Memory: %1 MB\n").arg(_systemInfo.availableMemory / (1024 * 1024));
    report += QString("CPU Usage: %1%\n").arg(_cpuUsage, 0, 'f', 2);
    report += QString("Memory Usage: %1 MB\n").arg(_memoryUsage / (1024 * 1024));
    report += QString("Network Connected: %1\n").arg(_networkConnected ? "Yes" : "No");
    report += QString("Average Latency: %1 ms\n").arg(getAverageLatency());
    
    return report;
}

QString MonitorManager::generatePerformanceReport() const
{
    QString report = "=== Performance Report ===\n";
    
    QHash<QString, double> metrics = getMetrics();
    for (auto it = metrics.begin(); it != metrics.end(); ++it) {
        if (it.key().startsWith("response_time_")) {
            QString operation = it.key().mid(13); // 移除 "response_time_" 前缀
            report += QString("%1: %2 ms\n").arg(operation).arg(it.value());
        }
    }
    
    return report;
}

QString MonitorManager::generateErrorReport() const
{
    QString report = "=== Error Report ===\n";
    
    QHash<QString, int> errorCounts = getErrorCounts();
    for (auto it = errorCounts.begin(); it != errorCounts.end(); ++it) {
        report += QString("%1: %2 errors\n").arg(it.key()).arg(it.value());
    }
    
    return report;
}

void MonitorManager::setMonitoringInterval(int intervalMs)
{
    _monitoringInterval = intervalMs;
    if (_monitoringTimer->isActive()) {
        _monitoringTimer->setInterval(intervalMs);
    }
}

void MonitorManager::setMaxHistorySize(int size)
{
    _maxHistorySize = size;
}

void MonitorManager::enableMetricType(MetricType type, bool enabled)
{
    _enabledMetrics[type] = enabled;
}

void MonitorManager::updateSystemMetrics()
{
    calculateCpuUsage();
    calculateMemoryUsage();
    
    // 记录系统指标
    recordMetric("cpu_usage", _cpuUsage, "%", CPU);
    recordMetric("memory_usage", _memoryUsage / (1024 * 1024), "MB", Memory);
    recordMetric("available_memory", _availableMemory / (1024 * 1024), "MB", Memory);
    recordMetric("memory_percentage", (_memoryUsage * 100.0) / _systemInfo.totalMemory, "%", Memory);
}

void MonitorManager::updateNetworkMetrics()
{
    // 网络指标更新
    recordMetric("network_connected", _networkConnected ? 1 : 0, "", Network);
}

void MonitorManager::updatePerformanceMetrics()
{
    // 性能指标更新
    QHash<QString, qint64> avgResponseTimes;
    for (auto it = _responseTimes.begin(); it != _responseTimes.end(); ++it) {
        avgResponseTimes[it.key()] = getAverageResponseTime(it.key());
    }
}

void MonitorManager::checkThresholds()
{
    checkErrorThresholds();
    checkPerformanceThresholds();
}

void MonitorManager::calculateCpuUsage()
{
    // 简化的CPU使用率计算
    // 在实际应用中，可能需要更复杂的计算
    static qint64 lastCpuTime = 0;
    static QDateTime lastTime = QDateTime::currentDateTime();
    
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 currentCpuTime = QThread::currentThreadId();
    
    if (lastCpuTime > 0) {
        qint64 timeDiff = lastTime.msecsTo(currentTime);
        if (timeDiff > 0) {
            _cpuUsage = (currentCpuTime - lastCpuTime) * 100.0 / timeDiff;
            _cpuUsage = qBound(0.0, _cpuUsage, 100.0);
        }
    }
    
    lastCpuTime = currentCpuTime;
    lastTime = currentTime;
}

void MonitorManager::calculateMemoryUsage()
{
    _memoryUsage = _systemInfo.totalMemory - _availableMemory;
}

void MonitorManager::checkErrorThresholds()
{
    QMutexLocker locker(&_dataMutex);
    
    for (auto it = _errorCounts.begin(); it != _errorCounts.end(); ++it) {
        if (it.value() >= ERROR_THRESHOLD) {
            emit errorThresholdExceeded(it.key(), it.value());
            LogManager::instance()->writeErrorLog(
                QString("Error threshold exceeded for %1: %2 errors").arg(it.key()).arg(it.value()),
                "MonitorManager"
            );
        }
    }
}

void MonitorManager::checkPerformanceThresholds()
{
    QMutexLocker locker(&_dataMutex);
    
    for (auto it = _responseTimes.begin(); it != _responseTimes.end(); ++it) {
        qint64 avgTime = getAverageResponseTime(it.key());
        if (avgTime > PERFORMANCE_THRESHOLD) {
            emit performanceAlert(it.key(), avgTime);
            LogManager::instance()->writePerformanceLog(
                QString("Performance alert for %1").arg(it.key()),
                avgTime,
                "ms"
            );
        }
    }
}

void MonitorManager::saveMetrics()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString metricsDir = QDir(appDir).absoluteFilePath("../../../../logs/client");
    QDir dir(metricsDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString metricsFile = dir.absoluteFilePath("monitoring_metrics.json");
    QFile file(metricsFile);
    
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject metrics;
        metrics["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        metrics["system_info"] = QJsonObject{
            {"os_name", _systemInfo.osName},
            {"os_version", _systemInfo.osVersion},
            {"cpu_architecture", _systemInfo.cpuArchitecture},
            {"cpu_count", _systemInfo.cpuCount},
            {"total_memory", _systemInfo.totalMemory},
            {"available_memory", _systemInfo.availableMemory}
        };
        
        QJsonObject currentMetrics;
        QHash<QString, double> metricsData = getMetrics();
        for (auto it = metricsData.begin(); it != metricsData.end(); ++it) {
            currentMetrics[it.key()] = it.value();
        }
        metrics["current_metrics"] = currentMetrics;
        
        QJsonObject eventCounts;
        QHash<QString, int> events = getEventCounts();
        for (auto it = events.begin(); it != events.end(); ++it) {
            eventCounts[it.key()] = it.value();
        }
        metrics["event_counts"] = eventCounts;
        
        QJsonObject errorCounts;
        QHash<QString, int> errors = getErrorCounts();
        for (auto it = errors.begin(); it != errors.end(); ++it) {
            errorCounts[it.key()] = it.value();
        }
        metrics["error_counts"] = errorCounts;
        
        QJsonDocument doc(metrics);
        file.write(doc.toJson());
    }
} 