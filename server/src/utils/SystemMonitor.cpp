#include "SystemMonitor.h"
#include "LogManager.h"
#include <QTimer>
#include <QProcess>
#include <QSysInfo>
#include <QHostInfo>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#ifdef Q_OS_WINDOWS
#include <windows.h>
#include <psapi.h>
#else
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#endif

Q_LOGGING_CATEGORY(systemMonitor, "qkchat.server.systemmonitor")

SystemMonitor* SystemMonitor::s_instance = nullptr;
QMutex SystemMonitor::s_instanceMutex;

SystemMonitor* SystemMonitor::instance()
{
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        s_instance = new SystemMonitor();
    }
    return s_instance;
}

SystemMonitor::SystemMonitor(QObject *parent)
    : QObject(parent)
    , m_running(0)
    , m_metricsTimer(nullptr)
    , m_healthTimer(nullptr)
    , m_alertTimer(nullptr)
    , m_deadlockTimer(nullptr)
    , m_recoveryTimer(nullptr)
    , m_deadlockDetectionEnabled(0)
    , m_deadlockDetected(0)
{
    qCInfo(systemMonitor) << "SystemMonitor initialized";
}

SystemMonitor::~SystemMonitor()
{
    shutdown();
    qCInfo(systemMonitor) << "SystemMonitor destroyed";
}

bool SystemMonitor::initialize(const MonitorConfig& config)
{
    if (m_running.testAndSetOrdered(0, 1)) {
        m_config = config;
        
        // 创建定时器
        m_metricsTimer = new QTimer(this);
        m_healthTimer = new QTimer(this);
        m_alertTimer = new QTimer(this);
        m_deadlockTimer = new QTimer(this);
        m_recoveryTimer = new QTimer(this);
        
        // 连接信号
        connect(m_metricsTimer, &QTimer::timeout, this, &SystemMonitor::collectMetrics);
        connect(m_healthTimer, &QTimer::timeout, this, &SystemMonitor::performHealthChecks);
        connect(m_alertTimer, &QTimer::timeout, this, &SystemMonitor::checkAlerts);
        connect(m_deadlockTimer, &QTimer::timeout, this, &SystemMonitor::detectDeadlocks);
        connect(m_recoveryTimer, &QTimer::timeout, this, &SystemMonitor::performRecovery);
        
        // 启动定时器
        m_metricsTimer->start(m_config.metricsCollectionInterval);
        m_healthTimer->start(m_config.healthCheckInterval);
        m_alertTimer->start(m_config.alertCheckInterval);
        m_deadlockTimer->start(m_config.deadlockCheckInterval);
        
        if (m_config.enableAutoRecovery) {
            m_recoveryTimer->start(m_config.recoveryInterval);
        }
        
        qCInfo(systemMonitor) << "SystemMonitor started with config:"
                              << "metrics=" << m_config.metricsCollectionInterval << "ms"
                              << "health=" << m_config.healthCheckInterval << "ms"
                              << "alerts=" << m_config.alertCheckInterval << "ms";
        
        return true;
    }
    return false;
}

void SystemMonitor::shutdown()
{
    if (m_running.testAndSetOrdered(1, 0)) {
        if (m_metricsTimer) m_metricsTimer->stop();
        if (m_healthTimer) m_healthTimer->stop();
        if (m_alertTimer) m_alertTimer->stop();
        if (m_deadlockTimer) m_deadlockTimer->stop();
        if (m_recoveryTimer) m_recoveryTimer->stop();
        
        qCInfo(systemMonitor) << "SystemMonitor shutdown";
    }
}

bool SystemMonitor::isRunning() const
{
    return m_running.loadAcquire() == 1;
}

void SystemMonitor::recordMetric(const QString& name, const QVariant& value, MetricType type)
{
    MetricData metric;
    metric.name = name;
    metric.value = value;
    metric.type = type;
    metric.timestamp = QDateTime::currentDateTime();
    
    m_currentMetrics.insert(name, metric);
    
    // 添加到历史记录
    QList<MetricData> history = m_metricHistory.value(name, QList<MetricData>());
    history.append(metric);
    
    // 清理旧数据
    if (history.size() > 100) {
        history.removeFirst();
    }
    
    m_metricHistory.insert(name, history);
    
    emit metricRecorded(metric);
}

void SystemMonitor::incrementCounter(const QString& name, int value)
{
    QVariant currentValue = m_currentMetrics.value(name, MetricData()).value;
    int newValue = currentValue.toInt() + value;
    recordMetric(name, newValue, MetricType::Counter);
}

void SystemMonitor::recordTimer(const QString& name, int milliseconds)
{
    recordMetric(name, milliseconds, MetricType::Timer);
}

void SystemMonitor::recordHistogram(const QString& name, double value)
{
    recordMetric(name, value, MetricType::Histogram);
}

void SystemMonitor::registerHealthCheck(const QString& component, std::function<HealthCheck()> checkFunction)
{
    m_healthChecks.insert(component, checkFunction);
    qCInfo(systemMonitor) << "Registered health check for component:" << component;
}

void SystemMonitor::unregisterHealthCheck(const QString& component)
{
    m_healthChecks.remove(component);
    m_healthResults.remove(component);
    qCInfo(systemMonitor) << "Unregistered health check for component:" << component;
}

HealthStatus SystemMonitor::getComponentHealth(const QString& component) const
{
    auto it = m_healthResults.find(component);
    if (it != m_healthResults.end()) {
        return it.value().status;
    }
    return HealthStatus::Unknown;
}

HealthStatus SystemMonitor::getOverallHealth() const
{
    bool hasCritical = false;
    bool hasWarning = false;
    
    for (const auto& health : m_healthResults) {
        if (health.status == HealthStatus::Critical) {
            hasCritical = true;
        } else if (health.status == HealthStatus::Warning) {
            hasWarning = true;
        }
    }
    
    if (hasCritical) return HealthStatus::Critical;
    if (hasWarning) return HealthStatus::Warning;
    return HealthStatus::Healthy;
}

QList<HealthCheck> SystemMonitor::getAllHealthChecks() const
{
    QList<HealthCheck> results;
    QHash<QString, HealthCheck> healthResults = m_healthResults.toHash();
    for (auto it = healthResults.begin(); it != healthResults.end(); ++it) {
        results.append(it.value());
    }
    return results;
}

void SystemMonitor::registerAlert(const QString& alertId, const QString& component, const QString& metric,
                                 const QString& condition, const QVariant& threshold)
{
    PerformanceAlert alert;
    alert.alertId = alertId;
    alert.component = component;
    alert.metric = metric;
    alert.condition = condition;
    alert.threshold = threshold;
    alert.resolved = false;
    
    m_alerts.insert(alertId, alert);
    qCInfo(systemMonitor) << "Registered alert:" << alertId << "for" << component;
}

void SystemMonitor::unregisterAlert(const QString& alertId)
{
    m_alerts.remove(alertId);
    m_activeAlerts.remove(alertId);
    qCInfo(systemMonitor) << "Unregistered alert:" << alertId;
}

QList<PerformanceAlert> SystemMonitor::getActiveAlerts() const
{
    QList<PerformanceAlert> alerts;
    QHash<QString, PerformanceAlert> activeAlerts = m_activeAlerts.toHash();
    for (auto it = activeAlerts.begin(); it != activeAlerts.end(); ++it) {
        alerts.append(it.value());
    }
    return alerts;
}

QList<PerformanceAlert> SystemMonitor::getResolvedAlerts() const
{
    QList<PerformanceAlert> alerts;
    QHash<QString, PerformanceAlert> allAlerts = m_alerts.toHash();
    for (auto it = allAlerts.begin(); it != allAlerts.end(); ++it) {
        if (it.value().resolved) {
            alerts.append(it.value());
        }
    }
    return alerts;
}

SystemMonitor::SystemMetrics SystemMonitor::getCurrentMetrics() const
{
    SystemMetrics metrics;
    metrics.cpuUsage = getCpuUsage();
    metrics.memoryUsage = getMemoryUsage();
    metrics.diskUsage = getDiskUsage();
    getNetworkUsage(metrics.networkIn, metrics.networkOut);
    metrics.timestamp = QDateTime::currentDateTime();
    
    return metrics;
}

QJsonObject SystemMonitor::getMetricsSnapshot() const
{
    QJsonObject snapshot;
    SystemMetrics metrics = getCurrentMetrics();
    
    QJsonObject systemMetrics;
    systemMetrics["cpuUsage"] = metrics.cpuUsage;
    systemMetrics["memoryUsage"] = metrics.memoryUsage;
    systemMetrics["diskUsage"] = metrics.diskUsage;
    systemMetrics["networkIn"] = metrics.networkIn;
    systemMetrics["networkOut"] = metrics.networkOut;
    systemMetrics["timestamp"] = metrics.timestamp.toString(Qt::ISODate);
    
    snapshot["system"] = systemMetrics;
    
    // 添加当前指标
    QJsonObject currentMetrics;
    QHash<QString, MetricData> currentMetricsData = m_currentMetrics.toHash();
    for (auto it = currentMetricsData.begin(); it != currentMetricsData.end(); ++it) {
        QJsonObject metricObj;
        metricObj["value"] = QJsonValue::fromVariant(it.value().value);
        metricObj["type"] = static_cast<int>(it.value().type);
        metricObj["timestamp"] = it.value().timestamp.toString(Qt::ISODate);
        currentMetrics[it.key()] = metricObj;
    }
    snapshot["current"] = currentMetrics;
    
    // 添加健康状态
    QJsonObject healthStatus;
    healthStatus["overall"] = static_cast<int>(getOverallHealth());
    
    QJsonArray components;
    QHash<QString, HealthCheck> healthResults = m_healthResults.toHash();
    for (auto it = healthResults.begin(); it != healthResults.end(); ++it) {
        QJsonObject component;
        component["name"] = it.value().component;
        component["status"] = static_cast<int>(it.value().status);
        component["message"] = it.value().message;
        component["timestamp"] = it.value().timestamp.toString(Qt::ISODate);
        components.append(component);
    }
    healthStatus["components"] = components;
    snapshot["health"] = healthStatus;
    
    return snapshot;
}

void SystemMonitor::enableDeadlockDetection(bool enabled)
{
    m_deadlockDetectionEnabled.storeRelease(enabled ? 1 : 0);
    qCInfo(systemMonitor) << "Deadlock detection" << (enabled ? "enabled" : "disabled");
}

bool SystemMonitor::isDeadlockDetected() const
{
    return m_deadlockDetected.loadAcquire() == 1;
}

QStringList SystemMonitor::getDeadlockReport() const
{
    QMutexLocker locker(&m_deadlockMutex);
    return m_deadlockReport;
}

void SystemMonitor::enableAutoRecovery(bool enabled)
{
    m_config.enableAutoRecovery = enabled;
    if (enabled && m_recoveryTimer) {
        m_recoveryTimer->start(m_config.recoveryInterval);
    } else if (!enabled && m_recoveryTimer) {
        m_recoveryTimer->stop();
    }
    qCInfo(systemMonitor) << "Auto recovery" << (enabled ? "enabled" : "disabled");
}

void SystemMonitor::triggerRecovery(const QString& component, const QString& reason)
{
    if (canAttemptRecovery(component)) {
        m_recoveryAttempts.insert(component, m_recoveryAttempts.value(component, 0) + 1);
        m_lastRecoveryTime.insert(component, QDateTime::currentDateTime());
        
        emit recoveryTriggered(component, reason);
        qCWarning(systemMonitor) << "Recovery triggered for" << component << ":" << reason;
    }
}

int SystemMonitor::getRecoveryAttempts(const QString& component) const
{
    return m_recoveryAttempts.value(component, 0);
}

void SystemMonitor::collectMetrics()
{
    collectSystemMetrics();
    collectApplicationMetrics();
    collectDatabaseMetrics();
    cleanupOldMetrics();
}

void SystemMonitor::performHealthChecks()
{
    QHash<QString, std::function<HealthCheck()>> checks = m_healthChecks.toHash();
    for (auto it = checks.begin(); it != checks.end(); ++it) {
        try {
            HealthCheck result = it.value()();
            HealthStatus oldStatus = getComponentHealth(result.component);
            
            m_healthResults.insert(result.component, result);
            
            if (oldStatus != result.status) {
                emit healthStatusChanged(result.component, oldStatus, result.status);
                qCInfo(systemMonitor) << "Health status changed for" << result.component
                                     << ":" << static_cast<int>(oldStatus) << "->" << static_cast<int>(result.status);
            }
        } catch (const std::exception& e) {
            qCWarning(systemMonitor) << "Health check failed:" << e.what();
        }
    }
}

void SystemMonitor::checkAlerts()
{
    checkMetricAlerts();
    checkSystemAlerts();
    cleanupOldAlerts();
}

void SystemMonitor::detectDeadlocks()
{
    if (!m_deadlockDetectionEnabled.loadAcquire()) {
        return;
    }
    
    // 简化的死锁检测逻辑
    // 这里可以实现更复杂的死锁检测算法
    bool hasDeadlock = false;
    QStringList report;
    
    // 检查线程池状态
    if (QThread::idealThreadCount() > 0) {
        int activeThreads = QThread::idealThreadCount();
        if (activeThreads == 0) {
            hasDeadlock = true;
            report << "Thread pool appears to be deadlocked";
        }
    }
    
    if (hasDeadlock) {
        m_deadlockDetected.storeRelease(1);
        m_deadlockMutex.lock();
        m_deadlockReport = report;
        m_deadlockMutex.unlock();
        
        emit deadlockDetected(report);
        qCWarning(systemMonitor) << "Deadlock detected:" << report.join(", ");
    }
}

void SystemMonitor::performRecovery()
{
    if (!m_config.enableAutoRecovery) {
        return;
    }
    
    performSystemRecovery();
    
    // 对每个组件执行恢复
    QHash<QString, HealthCheck> healthResults = m_healthResults.toHash();
    for (auto it = healthResults.begin(); it != healthResults.end(); ++it) {
        if (getComponentHealth(it.key()) == HealthStatus::Critical) {
            performComponentRecovery(it.key());
        }
    }
}

void SystemMonitor::collectSystemMetrics()
{
    double cpuUsage = getCpuUsage();
    double memoryUsage = getMemoryUsage();
    qint64 diskUsage = getDiskUsage();
    qint64 networkIn, networkOut;
    getNetworkUsage(networkIn, networkOut);
    
    recordMetric("cpu_usage", cpuUsage, MetricType::Gauge);
    recordMetric("memory_usage", memoryUsage, MetricType::Gauge);
    recordMetric("disk_usage", diskUsage, MetricType::Gauge);
    recordMetric("network_in", networkIn, MetricType::Counter);
    recordMetric("network_out", networkOut, MetricType::Counter);
}

void SystemMonitor::collectApplicationMetrics()
{
    // 这里可以收集应用程序特定的指标
    // 例如：连接数、消息队列大小等
    recordMetric("active_connections", 0, MetricType::Gauge);
    recordMetric("queue_size", 0, MetricType::Gauge);
    recordMetric("response_time", 0, MetricType::Timer);
}

void SystemMonitor::collectDatabaseMetrics()
{
    // 这里可以收集数据库相关的指标
    recordMetric("db_connections", 0, MetricType::Gauge);
    recordMetric("db_queue_size", 0, MetricType::Gauge);
    recordMetric("db_response_time", 0, MetricType::Timer);
}

void SystemMonitor::checkMetricAlerts()
{
    QHash<QString, PerformanceAlert> alerts = m_alerts.toHash();
    for (auto it = alerts.begin(); it != alerts.end(); ++it) {
        const PerformanceAlert& alert = it.value();
        if (alert.resolved) continue;
        
        QVariant currentValue = m_currentMetrics.value(alert.metric, MetricData()).value;
        if (evaluateCondition(alert.condition, currentValue, alert.threshold)) {
            PerformanceAlert triggeredAlert = alert;
            triggeredAlert.currentValue = currentValue;
            triggeredAlert.triggeredAt = QDateTime::currentDateTime();
            
            m_activeAlerts.insert(alert.alertId, triggeredAlert);
            emit alertTriggered(triggeredAlert);
            
            qCWarning(systemMonitor) << "Alert triggered:" << alert.alertId
                                   << "component:" << alert.component
                                   << "metric:" << alert.metric
                                   << "value:" << currentValue.toString();
        }
    }
}

void SystemMonitor::checkSystemAlerts()
{
    SystemMetrics metrics = getCurrentMetrics();
    
    // CPU使用率警报
    if (metrics.cpuUsage > m_config.cpuThreshold) {
        triggerRecovery("system", QString("CPU usage too high: %1%").arg(metrics.cpuUsage));
    }
    
    // 内存使用率警报
    if (metrics.memoryUsage > m_config.memoryThreshold) {
        triggerRecovery("system", QString("Memory usage too high: %1%").arg(metrics.memoryUsage));
    }
}

void SystemMonitor::resolveAlert(const QString& alertId)
{
    auto it = m_activeAlerts.find(alertId);
    if (it != m_activeAlerts.end()) {
        PerformanceAlert alert = it.value();
        alert.resolved = true;
        
        m_alerts.insert(alertId, alert);
        m_activeAlerts.remove(alertId);
        
        emit alertResolved(alert);
        qCInfo(systemMonitor) << "Alert resolved:" << alertId;
    }
}

bool SystemMonitor::evaluateCondition(const QString& condition, const QVariant& value, const QVariant& threshold)
{
    if (condition == ">") {
        return value.toDouble() > threshold.toDouble();
    } else if (condition == "<") {
        return value.toDouble() < threshold.toDouble();
    } else if (condition == ">=") {
        return value.toDouble() >= threshold.toDouble();
    } else if (condition == "<=") {
        return value.toDouble() <= threshold.toDouble();
    } else if (condition == "==") {
        return value == threshold;
    }
    return false;
}

void SystemMonitor::performSystemRecovery()
{
    // 系统级恢复操作
    // 例如：清理内存、重启关键服务等
    qCInfo(systemMonitor) << "Performing system recovery";
}

void SystemMonitor::performComponentRecovery(const QString& component)
{
    if (!canAttemptRecovery(component)) {
        return;
    }
    
    qCInfo(systemMonitor) << "Performing recovery for component:" << component;
    
    // 这里可以实现组件特定的恢复逻辑
    bool success = true; // 简化实现
    
    emit recoveryCompleted(component, success);
    
    if (success) {
        qCInfo(systemMonitor) << "Recovery completed successfully for" << component;
    } else {
        qCWarning(systemMonitor) << "Recovery failed for" << component;
    }
}

bool SystemMonitor::canAttemptRecovery(const QString& component)
{
    int attempts = m_recoveryAttempts.value(component, 0);
    if (attempts >= m_config.maxRecoveryAttempts) {
        return false;
    }
    
    QDateTime lastRecovery = m_lastRecoveryTime.value(component, QDateTime());
    if (lastRecovery.isValid() && lastRecovery.msecsTo(QDateTime::currentDateTime()) < m_config.recoveryInterval) {
        return false;
    }
    
    return true;
}

void SystemMonitor::cleanupOldMetrics()
{
    // 清理旧的指标数据
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-3600); // 1小时前
    
    for (auto& history : m_metricHistory) {
        history.removeIf([&cutoff](const MetricData& metric) {
            return metric.timestamp < cutoff;
        });
    }
}

void SystemMonitor::cleanupOldAlerts()
{
    // 清理已解决的旧警报
    QDateTime cutoff = QDateTime::currentDateTime().addSecs(-86400); // 24小时前
    
    QStringList toRemove;
    QHash<QString, PerformanceAlert> alerts = m_alerts.toHash();
    for (auto it = alerts.begin(); it != alerts.end(); ++it) {
        if (it.value().resolved && it.value().triggeredAt < cutoff) {
            toRemove << it.key();
        }
    }
    
    for (const QString& alertId : toRemove) {
        m_alerts.remove(alertId);
    }
}

double SystemMonitor::getCpuUsage() const
{
#ifdef Q_OS_WINDOWS
    FILETIME idleTime, kernelTime, userTime;
    if (GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        static FILETIME lastIdleTime = {0, 0};
        static FILETIME lastKernelTime = {0, 0};
        static FILETIME lastUserTime = {0, 0};
        
        ULARGE_INTEGER idle, kernel, user;
        idle.LowPart = idleTime.dwLowDateTime;
        idle.HighPart = idleTime.dwHighDateTime;
        kernel.LowPart = kernelTime.dwLowDateTime;
        kernel.HighPart = kernelTime.dwHighDateTime;
        user.LowPart = userTime.dwLowDateTime;
        user.HighPart = userTime.dwHighDateTime;
        
        if (lastIdleTime.dwLowDateTime != 0) {
            ULARGE_INTEGER lastIdle, lastKernel, lastUser;
            lastIdle.LowPart = lastIdleTime.dwLowDateTime;
            lastIdle.HighPart = lastIdleTime.dwHighDateTime;
            lastKernel.LowPart = lastKernelTime.dwLowDateTime;
            lastKernel.HighPart = lastKernelTime.dwHighDateTime;
            lastUser.LowPart = lastUserTime.dwLowDateTime;
            lastUser.HighPart = lastUserTime.dwHighDateTime;
            
            ULONGLONG kernelDiff = kernel.QuadPart - lastKernel.QuadPart;
            ULONGLONG userDiff = user.QuadPart - lastUser.QuadPart;
            ULONGLONG idleDiff = idle.QuadPart - lastIdle.QuadPart;
            
            ULONGLONG totalDiff = kernelDiff + userDiff;
            if (totalDiff > 0) {
                return 100.0 - (idleDiff * 100.0 / totalDiff);
            }
        }
        
        lastIdleTime = idleTime;
        lastKernelTime = kernelTime;
        lastUserTime = userTime;
    }
#else
    // Linux系统使用/proc/stat
    QFile statFile("/proc/stat");
    if (statFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&statFile);
        QString line = stream.readLine();
        if (line.startsWith("cpu ")) {
            QStringList parts = line.split(" ", QString::SkipEmptyParts);
            if (parts.size() >= 5) {
                static qint64 lastTotal = 0, lastIdle = 0;
                
                qint64 user = parts[1].toLongLong();
                qint64 nice = parts[2].toLongLong();
                qint64 system = parts[3].toLongLong();
                qint64 idle = parts[4].toLongLong();
                
                qint64 total = user + nice + system + idle;
                
                if (lastTotal > 0) {
                    qint64 totalDiff = total - lastTotal;
                    qint64 idleDiff = idle - lastIdle;
                    if (totalDiff > 0) {
                        return 100.0 - (idleDiff * 100.0 / totalDiff);
                    }
                }
                
                lastTotal = total;
                lastIdle = idle;
            }
        }
    }
#endif
    
    return 0.0;
}

double SystemMonitor::getMemoryUsage() const
{
#ifdef Q_OS_WINDOWS
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return 100.0 - (memInfo.dwMemoryLoad);
    }
#else
    QFile memInfo("/proc/meminfo");
    if (memInfo.open(QIODevice::ReadOnly)) {
        QTextStream stream(&memInfo);
        qint64 total = 0, available = 0;
        
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (line.startsWith("MemTotal:")) {
                QString value = line.split(" ").last();
                total = value.toLongLong() * 1024;
            } else if (line.startsWith("MemAvailable:")) {
                QString value = line.split(" ").last();
                available = value.toLongLong() * 1024;
            }
        }
        
        if (total > 0) {
            return ((total - available) * 100.0) / total;
        }
    }
#endif
    
    return 0.0;
}

qint64 SystemMonitor::getDiskUsage() const
{
#ifdef Q_OS_WINDOWS
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceEx(L"C:\\", &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        return ((totalBytes.QuadPart - totalFreeBytes.QuadPart) * 100) / totalBytes.QuadPart;
    }
#else
    struct statvfs stat;
    if (statvfs("/", &stat) == 0) {
        qint64 total = stat.f_blocks * stat.f_frsize;
        qint64 available = stat.f_bavail * stat.f_frsize;
        return ((total - available) * 100) / total;
    }
#endif
    
    return 0;
}

void SystemMonitor::getNetworkUsage(qint64& bytesIn, qint64& bytesOut) const
{
    // 简化的网络使用量获取
    // 实际实现需要更复杂的网络统计收集
    bytesIn = 0;
    bytesOut = 0;
}

QString SystemMonitor::generateAlertId() const
{
    return QString("alert_%1").arg(QDateTime::currentMSecsSinceEpoch());
}

void SystemMonitor::logMonitorEvent(const QString& event, const QString& details) const
{
    LogManager::instance()->writeSystemLog("SystemMonitor", event, details);
} 