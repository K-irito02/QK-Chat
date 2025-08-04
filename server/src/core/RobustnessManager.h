#ifndef ROBUSTNESS_MANAGER_H
#define ROBUSTNESS_MANAGER_H

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include <QQueue>
#include <QAtomicInt>
#include <QDateTime>
#include <QLoggingCategory>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>
#include <functional>
#include <chrono>
#include <thread>
#include <atomic>

Q_DECLARE_LOGGING_CATEGORY(robustness)

/**
 * @brief 故障类型枚举
 */
enum class RobustnessFailureType {
    DatabaseFailure,      // 数据库故障
    NetworkFailure,       // 网络故障
    ThreadPoolFailure,    // 线程池故障
    MemoryExhaustion,     // 内存耗尽
    SSLHandshakeFailure,  // SSL握手失败
    MessageQueueOverflow, // 消息队列溢出
    ComponentCrash        // 组件崩溃
};

/**
 * @brief 恢复策略枚举
 */
enum class RobustnessRecoveryStrategy {
    Restart,              // 重启组件
    Fallback,             // 降级处理
    CircuitBreaker,       // 熔断保护
    RetryWithBackoff,     // 退避重试
    GradualRecovery,      // 逐步恢复
    ManualIntervention    // 手动干预
};

/**
 * @brief 故障信息结构
 */
struct RobustnessFailureInfo {
    RobustnessFailureType type;
    QString component;
    QString description;
    QDateTime timestamp;
    int severity{0};      // 严重程度 0-10
    QJsonObject context;  // 故障上下文
    QString stackTrace;   // 堆栈跟踪
};

/**
 * @brief 恢复动作结构
 */
struct RobustnessRecoveryAction {
    RobustnessRecoveryStrategy strategy;
    std::function<bool()> action;
    int maxRetries{3};
    int currentRetries{0};
    std::chrono::milliseconds backoffDelay{1000};
    bool isAsync{false};
};

/**
 * @brief 熔断器状态
 */
enum class CircuitBreakerState {
    Closed,     // 正常状态
    Open,       // 熔断状态
    HalfOpen    // 半开状态
};

/**
 * @brief 熔断器管理
 */
class CircuitBreakerManager : public QObject
{
    Q_OBJECT

public:
    struct CircuitConfig {
        int failureThreshold{10};        // 失败阈值
        int successThreshold{5};         // 成功阈值
        std::chrono::milliseconds timeout{30000}; // 超时时间
        std::chrono::milliseconds halfOpenTimeout{10000}; // 半开超时
    };

    struct CircuitStats {
        QAtomicInt totalRequests{0};
        QAtomicInt successfulRequests{0};
        QAtomicInt failedRequests{0};
        QAtomicInt consecutiveFailures{0};
        QAtomicInt consecutiveSuccesses{0};
        QDateTime lastFailureTime;
        QDateTime lastSuccessTime;
        CircuitBreakerState state{CircuitBreakerState::Closed};
    };

    explicit CircuitBreakerManager(QObject *parent = nullptr);
    
    void registerCircuit(const QString& circuitName, const CircuitConfig& config);
    bool canExecute(const QString& circuitName);
    void recordSuccess(const QString& circuitName);
    void recordFailure(const QString& circuitName);
    CircuitBreakerState getState(const QString& circuitName) const;
    CircuitStats getStats(const QString& circuitName) const;

signals:
    void circuitOpened(const QString& circuitName);
    void circuitClosed(const QString& circuitName);
    void circuitHalfOpened(const QString& circuitName);

private:
    QHash<QString, CircuitConfig> m_configs;
    QHash<QString, CircuitStats> m_stats;
    mutable QMutex m_mutex;
    
    void updateCircuitState(const QString& circuitName);
};

/**
 * @brief 内存监控器
 */
class MemoryMonitor : public QObject
{
    Q_OBJECT

public:
    struct MemoryStats {
        qint64 totalMemory{0};
        qint64 usedMemory{0};
        qint64 freeMemory{0};
        qint64 processMemory{0};
        double memoryUsagePercent{0.0};
        QDateTime timestamp;
    };

    struct MemoryThresholds {
        double warningThreshold{0.8};    // 80%
        double criticalThreshold{0.9};   // 90%
        double emergencyThreshold{0.95}; // 95%
    };

    explicit MemoryMonitor(QObject *parent = nullptr);
    
    void setThresholds(const MemoryThresholds& thresholds);
    MemoryStats getCurrentStats() const;
    void startMonitoring(int intervalMs = 5000);
    void stopMonitoring();
    
    // 内存清理策略
    void registerCleanupHandler(const QString& name, std::function<bool(int)> handler);
    void triggerCleanup(int level = 1); // 1=轻度, 2=中度, 3=重度

signals:
    void memoryWarning(double usagePercent);
    void memoryCritical(double usagePercent);
    void memoryEmergency(double usagePercent);
    void cleanupCompleted(const QString& handlerName, bool success);

private slots:
    void checkMemoryUsage();

private:
    QTimer* m_monitorTimer;
    MemoryThresholds m_thresholds;
    MemoryStats m_lastStats;
    QHash<QString, std::function<bool(int)>> m_cleanupHandlers;
    
    MemoryStats collectMemoryStats() const;
    void executeCleanupHandlers(int level);
};

/**
 * @brief 线程饥饿检测器
 */
class ThreadStarvationDetector : public QObject
{
    Q_OBJECT

public:
    struct ThreadInfo {
        QThread* thread;
        QString threadName;
        QDateTime lastActivity;
        QAtomicInt taskCount{0};
        QAtomicInt completedTasks{0};
        bool isStarving{false};
    };

    explicit ThreadStarvationDetector(QObject *parent = nullptr);
    
    void registerThread(QThread* thread, const QString& name);
    void recordActivity(QThread* thread);
    void recordTaskCompletion(QThread* thread);
    void setStarvationThreshold(int seconds) { m_starvationThreshold = seconds; }
    
    QList<ThreadInfo> getStarvingThreads() const;

signals:
    void threadStarvationDetected(const QString& threadName);
    void threadRecovered(const QString& threadName);

private slots:
    void checkThreadStarvation();

private:
    QHash<QThread*, ThreadInfo> m_threadInfo;
    QTimer* m_checkTimer;
    int m_starvationThreshold{30}; // 30秒
    mutable QMutex m_mutex;
};

/**
 * @brief 性能降级管理器
 */
class PerformanceDegradationManager : public QObject
{
    Q_OBJECT

public:
    enum class DegradationLevel {
        Normal = 0,
        Light = 1,      // 轻度降级
        Moderate = 2,   // 中度降级
        Heavy = 3,      // 重度降级
        Emergency = 4   // 紧急降级
    };

    struct DegradationConfig {
        double cpuThreshold{0.8};
        double memoryThreshold{0.8};
        double diskIOThreshold{0.8};
        double networkThreshold{0.8};
        int responseTimeThreshold{1000}; // 毫秒
    };

    explicit PerformanceDegradationManager(QObject *parent = nullptr);
    
    void setConfig(const DegradationConfig& config);
    DegradationLevel getCurrentLevel() const;
    void registerDegradationHandler(DegradationLevel level, std::function<void()> handler);
    void updateSystemMetrics(double cpuUsage, double memoryUsage, 
                           double diskIO, double networkIO, int avgResponseTime);

signals:
    void degradationLevelChanged(DegradationLevel newLevel, DegradationLevel oldLevel);
    void performanceRecovered();

private:
    DegradationConfig m_config;
    std::atomic<DegradationLevel> m_currentLevel{DegradationLevel::Normal};
    QHash<DegradationLevel, std::function<void()>> m_handlers;
    
    // 系统指标
    std::atomic<double> m_cpuUsage{0.0};
    std::atomic<double> m_memoryUsage{0.0};
    std::atomic<double> m_diskIO{0.0};
    std::atomic<double> m_networkIO{0.0};
    std::atomic<int> m_avgResponseTime{0};
    
    DegradationLevel calculateDegradationLevel() const;
    void applyDegradation(DegradationLevel level);
};

/**
 * @brief 热更新配置管理器
 */
class HotConfigManager : public QObject
{
    Q_OBJECT

public:
    struct ConfigWatcher {
        QString filePath;
        QDateTime lastModified;
        std::function<void(const QJsonObject&)> callback;
    };

    explicit HotConfigManager(QObject *parent = nullptr);
    
    void watchConfigFile(const QString& filePath, std::function<void(const QJsonObject&)> callback);
    void unwatchConfigFile(const QString& filePath);
    void reloadAllConfigs();
    
    // 配置验证
    void setConfigValidator(const QString& filePath, std::function<bool(const QJsonObject&)> validator);

signals:
    void configChanged(const QString& filePath, const QJsonObject& newConfig);
    void configError(const QString& filePath, const QString& error);

private slots:
    void checkConfigChanges();

private:
    QTimer* m_watchTimer;
    QHash<QString, ConfigWatcher> m_watchers;
    QHash<QString, std::function<bool(const QJsonObject&)>> m_validators;
    
    QJsonObject loadConfigFile(const QString& filePath) const;
    bool validateConfig(const QString& filePath, const QJsonObject& config) const;
};

/**
 * @brief 健壮性管理器主类
 */
class RobustnessManager : public QObject
{
    Q_OBJECT

public:
    struct SystemHealth {
        bool isHealthy{true};
        double healthScore{1.0}; // 0.0-1.0
        QStringList healthIssues;
        QDateTime lastUpdate;
        
        // 组件健康状态
        QHash<QString, bool> componentHealth;
    };

    explicit RobustnessManager(QObject *parent = nullptr);
    ~RobustnessManager();
    
    // 故障恢复
    void registerRecoveryAction(RobustnessFailureType type, const QString& component, const RobustnessRecoveryAction& action);
    void reportFailure(const RobustnessFailureInfo& failure);
    bool executeRecovery(RobustnessFailureType type, const QString& component);
    
    // 健康检查
    void registerHealthChecker(const QString& component, std::function<bool()> checker);
    SystemHealth getSystemHealth() const;
    void performHealthCheck();
    
    // 组件管理
    CircuitBreakerManager* circuitBreakerManager() const { return m_circuitBreaker; }
    MemoryMonitor* memoryMonitor() const { return m_memoryMonitor; }
    ThreadStarvationDetector* threadStarvationDetector() const { return m_threadStarvation; }
    PerformanceDegradationManager* degradationManager() const { return m_degradationManager; }
    HotConfigManager* configManager() const { return m_configManager; }
    
    // 统计信息
    QJsonObject getFailureStatistics() const;
    QJsonObject getRecoveryStatistics() const;

signals:
    void failureDetected(const RobustnessFailureInfo& failure);
    void recoveryTriggered(RobustnessFailureType type, const QString& component);
    void recoveryCompleted(RobustnessFailureType type, const QString& component, bool success);
    void systemHealthChanged(const SystemHealth& health);

private slots:
    void performPeriodicHealthCheck();
    void handleCircuitBreakerEvent();
    void handleMemoryAlert();
    void handleThreadStarvation();
    void handlePerformanceDegradation();
    void handleConfigChanged();

private:
    // 故障恢复
    QHash<QPair<RobustnessFailureType, QString>, RobustnessRecoveryAction> m_recoveryActions;
    QQueue<RobustnessFailureInfo> m_failureHistory;
    mutable QMutex m_failureMutex;
    
    // 健康检查
    QHash<QString, std::function<bool()>> m_healthCheckers;
    SystemHealth m_systemHealth;
    QTimer* m_healthCheckTimer;
    mutable QMutex m_healthMutex;
    
    // 子管理器
    CircuitBreakerManager* m_circuitBreaker;
    MemoryMonitor* m_memoryMonitor;
    ThreadStarvationDetector* m_threadStarvation;
    PerformanceDegradationManager* m_degradationManager;
    HotConfigManager* m_configManager;
    
    // 统计信息
    QHash<RobustnessFailureType, QAtomicInt> m_failureCount;
    QHash<RobustnessFailureType, QAtomicInt> m_recoveryCount;
    QHash<RobustnessFailureType, QAtomicInt> m_recoverySuccess;
    
    void initializeSubManagers();
    void setupSignalConnections();
    void executeRecoveryAction(const RobustnessRecoveryAction& action, const RobustnessFailureInfo& failure);
    void updateSystemHealth();
    void cleanupFailureHistory();
};

#endif // ROBUSTNESS_MANAGER_H