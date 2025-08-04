#include "RobustnessManager.h"
#include <QDebug>
#include <QCoreApplication>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRandomGenerator>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QHostInfo>
#include <algorithm>

Q_LOGGING_CATEGORY(robustness, "qkchat.server.robustness")

// ============================================================================
// CircuitBreakerManager 实现
// ============================================================================

CircuitBreakerManager::CircuitBreakerManager(QObject *parent)
    : QObject(parent)
{
    qCInfo(robustness) << "CircuitBreakerManager initialized";
}

void CircuitBreakerManager::registerCircuit(const QString& circuitName, const CircuitConfig& config)
{
    QMutexLocker locker(&m_mutex);
    
    m_configs[circuitName] = config;
    m_stats[circuitName] = CircuitStats();
    
    qCInfo(robustness) << "Circuit registered:" << circuitName 
                       << "threshold:" << config.failureThreshold;
}

bool CircuitBreakerManager::canExecute(const QString& circuitName)
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_stats.find(circuitName);
    if (it == m_stats.end()) {
        return true; // 未注册的熔断器默认允许执行
    }
    
    CircuitStats& stats = it.value();
    
    // 如果是熔断状态，检查是否可以进入半开状态
    if (stats.state == CircuitBreakerState::Open) {
        auto config = m_configs[circuitName];
        QDateTime now = QDateTime::currentDateTime();
        
        if (stats.lastFailureTime.msecsTo(now) >= config.timeout.count()) {
            stats.state = CircuitBreakerState::HalfOpen;
            emit circuitHalfOpened(circuitName);
            qCInfo(robustness) << "Circuit moved to half-open:" << circuitName;
        } else {
            return false; // 仍在熔断状态
        }
    }
    
    stats.totalRequests.fetchAndAddOrdered(1);
    return true;
}

void CircuitBreakerManager::recordSuccess(const QString& circuitName)
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_stats.find(circuitName);
    if (it == m_stats.end()) {
        return;
    }
    
    CircuitStats& stats = it.value();
    stats.successfulRequests.fetchAndAddOrdered(1);
    stats.consecutiveFailures.storeRelease(0);
    stats.lastSuccessTime = QDateTime::currentDateTime();
    
    if (stats.state == CircuitBreakerState::HalfOpen) {
        stats.consecutiveSuccesses.fetchAndAddOrdered(1);
        
        auto config = m_configs[circuitName];
        if (stats.consecutiveSuccesses.loadAcquire() >= config.successThreshold) {
            stats.state = CircuitBreakerState::Closed;
            stats.consecutiveSuccesses.storeRelease(0);
            emit circuitClosed(circuitName);
            qCInfo(robustness) << "Circuit closed:" << circuitName;
        }
    }
}

void CircuitBreakerManager::recordFailure(const QString& circuitName)
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_stats.find(circuitName);
    if (it == m_stats.end()) {
        return;
    }
    
    CircuitStats& stats = it.value();
    stats.failedRequests.fetchAndAddOrdered(1);
    stats.lastFailureTime = QDateTime::currentDateTime();
    
    updateCircuitState(circuitName);
}

void CircuitBreakerManager::updateCircuitState(const QString& circuitName)
{
    CircuitStats& stats = m_stats[circuitName];
    auto config = m_configs[circuitName];
    
    if (stats.state == CircuitBreakerState::Closed) {
        int failures = stats.consecutiveFailures.fetchAndAddOrdered(1) + 1;
        
        if (failures >= config.failureThreshold) {
            stats.state = CircuitBreakerState::Open;
            stats.consecutiveSuccesses.storeRelease(0);
            emit circuitOpened(circuitName);
            qCWarning(robustness) << "Circuit opened:" << circuitName 
                                  << "failures:" << failures;
        }
    } else if (stats.state == CircuitBreakerState::HalfOpen) {
        stats.state = CircuitBreakerState::Open;
        stats.consecutiveSuccesses.storeRelease(0);
        emit circuitOpened(circuitName);
        qCWarning(robustness) << "Circuit re-opened:" << circuitName;
    }
}

CircuitBreakerState CircuitBreakerManager::getState(const QString& circuitName) const
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_stats.find(circuitName);
    if (it == m_stats.end()) {
        return CircuitBreakerState::Closed;
    }
    
    return it.value().state;
}

CircuitBreakerManager::CircuitStats CircuitBreakerManager::getStats(const QString& circuitName) const
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_stats.find(circuitName);
    if (it == m_stats.end()) {
        return CircuitStats();
    }
    
    return it.value();
}

// ============================================================================
// MemoryMonitor 实现
// ============================================================================

MemoryMonitor::MemoryMonitor(QObject *parent)
    : QObject(parent)
    , m_monitorTimer(new QTimer(this))
{
    connect(m_monitorTimer, &QTimer::timeout, this, &MemoryMonitor::checkMemoryUsage);
    qCInfo(robustness) << "MemoryMonitor initialized";
}

void MemoryMonitor::setThresholds(const MemoryThresholds& thresholds)
{
    m_thresholds = thresholds;
    qCInfo(robustness) << "Memory thresholds updated - warning:" << thresholds.warningThreshold
                       << "critical:" << thresholds.criticalThreshold
                       << "emergency:" << thresholds.emergencyThreshold;
}

MemoryMonitor::MemoryStats MemoryMonitor::getCurrentStats() const
{
    return collectMemoryStats();
}

void MemoryMonitor::startMonitoring(int intervalMs)
{
    m_monitorTimer->start(intervalMs);
    qCInfo(robustness) << "Memory monitoring started with interval:" << intervalMs << "ms";
}

void MemoryMonitor::stopMonitoring()
{
    m_monitorTimer->stop();
    qCInfo(robustness) << "Memory monitoring stopped";
}

void MemoryMonitor::registerCleanupHandler(const QString& name, std::function<bool(int)> handler)
{
    m_cleanupHandlers[name] = handler;
    qCInfo(robustness) << "Cleanup handler registered:" << name;
}

void MemoryMonitor::triggerCleanup(int level)
{
    qCInfo(robustness) << "Triggering memory cleanup, level:" << level;
    executeCleanupHandlers(level);
}

void MemoryMonitor::checkMemoryUsage()
{
    MemoryStats currentStats = collectMemoryStats();
    m_lastStats = currentStats;
    
    double usagePercent = currentStats.memoryUsagePercent;
    
    if (usagePercent >= m_thresholds.emergencyThreshold) {
        emit memoryEmergency(usagePercent);
        triggerCleanup(3); // 重度清理
    } else if (usagePercent >= m_thresholds.criticalThreshold) {
        emit memoryCritical(usagePercent);
        triggerCleanup(2); // 中度清理
    } else if (usagePercent >= m_thresholds.warningThreshold) {
        emit memoryWarning(usagePercent);
        triggerCleanup(1); // 轻度清理
    }
}

MemoryMonitor::MemoryStats MemoryMonitor::collectMemoryStats() const
{
    MemoryStats stats;
    stats.timestamp = QDateTime::currentDateTime();
    
#ifdef Q_OS_LINUX
    // 读取 /proc/meminfo
    QFile memFile("/proc/meminfo");
    if (memFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&memFile);
        QString line;
        
        while (stream.readLineInto(&line)) {
            if (line.startsWith("MemTotal:")) {
                stats.totalMemory = line.split(QRegExp("\\s+"))[1].toLongLong() * 1024;
            } else if (line.startsWith("MemAvailable:")) {
                stats.freeMemory = line.split(QRegExp("\\s+"))[1].toLongLong() * 1024;
            }
        }
        
        stats.usedMemory = stats.totalMemory - stats.freeMemory;
        stats.memoryUsagePercent = static_cast<double>(stats.usedMemory) / stats.totalMemory;
    }
    
    // 读取当前进程内存使用
    QFile statusFile(QString("/proc/%1/status").arg(QCoreApplication::applicationPid()));
    if (statusFile.open(QIODevice::ReadOnly)) {
        QTextStream stream(&statusFile);
        QString line;
        
        while (stream.readLineInto(&line)) {
            if (line.startsWith("VmRSS:")) {
                stats.processMemory = line.split(QRegExp("\\s+"))[1].toLongLong() * 1024;
                break;
            }
        }
    }
#else
    // 其他平台的内存获取逻辑
    stats.totalMemory = 8LL * 1024 * 1024 * 1024; // 默认8GB
    stats.usedMemory = stats.totalMemory * 0.5;    // 假设使用50%
    stats.freeMemory = stats.totalMemory - stats.usedMemory;
    stats.processMemory = 100 * 1024 * 1024;       // 假设进程使用100MB
    stats.memoryUsagePercent = 0.5;
#endif
    
    return stats;
}

void MemoryMonitor::executeCleanupHandlers(int level)
{
    for (auto it = m_cleanupHandlers.begin(); it != m_cleanupHandlers.end(); ++it) {
        try {
            bool success = it.value()(level);
            emit cleanupCompleted(it.key(), success);
            
            if (success) {
                qCInfo(robustness) << "Cleanup handler executed successfully:" 
                                   << it.key() << "level:" << level;
            } else {
                qCWarning(robustness) << "Cleanup handler failed:" 
                                      << it.key() << "level:" << level;
            }
        } catch (const std::exception& e) {
            qCCritical(robustness) << "Cleanup handler exception:" 
                                   << it.key() << e.what();
        }
    }
}

// ============================================================================
// ThreadStarvationDetector 实现
// ============================================================================

ThreadStarvationDetector::ThreadStarvationDetector(QObject *parent)
    : QObject(parent)
    , m_checkTimer(new QTimer(this))
{
    connect(m_checkTimer, &QTimer::timeout, this, &ThreadStarvationDetector::checkThreadStarvation);
    m_checkTimer->start(10000); // 每10秒检查一次
    
    qCInfo(robustness) << "ThreadStarvationDetector initialized";
}

void ThreadStarvationDetector::registerThread(QThread* thread, const QString& name)
{
    QMutexLocker locker(&m_mutex);
    
    ThreadInfo info;
    info.thread = thread;
    info.threadName = name;
    info.lastActivity = QDateTime::currentDateTime();
    
    m_threadInfo[thread] = info;
    
    qCInfo(robustness) << "Thread registered for starvation detection:" << name;
}

void ThreadStarvationDetector::recordActivity(QThread* thread)
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_threadInfo.find(thread);
    if (it != m_threadInfo.end()) {
        it.value().lastActivity = QDateTime::currentDateTime();
        
        if (it.value().isStarving) {
            it.value().isStarving = false;
            emit threadRecovered(it.value().threadName);
        }
    }
}

void ThreadStarvationDetector::recordTaskCompletion(QThread* thread)
{
    QMutexLocker locker(&m_mutex);
    
    auto it = m_threadInfo.find(thread);
    if (it != m_threadInfo.end()) {
        it.value().completedTasks.fetchAndAddOrdered(1);
        it.value().lastActivity = QDateTime::currentDateTime();
    }
}

QList<ThreadStarvationDetector::ThreadInfo> ThreadStarvationDetector::getStarvingThreads() const
{
    QMutexLocker locker(&m_mutex);
    
    QList<ThreadInfo> starvingThreads;
    for (const ThreadInfo& info : m_threadInfo) {
        if (info.isStarving) {
            starvingThreads.append(info);
        }
    }
    
    return starvingThreads;
}

void ThreadStarvationDetector::checkThreadStarvation()
{
    QMutexLocker locker(&m_mutex);
    
    QDateTime now = QDateTime::currentDateTime();
    
    for (auto it = m_threadInfo.begin(); it != m_threadInfo.end(); ++it) {
        ThreadInfo& info = it.value();
        
        int secondsSinceActivity = info.lastActivity.secsTo(now);
        
        if (secondsSinceActivity > m_starvationThreshold && !info.isStarving) {
            info.isStarving = true;
            emit threadStarvationDetected(info.threadName);
            
            qCWarning(robustness) << "Thread starvation detected:" << info.threadName
                                  << "idle for:" << secondsSinceActivity << "seconds";
        }
    }
}

// ============================================================================
// PerformanceDegradationManager 实现
// ============================================================================

PerformanceDegradationManager::PerformanceDegradationManager(QObject *parent)
    : QObject(parent)
{
    qCInfo(robustness) << "PerformanceDegradationManager initialized";
}

void PerformanceDegradationManager::setConfig(const DegradationConfig& config)
{
    m_config = config;
    qCInfo(robustness) << "Degradation config updated - CPU:" << config.cpuThreshold
                       << "Memory:" << config.memoryThreshold;
}

PerformanceDegradationManager::DegradationLevel PerformanceDegradationManager::getCurrentLevel() const
{
    return m_currentLevel.load();
}

void PerformanceDegradationManager::registerDegradationHandler(DegradationLevel level, std::function<void()> handler)
{
    m_handlers[level] = handler;
    qCInfo(robustness) << "Degradation handler registered for level:" << static_cast<int>(level);
}

void PerformanceDegradationManager::updateSystemMetrics(double cpuUsage, double memoryUsage, 
                                                       double diskIO, double networkIO, int avgResponseTime)
{
    m_cpuUsage.store(cpuUsage);
    m_memoryUsage.store(memoryUsage);
    m_diskIO.store(diskIO);
    m_networkIO.store(networkIO);
    m_avgResponseTime.store(avgResponseTime);
    
    DegradationLevel newLevel = calculateDegradationLevel();
    DegradationLevel currentLevel = m_currentLevel.exchange(newLevel);
    
    if (newLevel != currentLevel) {
        emit degradationLevelChanged(newLevel, currentLevel);
        applyDegradation(newLevel);
        
        if (newLevel == DegradationLevel::Normal && currentLevel != DegradationLevel::Normal) {
            emit performanceRecovered();
        }
        
        qCInfo(robustness) << "Performance degradation level changed from" 
                           << static_cast<int>(currentLevel) << "to" << static_cast<int>(newLevel);
    }
}

PerformanceDegradationManager::DegradationLevel PerformanceDegradationManager::calculateDegradationLevel() const
{
    double cpu = m_cpuUsage.load();
    double memory = m_memoryUsage.load();
    double disk = m_diskIO.load();
    double network = m_networkIO.load();
    int responseTime = m_avgResponseTime.load();
    
    // 计算综合压力指数
    double pressureIndex = 0.0;
    
    if (cpu > m_config.cpuThreshold) pressureIndex += (cpu - m_config.cpuThreshold) * 2.0;
    if (memory > m_config.memoryThreshold) pressureIndex += (memory - m_config.memoryThreshold) * 2.5;
    if (disk > m_config.diskIOThreshold) pressureIndex += (disk - m_config.diskIOThreshold) * 1.5;
    if (network > m_config.networkThreshold) pressureIndex += (network - m_config.networkThreshold) * 1.0;
    if (responseTime > m_config.responseTimeThreshold) {
        pressureIndex += static_cast<double>(responseTime - m_config.responseTimeThreshold) / 1000.0;
    }
    
    // 根据压力指数确定降级级别
    if (pressureIndex >= 1.5) return DegradationLevel::Emergency;
    if (pressureIndex >= 1.0) return DegradationLevel::Heavy;
    if (pressureIndex >= 0.5) return DegradationLevel::Moderate;
    if (pressureIndex >= 0.2) return DegradationLevel::Light;
    
    return DegradationLevel::Normal;
}

void PerformanceDegradationManager::applyDegradation(DegradationLevel level)
{
    auto it = m_handlers.find(level);
    if (it != m_handlers.end()) {
        try {
            it.value()();
            qCInfo(robustness) << "Applied degradation level:" << static_cast<int>(level);
        } catch (const std::exception& e) {
            qCCritical(robustness) << "Degradation handler exception:" << e.what();
        }
    }
}

// ============================================================================
// HotConfigManager 实现
// ============================================================================

HotConfigManager::HotConfigManager(QObject *parent)
    : QObject(parent)
    , m_watchTimer(new QTimer(this))
{
    connect(m_watchTimer, &QTimer::timeout, this, &HotConfigManager::checkConfigChanges);
    m_watchTimer->start(5000); // 每5秒检查一次
    
    qCInfo(robustness) << "HotConfigManager initialized";
}

void HotConfigManager::watchConfigFile(const QString& filePath, std::function<void(const QJsonObject&)> callback)
{
    ConfigWatcher watcher;
    watcher.filePath = filePath;
    watcher.callback = callback;
    
    QFileInfo fileInfo(filePath);
    if (fileInfo.exists()) {
        watcher.lastModified = fileInfo.lastModified();
    }
    
    m_watchers[filePath] = watcher;
    
    qCInfo(robustness) << "Config file added to watch list:" << filePath;
}

void HotConfigManager::unwatchConfigFile(const QString& filePath)
{
    m_watchers.remove(filePath);
    m_validators.remove(filePath);
    
    qCInfo(robustness) << "Config file removed from watch list:" << filePath;
}

void HotConfigManager::reloadAllConfigs()
{
    qCInfo(robustness) << "Reloading all config files";
    
    for (auto it = m_watchers.begin(); it != m_watchers.end(); ++it) {
        QJsonObject config = loadConfigFile(it.key());
        if (!config.isEmpty()) {
            if (validateConfig(it.key(), config)) {
                it.value().callback(config);
                emit configChanged(it.key(), config);
            }
        }
    }
}

void HotConfigManager::setConfigValidator(const QString& filePath, std::function<bool(const QJsonObject&)> validator)
{
    m_validators[filePath] = validator;
    qCInfo(robustness) << "Config validator set for:" << filePath;
}

void HotConfigManager::checkConfigChanges()
{
    for (auto it = m_watchers.begin(); it != m_watchers.end(); ++it) {
        const QString& filePath = it.key();
        ConfigWatcher& watcher = it.value();
        
        QFileInfo fileInfo(filePath);
        if (!fileInfo.exists()) {
            continue;
        }
        
        QDateTime lastModified = fileInfo.lastModified();
        if (lastModified > watcher.lastModified) {
            watcher.lastModified = lastModified;
            
            QJsonObject config = loadConfigFile(filePath);
            if (!config.isEmpty()) {
                if (validateConfig(filePath, config)) {
                    watcher.callback(config);
                    emit configChanged(filePath, config);
                    
                    qCInfo(robustness) << "Config file reloaded:" << filePath;
                } else {
                    emit configError(filePath, "Config validation failed");
                }
            } else {
                emit configError(filePath, "Failed to load config file");
            }
        }
    }
}

QJsonObject HotConfigManager::loadConfigFile(const QString& filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(robustness) << "Failed to open config file:" << filePath;
        return QJsonObject();
    }
    
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qCWarning(robustness) << "Failed to parse config file:" << filePath 
                              << "error:" << error.errorString();
        return QJsonObject();
    }
    
    return doc.object();
}

bool HotConfigManager::validateConfig(const QString& filePath, const QJsonObject& config) const
{
    auto it = m_validators.find(filePath);
    if (it != m_validators.end()) {
        try {
            return it.value()(config);
        } catch (const std::exception& e) {
            qCWarning(robustness) << "Config validation exception:" << filePath << e.what();
            return false;
        }
    }
    
    return true; // 没有验证器则默认通过
}

// ============================================================================
// RobustnessManager 实现
// ============================================================================

RobustnessManager::RobustnessManager(QObject *parent)
    : QObject(parent)
    , m_healthCheckTimer(new QTimer(this))
{
    initializeSubManagers();
    setupSignalConnections();
    
    connect(m_healthCheckTimer, &QTimer::timeout, this, &RobustnessManager::performPeriodicHealthCheck);
    m_healthCheckTimer->start(30000); // 每30秒进行健康检查
    
    qCInfo(robustness) << "RobustnessManager initialized";
}

RobustnessManager::~RobustnessManager()
{
    qCInfo(robustness) << "RobustnessManager destroyed";
}

void RobustnessManager::registerRecoveryAction(FailureType type, const QString& component, const RecoveryAction& action)
{
    QPair<FailureType, QString> key(type, component);
    m_recoveryActions[key] = action;
    
    qCInfo(robustness) << "Recovery action registered for" << static_cast<int>(type) 
                       << component << "strategy:" << static_cast<int>(action.strategy);
}

void RobustnessManager::reportFailure(const FailureInfo& failure)
{
    {
        QMutexLocker locker(&m_failureMutex);
        m_failureHistory.enqueue(failure);
        
        // 限制故障历史大小
        while (m_failureHistory.size() > 1000) {
            m_failureHistory.dequeue();
        }
    }
    
    m_failureCount[failure.type].fetchAndAddOrdered(1);
    
    emit failureDetected(failure);
    
    qCWarning(robustness) << "Failure reported:" << failure.component 
                          << "type:" << static_cast<int>(failure.type)
                          << "description:" << failure.description;
    
    // 尝试自动恢复
    executeRecovery(failure.type, failure.component);
}

bool RobustnessManager::executeRecovery(FailureType type, const QString& component)
{
    QPair<FailureType, QString> key(type, component);
    auto it = m_recoveryActions.find(key);
    
    if (it == m_recoveryActions.end()) {
        qCWarning(robustness) << "No recovery action found for" << static_cast<int>(type) << component;
        return false;
    }
    
    RecoveryAction& action = it.value();
    m_recoveryCount[type].fetchAndAddOrdered(1);
    
    emit recoveryTriggered(type, component);
    
    try {
        bool success = action.action();
        
        if (success) {
            m_recoverySuccess[type].fetchAndAddOrdered(1);
            action.currentRetries = 0;
            emit recoveryCompleted(type, component, true);
            
            qCInfo(robustness) << "Recovery successful for" << component;
            return true;
        } else {
            action.currentRetries++;
            
            if (action.currentRetries < action.maxRetries) {
                // 退避重试
                QTimer::singleShot(action.backoffDelay.count() * action.currentRetries, 
                                  [this, type, component]() {
                    executeRecovery(type, component);
                });
                
                qCWarning(robustness) << "Recovery failed, will retry for" << component
                                      << "attempt:" << action.currentRetries;
            } else {
                emit recoveryCompleted(type, component, false);
                qCCritical(robustness) << "Recovery failed after max retries for" << component;
            }
        }
    } catch (const std::exception& e) {
        emit recoveryCompleted(type, component, false);
        qCCritical(robustness) << "Recovery action exception:" << component << e.what();
    }
    
    return false;
}

void RobustnessManager::registerHealthChecker(const QString& component, std::function<bool()> checker)
{
    QMutexLocker locker(&m_healthMutex);
    m_healthCheckers[component] = checker;
    
    qCInfo(robustness) << "Health checker registered for:" << component;
}

RobustnessManager::SystemHealth RobustnessManager::getSystemHealth() const
{
    QMutexLocker locker(&m_healthMutex);
    return m_systemHealth;
}

void RobustnessManager::performHealthCheck()
{
    updateSystemHealth();
}

QJsonObject RobustnessManager::getFailureStatistics() const
{
    QJsonObject stats;
    
    for (auto it = m_failureCount.begin(); it != m_failureCount.end(); ++it) {
        QString typeName = QString::number(static_cast<int>(it.key()));
        stats[typeName] = it.value().loadAcquire();
    }
    
    return stats;
}

QJsonObject RobustnessManager::getRecoveryStatistics() const
{
    QJsonObject stats;
    
    for (auto it = m_recoveryCount.begin(); it != m_recoveryCount.end(); ++it) {
        QString typeName = QString::number(static_cast<int>(it.key()));
        QJsonObject typeStats;
        
        typeStats["total"] = it.value().loadAcquire();
        typeStats["successful"] = m_recoverySuccess[it.key()].loadAcquire();
        
        stats[typeName] = typeStats;
    }
    
    return stats;
}

void RobustnessManager::initializeSubManagers()
{
    m_circuitBreaker = new CircuitBreakerManager(this);
    m_memoryMonitor = new MemoryMonitor(this);
    m_threadStarvation = new ThreadStarvationDetector(this);
    m_degradationManager = new PerformanceDegradationManager(this);
    m_configManager = new HotConfigManager(this);
}

void RobustnessManager::setupSignalConnections()
{
    // 熔断器事件
    connect(m_circuitBreaker, &CircuitBreakerManager::circuitOpened,
            this, &RobustnessManager::handleCircuitBreakerEvent);
    
    // 内存监控事件
    connect(m_memoryMonitor, &MemoryMonitor::memoryWarning,
            this, &RobustnessManager::handleMemoryAlert);
    connect(m_memoryMonitor, &MemoryMonitor::memoryCritical,
            this, &RobustnessManager::handleMemoryAlert);
    connect(m_memoryMonitor, &MemoryMonitor::memoryEmergency,
            this, &RobustnessManager::handleMemoryAlert);
    
    // 线程饥饿事件
    connect(m_threadStarvation, &ThreadStarvationDetector::threadStarvationDetected,
            this, &RobustnessManager::handleThreadStarvation);
    
    // 性能降级事件
    connect(m_degradationManager, &PerformanceDegradationManager::degradationLevelChanged,
            this, &RobustnessManager::handlePerformanceDegradation);
    
    // 配置变更事件
    connect(m_configManager, &HotConfigManager::configChanged,
            this, &RobustnessManager::handleConfigChanged);
}

void RobustnessManager::performPeriodicHealthCheck()
{
    updateSystemHealth();
}

void RobustnessManager::handleCircuitBreakerEvent()
{
    // 处理熔断器事件
    qCInfo(robustness) << "Circuit breaker event handled";
}

void RobustnessManager::handleMemoryAlert()
{
    // 处理内存告警
    qCInfo(robustness) << "Memory alert handled";
}

void RobustnessManager::handleThreadStarvation()
{
    // 处理线程饥饿
    qCInfo(robustness) << "Thread starvation handled";
}

void RobustnessManager::handlePerformanceDegradation()
{
    // 处理性能降级
    qCInfo(robustness) << "Performance degradation handled";
}

void RobustnessManager::updateSystemHealth()
{
    QMutexLocker locker(&m_healthMutex);
    
    m_systemHealth.lastUpdate = QDateTime::currentDateTime();
    m_systemHealth.healthIssues.clear();
    m_systemHealth.componentHealth.clear();
    
    // 检查各组件健康状态
    bool allHealthy = true;
    for (auto it = m_healthCheckers.begin(); it != m_healthCheckers.end(); ++it) {
        try {
            bool healthy = it.value()();
            m_systemHealth.componentHealth[it.key()] = healthy;
            
            if (!healthy) {
                allHealthy = false;
                m_systemHealth.healthIssues.append(QString("Component unhealthy: %1").arg(it.key()));
            }
        } catch (const std::exception& e) {
            allHealthy = false;
            m_systemHealth.componentHealth[it.key()] = false;
            m_systemHealth.healthIssues.append(QString("Health check failed: %1 - %2").arg(it.key(), e.what()));
        }
    }
    
    m_systemHealth.isHealthy = allHealthy;
    m_systemHealth.healthScore = allHealthy ? 1.0 : 0.5; // 简化的健康评分
    
    emit systemHealthChanged(m_systemHealth);
}

void RobustnessManager::cleanupFailureHistory()
{
    QMutexLocker locker(&m_failureMutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-7); // 保留7天的故障历史
    
    while (!m_failureHistory.isEmpty() && m_failureHistory.head().timestamp < cutoff) {
        m_failureHistory.dequeue();
    }
}

#include "RobustnessManager.moc"