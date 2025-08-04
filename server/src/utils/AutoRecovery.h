#ifndef AUTORECOVERY_H
#define AUTORECOVERY_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <QMutex>
#include <QHash>
#include <QJsonObject>
#include <functional>
#include <memory>

Q_DECLARE_LOGGING_CATEGORY(autoRecovery)

/**
 * @brief 故障类型
 */
enum class FailureType {
    Unknown = 0,
    ConnectionFailure = 1,      // 连接故障
    DatabaseFailure = 2,        // 数据库故障
    MemoryLeak = 3,            // 内存泄漏
    ThreadDeadlock = 4,        // 线程死锁
    QueueOverflow = 5,         // 队列溢出
    PerformanceDegradation = 6, // 性能下降
    ServiceTimeout = 7,        // 服务超时
    ResourceExhaustion = 8,    // 资源耗尽
    NetworkFailure = 9,        // 网络故障
    ConfigurationError = 10    // 配置错误
};

/**
 * @brief 恢复策略
 */
enum class RecoveryStrategy {
    None = 0,
    Restart = 1,               // 重启组件
    Reconnect = 2,             // 重新连接
    ClearCache = 3,            // 清理缓存
    ReduceLoad = 4,            // 降低负载
    Failover = 5,              // 故障转移
    GracefulDegradation = 6,   // 优雅降级
    ResourceCleanup = 7,       // 资源清理
    ConfigReload = 8,          // 重新加载配置
    Custom = 9                 // 自定义策略
};

/**
 * @brief 故障信息
 */
struct FailureInfo {
    QString id;
    QString component;
    FailureType type;
    QString description;
    QDateTime detectedAt;
    QDateTime lastOccurrence;
    int occurrenceCount;
    QJsonObject context;
    bool resolved;
    
    FailureInfo() 
        : type(FailureType::Unknown)
        , detectedAt(QDateTime::currentDateTime())
        , lastOccurrence(QDateTime::currentDateTime())
        , occurrenceCount(1)
        , resolved(false)
    {}
};

/**
 * @brief 恢复动作
 */
struct RecoveryAction {
    QString id;
    QString name;
    RecoveryStrategy strategy;
    std::function<bool()> action;
    int priority;              // 优先级（数字越小优先级越高）
    int maxAttempts;           // 最大尝试次数
    int currentAttempts;       // 当前尝试次数
    int cooldownSeconds;       // 冷却时间（秒）
    QDateTime lastAttempt;     // 最后尝试时间
    bool enabled;
    
    RecoveryAction()
        : strategy(RecoveryStrategy::None)
        , priority(10)
        , maxAttempts(3)
        , currentAttempts(0)
        , cooldownSeconds(60)
        , enabled(true)
    {}
    
    bool canAttempt() const {
        if (!enabled || currentAttempts >= maxAttempts) {
            return false;
        }
        
        if (lastAttempt.isValid()) {
            QDateTime cooldownEnd = lastAttempt.addSecs(cooldownSeconds);
            return QDateTime::currentDateTime() > cooldownEnd;
        }
        
        return true;
    }
};

/**
 * @brief 恢复结果
 */
struct RecoveryResult {
    QString actionId;
    bool success;
    QString message;
    QDateTime timestamp;
    int attemptNumber;
    qint64 executionTime;      // 毫秒
    
    RecoveryResult()
        : success(false)
        , timestamp(QDateTime::currentDateTime())
        , attemptNumber(0)
        , executionTime(0)
    {}
};

/**
 * @brief 自动恢复系统
 * 
 * 功能：
 * - 故障检测和分类
 * - 自动恢复策略执行
 * - 恢复历史记录
 * - 故障模式学习
 * - 预防性维护
 */
class AutoRecovery : public QObject
{
    Q_OBJECT

public:
    struct RecoveryConfig {
        bool enabled;
        int maxConcurrentRecoveries;   // 最大并发恢复数
        int globalCooldownSeconds;     // 全局冷却时间
        int maxRecoveryAttempts;       // 全局最大尝试次数
        int failureThreshold;          // 故障阈值
        int recoveryTimeoutSeconds;    // 恢复超时时间
        bool enablePreventiveMaintenance; // 启用预防性维护
        bool enableLearning;           // 启用学习模式
        
        RecoveryConfig()
            : enabled(true)
            , maxConcurrentRecoveries(3)
            , globalCooldownSeconds(300)
            , maxRecoveryAttempts(5)
            , failureThreshold(3)
            , recoveryTimeoutSeconds(120)
            , enablePreventiveMaintenance(true)
            , enableLearning(true)
        {}
    };

    struct RecoveryStats {
        int totalFailures;
        int totalRecoveries;
        int successfulRecoveries;
        int failedRecoveries;
        double successRate;
        QDateTime lastFailure;
        QDateTime lastRecovery;
        QHash<FailureType, int> failuresByType;
        QHash<RecoveryStrategy, int> strategiesUsed;
    };

    static AutoRecovery* instance();
    
    // 初始化和配置
    bool initialize(const RecoveryConfig& config = RecoveryConfig());
    void shutdown();
    bool isEnabled() const;
    
    // 故障报告
    void reportFailure(const QString& component, FailureType type, const QString& description,
                      const QJsonObject& context = QJsonObject());
    void reportFailureResolved(const QString& failureId);
    
    // 恢复动作注册
    void registerRecoveryAction(const QString& component, FailureType failureType,
                               const RecoveryAction& action);
    void unregisterRecoveryAction(const QString& component, FailureType failureType,
                                 const QString& actionId);
    
    // 手动恢复
    bool triggerRecovery(const QString& component, FailureType failureType = FailureType::Unknown);
    bool executeRecoveryAction(const QString& actionId);
    
    // 预防性维护
    void schedulePreventiveMaintenance(const QString& component, std::function<bool()> maintenanceAction,
                                      int intervalHours = 24);
    void cancelPreventiveMaintenance(const QString& component);
    
    // 状态查询
    QList<FailureInfo> getActiveFailures() const;
    QList<FailureInfo> getFailureHistory(const QString& component = QString()) const;
    QList<RecoveryResult> getRecoveryHistory(const QString& component = QString()) const;
    RecoveryStats getStats() const;
    
    // 配置管理
    void setRecoveryEnabled(bool enabled);
    void setMaxConcurrentRecoveries(int max);
    void setGlobalCooldown(int seconds);
    void enableLearning(bool enabled);
    
    // 学习和优化
    void analyzeFailurePatterns();
    void optimizeRecoveryStrategies();
    QJsonObject getFailureAnalysis() const;

signals:
    void failureDetected(const FailureInfo& failure);
    void failureResolved(const QString& failureId);
    void recoveryStarted(const QString& component, const QString& actionId);
    void recoveryCompleted(const RecoveryResult& result);
    void recoveryFailed(const QString& component, const QString& reason);
    void preventiveMaintenanceExecuted(const QString& component, bool success);
    void systemStabilized();
    void systemUnstable();

private slots:
    void processRecoveryQueue();
    void performPreventiveMaintenance();
    void analyzeSystemHealth();
    void cleanupOldData();

private:
    explicit AutoRecovery(QObject *parent = nullptr);
    ~AutoRecovery();
    
    static AutoRecovery* s_instance;
    static QMutex s_instanceMutex;
    
    // 配置
    RecoveryConfig m_config;
    QAtomicInt m_enabled{0};
    
    // 数据存储
    mutable QMutex m_dataMutex;
    QHash<QString, FailureInfo> m_activeFailures;
    QList<FailureInfo> m_failureHistory;
    QHash<QString, QList<RecoveryAction>> m_recoveryActions; // component -> actions
    QList<RecoveryResult> m_recoveryHistory;
    
    // 预防性维护
    QHash<QString, std::function<bool()>> m_maintenanceActions;
    QHash<QString, QDateTime> m_lastMaintenance;
    
    // 恢复队列
    QList<QString> m_recoveryQueue;
    QAtomicInt m_activeRecoveries{0};
    
    // 定时器
    QTimer* m_recoveryTimer;
    QTimer* m_maintenanceTimer;
    QTimer* m_analysisTimer;
    QTimer* m_cleanupTimer;
    
    // 统计信息
    RecoveryStats m_stats;
    
    // 学习数据
    QHash<QString, QList<FailureInfo>> m_failurePatterns;
    QHash<QString, double> m_strategyEffectiveness;
    
    // 内部方法
    QString generateFailureId() const;
    QString generateActionId() const;
    
    void executeRecovery(const QString& component, FailureType failureType);
    bool executeAction(const RecoveryAction& action);
    void updateRecoveryStats(const RecoveryResult& result);
    
    QList<RecoveryAction> getRecoveryActions(const QString& component, FailureType failureType) const;
    void sortActionsByPriority(QList<RecoveryAction>& actions) const;
    
    bool isSystemStable() const;
    void updateSystemStability();
    
    // 学习算法
    void learnFromFailure(const FailureInfo& failure);
    void learnFromRecovery(const RecoveryResult& result);
    void updateStrategyEffectiveness(const QString& strategy, bool success);
    
    // 预定义恢复动作
    void registerDefaultRecoveryActions();
    RecoveryAction createRestartAction(const QString& component);
    RecoveryAction createReconnectAction(const QString& component);
    RecoveryAction createClearCacheAction(const QString& component);
    RecoveryAction createReduceLoadAction(const QString& component);
    
    // 工具方法
    QString failureTypeToString(FailureType type) const;
    QString recoveryStrategyToString(RecoveryStrategy strategy) const;
    void logRecoveryEvent(const QString& event, const QString& details = QString()) const;
};

#endif // AUTORECOVERY_H
