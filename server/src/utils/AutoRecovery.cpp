#include "AutoRecovery.h"
#include <QDebug>
#include <QUuid>
#include <QCoreApplication>
#include <QThread>
#include <QJsonDocument>
#include <QJsonArray>
#include <algorithm>

Q_LOGGING_CATEGORY(autoRecovery, "qkchat.server.autorecovery")

AutoRecovery* AutoRecovery::s_instance = nullptr;
QMutex AutoRecovery::s_instanceMutex;

AutoRecovery* AutoRecovery::instance()
{
    QMutexLocker locker(&s_instanceMutex);
    if (!s_instance) {
        s_instance = new AutoRecovery();
    }
    return s_instance;
}

AutoRecovery::AutoRecovery(QObject *parent)
    : QObject(parent)
    , m_recoveryTimer(new QTimer(this))
    , m_maintenanceTimer(new QTimer(this))
    , m_analysisTimer(new QTimer(this))
    , m_cleanupTimer(new QTimer(this))
{
    connect(m_recoveryTimer, &QTimer::timeout, this, &AutoRecovery::processRecoveryQueue);
    connect(m_maintenanceTimer, &QTimer::timeout, this, &AutoRecovery::performPreventiveMaintenance);
    connect(m_analysisTimer, &QTimer::timeout, this, &AutoRecovery::analyzeSystemHealth);
    connect(m_cleanupTimer, &QTimer::timeout, this, &AutoRecovery::cleanupOldData);
    
    qCInfo(autoRecovery) << "AutoRecovery created";
}

AutoRecovery::~AutoRecovery()
{
    shutdown();
    qCInfo(autoRecovery) << "AutoRecovery destroyed";
}

bool AutoRecovery::initialize(const RecoveryConfig& config)
{
    qCInfo(autoRecovery) << "Initializing AutoRecovery...";
    
    m_config = config;
    
    if (config.enabled) {
        // 注册默认恢复动作
        registerDefaultRecoveryActions();
        
        // 启动定时器
        m_recoveryTimer->start(5000); // 每5秒检查恢复队列
        m_maintenanceTimer->start(3600000); // 每小时执行预防性维护
        m_analysisTimer->start(300000); // 每5分钟分析系统健康
        m_cleanupTimer->start(1800000); // 每30分钟清理旧数据
        
        m_enabled.storeRelease(1);
        
        qCInfo(autoRecovery) << "AutoRecovery initialized successfully";
        return true;
    }
    
    qCInfo(autoRecovery) << "AutoRecovery disabled by configuration";
    return true;
}

void AutoRecovery::shutdown()
{
    if (m_enabled.testAndSetOrdered(1, 0)) {
        qCInfo(autoRecovery) << "Shutting down AutoRecovery...";
        
        m_recoveryTimer->stop();
        m_maintenanceTimer->stop();
        m_analysisTimer->stop();
        m_cleanupTimer->stop();
        
        // 等待活跃恢复完成
        while (m_activeRecoveries.loadAcquire() > 0) {
            QCoreApplication::processEvents();
            QThread::msleep(100);
        }
        
        qCInfo(autoRecovery) << "AutoRecovery shutdown complete";
    }
}

bool AutoRecovery::isEnabled() const
{
    return m_enabled.loadAcquire() > 0;
}

void AutoRecovery::reportFailure(const QString& component, FailureType type, const QString& description,
                                const QJsonObject& context)
{
    if (!isEnabled()) {
        return;
    }
    
    QString failureId = generateFailureId();
    
    FailureInfo failure;
    failure.id = failureId;
    failure.component = component;
    failure.type = type;
    failure.description = description;
    failure.context = context;
    
    QMutexLocker locker(&m_dataMutex);
    
    // 检查是否已存在相同的故障
    QString key = QString("%1:%2").arg(component).arg(static_cast<int>(type));
    if (m_activeFailures.contains(key)) {
        // 更新现有故障
        FailureInfo& existing = m_activeFailures[key];
        existing.lastOccurrence = QDateTime::currentDateTime();
        existing.occurrenceCount++;
        existing.context = context; // 更新上下文
    } else {
        // 添加新故障
        m_activeFailures[key] = failure;
        m_failureHistory.append(failure);
        
        // 更新统计
        m_stats.totalFailures++;
        m_stats.failuresByType[type]++;
        m_stats.lastFailure = failure.detectedAt;
        
        emit failureDetected(failure);
        
        // 触发恢复
        if (failure.occurrenceCount >= m_config.failureThreshold) {
            m_recoveryQueue.append(component);
        }
    }
    
    logRecoveryEvent("FAILURE_REPORTED", QString("Component: %1, Type: %2, Description: %3")
                    .arg(component).arg(static_cast<int>(type)).arg(description));
}

void AutoRecovery::reportFailureResolved(const QString& failureId)
{
    QMutexLocker locker(&m_dataMutex);
    
    // 查找并解决故障
    for (auto it = m_activeFailures.begin(); it != m_activeFailures.end(); ++it) {
        if (it.value().id == failureId) {
            it.value().resolved = true;
            emit failureResolved(failureId);
            m_activeFailures.erase(it);
            break;
        }
    }
    
    logRecoveryEvent("FAILURE_RESOLVED", failureId);
}

void AutoRecovery::registerRecoveryAction(const QString& component, FailureType failureType,
                                         const RecoveryAction& action)
{
    QString key = QString("%1:%2").arg(component).arg(static_cast<int>(failureType));
    
    QMutexLocker locker(&m_dataMutex);
    m_recoveryActions[key].append(action);
    
    logRecoveryEvent("ACTION_REGISTERED", QString("Component: %1, Type: %2, Action: %3")
                    .arg(component).arg(static_cast<int>(failureType)).arg(action.name));
}

bool AutoRecovery::triggerRecovery(const QString& component, FailureType failureType)
{
    if (!isEnabled()) {
        return false;
    }
    
    if (m_activeRecoveries.loadAcquire() >= m_config.maxConcurrentRecoveries) {
        qCWarning(autoRecovery) << "Maximum concurrent recoveries reached";
        return false;
    }
    
    executeRecovery(component, failureType);
    return true;
}

QList<FailureInfo> AutoRecovery::getActiveFailures() const
{
    QMutexLocker locker(&m_dataMutex);
    
    QList<FailureInfo> failures;
    for (const auto& failure : m_activeFailures) {
        failures.append(failure);
    }
    
    return failures;
}

QList<RecoveryResult> AutoRecovery::getRecoveryHistory(const QString& component) const
{
    QMutexLocker locker(&m_dataMutex);
    
    if (component.isEmpty()) {
        return m_recoveryHistory;
    }
    
    QList<RecoveryResult> filtered;
    for (const auto& result : m_recoveryHistory) {
        if (result.actionId.contains(component)) {
            filtered.append(result);
        }
    }
    
    return filtered;
}

AutoRecovery::RecoveryStats AutoRecovery::getStats() const
{
    return m_stats;
}

void AutoRecovery::processRecoveryQueue()
{
    if (m_recoveryQueue.isEmpty()) {
        return;
    }
    
    if (m_activeRecoveries.loadAcquire() >= m_config.maxConcurrentRecoveries) {
        return;
    }
    
    QString component = m_recoveryQueue.takeFirst();
    executeRecovery(component, FailureType::Unknown);
}

void AutoRecovery::performPreventiveMaintenance()
{
    if (!m_config.enablePreventiveMaintenance) {
        return;
    }
    
    qCDebug(autoRecovery) << "Performing preventive maintenance...";
    
    QMutexLocker locker(&m_dataMutex);
    
    QDateTime now = QDateTime::currentDateTime();
    for (auto it = m_maintenanceActions.begin(); it != m_maintenanceActions.end(); ++it) {
        QString component = it.key();
        QDateTime lastMaintenance = m_lastMaintenance.value(component);
        
        if (!lastMaintenance.isValid() || lastMaintenance.addSecs(86400) < now) { // 24小时
            try {
                bool success = it.value()();
                m_lastMaintenance[component] = now;
                emit preventiveMaintenanceExecuted(component, success);
                
                logRecoveryEvent("PREVENTIVE_MAINTENANCE", 
                               QString("Component: %1, Success: %2").arg(component).arg(success));
            } catch (const std::exception& e) {
                qCWarning(autoRecovery) << "Preventive maintenance failed for" << component << ":" << e.what();
            }
        }
    }
}

void AutoRecovery::analyzeSystemHealth()
{
    bool wasStable = isSystemStable();
    updateSystemStability();
    bool isStable = isSystemStable();
    
    if (wasStable && !isStable) {
        emit systemUnstable();
        logRecoveryEvent("SYSTEM_UNSTABLE", "System stability degraded");
    } else if (!wasStable && isStable) {
        emit systemStabilized();
        logRecoveryEvent("SYSTEM_STABILIZED", "System stability restored");
    }
    
    // 分析故障模式
    if (m_config.enableLearning) {
        analyzeFailurePatterns();
    }
}

void AutoRecovery::cleanupOldData()
{
    QMutexLocker locker(&m_dataMutex);
    
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-7); // 保留7天
    
    // 清理故障历史
    m_failureHistory.erase(
        std::remove_if(m_failureHistory.begin(), m_failureHistory.end(),
                      [cutoff](const FailureInfo& failure) {
                          return failure.detectedAt < cutoff;
                      }),
        m_failureHistory.end());
    
    // 清理恢复历史
    m_recoveryHistory.erase(
        std::remove_if(m_recoveryHistory.begin(), m_recoveryHistory.end(),
                      [cutoff](const RecoveryResult& result) {
                          return result.timestamp < cutoff;
                      }),
        m_recoveryHistory.end());
    
    logRecoveryEvent("DATA_CLEANUP", "Old data cleaned up");
}

QString AutoRecovery::generateFailureId() const
{
    return QString("failure_%1_%2")
           .arg(QDateTime::currentMSecsSinceEpoch())
           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}

QString AutoRecovery::generateActionId() const
{
    return QString("action_%1_%2")
           .arg(QDateTime::currentMSecsSinceEpoch())
           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
}

void AutoRecovery::executeRecovery(const QString& component, FailureType failureType)
{
    m_activeRecoveries.fetchAndAddOrdered(1);
    
    emit recoveryStarted(component, QString("recovery_%1").arg(component));
    
    QList<RecoveryAction> actions = getRecoveryActions(component, failureType);
    if (actions.isEmpty()) {
        qCWarning(autoRecovery) << "No recovery actions found for component:" << component;
        m_activeRecoveries.fetchAndSubOrdered(1);
        return;
    }
    
    // 按优先级排序
    sortActionsByPriority(actions);
    
    bool recoverySuccessful = false;
    for (const auto& action : actions) {
        if (!action.canAttempt()) {
            continue;
        }
        
        RecoveryResult result;
        result.actionId = action.id;
        result.attemptNumber = action.currentAttempts + 1;
        
        QDateTime startTime = QDateTime::currentDateTime();
        
        try {
            bool success = executeAction(action);
            result.success = success;
            result.executionTime = startTime.msecsTo(QDateTime::currentDateTime());
            
            if (success) {
                recoverySuccessful = true;
                result.message = "Recovery action executed successfully";
                break;
            } else {
                result.message = "Recovery action failed";
            }
        } catch (const std::exception& e) {
            result.success = false;
            result.message = QString("Exception during recovery: %1").arg(e.what());
            result.executionTime = startTime.msecsTo(QDateTime::currentDateTime());
        }
        
        updateRecoveryStats(result);
        
        QMutexLocker locker(&m_dataMutex);
        m_recoveryHistory.append(result);
    }
    
    if (recoverySuccessful) {
        emit recoveryCompleted(RecoveryResult());
        logRecoveryEvent("RECOVERY_SUCCESS", component);
    } else {
        emit recoveryFailed(component, "All recovery actions failed");
        logRecoveryEvent("RECOVERY_FAILED", component);
    }
    
    m_activeRecoveries.fetchAndSubOrdered(1);
}

bool AutoRecovery::executeAction(const RecoveryAction& action)
{
    if (!action.action) {
        return false;
    }
    
    try {
        return action.action();
    } catch (const std::exception& e) {
        qCWarning(autoRecovery) << "Recovery action" << action.name << "threw exception:" << e.what();
        return false;
    }
}

QList<RecoveryAction> AutoRecovery::getRecoveryActions(const QString& component, FailureType failureType) const
{
    QMutexLocker locker(&m_dataMutex);
    
    QString key = QString("%1:%2").arg(component).arg(static_cast<int>(failureType));
    if (m_recoveryActions.contains(key)) {
        return m_recoveryActions[key];
    }
    
    // 如果没有特定类型的动作，尝试通用动作
    QString genericKey = QString("%1:%2").arg(component).arg(static_cast<int>(FailureType::Unknown));
    if (m_recoveryActions.contains(genericKey)) {
        return m_recoveryActions[genericKey];
    }
    
    return QList<RecoveryAction>();
}

void AutoRecovery::sortActionsByPriority(QList<RecoveryAction>& actions) const
{
    std::sort(actions.begin(), actions.end(),
             [](const RecoveryAction& a, const RecoveryAction& b) {
                 return a.priority < b.priority; // 数字越小优先级越高
             });
}

bool AutoRecovery::isSystemStable() const
{
    QMutexLocker locker(&m_dataMutex);
    
    // 检查最近的故障数量
    QDateTime recentTime = QDateTime::currentDateTime().addSecs(-300); // 5分钟内
    int recentFailures = 0;
    
    for (const auto& failure : m_activeFailures) {
        if (failure.lastOccurrence > recentTime) {
            recentFailures++;
        }
    }
    
    return recentFailures < 3; // 5分钟内少于3个故障认为是稳定的
}

void AutoRecovery::updateSystemStability()
{
    // 系统稳定性更新逻辑
}

void AutoRecovery::updateRecoveryStats(const RecoveryResult& result)
{
    m_stats.totalRecoveries++;
    if (result.success) {
        m_stats.successfulRecoveries++;
    } else {
        m_stats.failedRecoveries++;
    }
    
    if (m_stats.totalRecoveries > 0) {
        m_stats.successRate = static_cast<double>(m_stats.successfulRecoveries) / m_stats.totalRecoveries;
    }
    
    m_stats.lastRecovery = result.timestamp;
}

void AutoRecovery::analyzeFailurePatterns()
{
    // 故障模式分析的简化实现
    QMutexLocker locker(&m_dataMutex);
    
    // 按组件分组故障
    QHash<QString, QList<FailureInfo>> componentFailures;
    for (const auto& failure : m_failureHistory) {
        componentFailures[failure.component].append(failure);
    }
    
    // 分析每个组件的故障模式
    for (auto it = componentFailures.begin(); it != componentFailures.end(); ++it) {
        const QString& component = it.key();
        const QList<FailureInfo>& failures = it.value();
        
        if (failures.size() >= 3) {
            // 检查是否有重复的故障模式
            QHash<FailureType, int> typeCount;
            for (const auto& failure : failures) {
                typeCount[failure.type]++;
            }
            
            for (auto typeIt = typeCount.begin(); typeIt != typeCount.end(); ++typeIt) {
                if (typeIt.value() >= 2) {
                    logRecoveryEvent("PATTERN_DETECTED", 
                                   QString("Component: %1, Type: %2, Count: %3")
                                   .arg(component).arg(static_cast<int>(typeIt.key())).arg(typeIt.value()));
                }
            }
        }
    }
}

void AutoRecovery::registerDefaultRecoveryActions()
{
    // 注册默认的恢复动作
    
    // 连接故障恢复
    registerRecoveryAction("ConnectionManager", FailureType::ConnectionFailure,
                          createReconnectAction("ConnectionManager"));
    
    // 数据库故障恢复
    registerRecoveryAction("DatabasePool", FailureType::DatabaseFailure,
                          createRestartAction("DatabasePool"));
    
    // 内存泄漏恢复
    registerRecoveryAction("MemoryManager", FailureType::MemoryLeak,
                          createClearCacheAction("MemoryManager"));
    
    // 线程死锁恢复
    registerRecoveryAction("ThreadManager", FailureType::ThreadDeadlock,
                          createRestartAction("ThreadManager"));
    
    // 队列溢出恢复
    registerRecoveryAction("MessageEngine", FailureType::QueueOverflow,
                          createReduceLoadAction("MessageEngine"));
}

RecoveryAction AutoRecovery::createRestartAction(const QString& component)
{
    RecoveryAction action;
    action.id = generateActionId();
    action.name = QString("Restart %1").arg(component);
    action.strategy = RecoveryStrategy::Restart;
    action.priority = 5;
    action.maxAttempts = 3;
    action.cooldownSeconds = 60;
    
    action.action = [component]() -> bool {
        qCInfo(autoRecovery) << "Restarting component:" << component;
        // 实际的重启逻辑应该在这里实现
        return true;
    };
    
    return action;
}

RecoveryAction AutoRecovery::createReconnectAction(const QString& component)
{
    RecoveryAction action;
    action.id = generateActionId();
    action.name = QString("Reconnect %1").arg(component);
    action.strategy = RecoveryStrategy::Reconnect;
    action.priority = 3;
    action.maxAttempts = 5;
    action.cooldownSeconds = 30;
    
    action.action = [component]() -> bool {
        qCInfo(autoRecovery) << "Reconnecting component:" << component;
        // 实际的重连逻辑应该在这里实现
        return true;
    };
    
    return action;
}

RecoveryAction AutoRecovery::createClearCacheAction(const QString& component)
{
    RecoveryAction action;
    action.id = generateActionId();
    action.name = QString("Clear Cache %1").arg(component);
    action.strategy = RecoveryStrategy::ClearCache;
    action.priority = 2;
    action.maxAttempts = 2;
    action.cooldownSeconds = 120;
    
    action.action = [component]() -> bool {
        qCInfo(autoRecovery) << "Clearing cache for component:" << component;
        // 实际的缓存清理逻辑应该在这里实现
        return true;
    };
    
    return action;
}

RecoveryAction AutoRecovery::createReduceLoadAction(const QString& component)
{
    RecoveryAction action;
    action.id = generateActionId();
    action.name = QString("Reduce Load %1").arg(component);
    action.strategy = RecoveryStrategy::ReduceLoad;
    action.priority = 1;
    action.maxAttempts = 3;
    action.cooldownSeconds = 60;
    
    action.action = [component]() -> bool {
        qCInfo(autoRecovery) << "Reducing load for component:" << component;
        // 实际的负载降低逻辑应该在这里实现
        return true;
    };
    
    return action;
}

void AutoRecovery::logRecoveryEvent(const QString& event, const QString& details) const
{
    if (details.isEmpty()) {
        qCDebug(autoRecovery) << event;
    } else {
        qCDebug(autoRecovery) << event << ":" << details;
    }
}
