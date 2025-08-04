#ifndef ENHANCED_CHAT_SERVER_H
#define ENHANCED_CHAT_SERVER_H

#include "ChatServer.h"
#include "ThreadSafetyEnhancements.h"
#include "RobustnessManager.h"
#include "StackTraceCollector.h"
#include "ArchitectureOptimizer.h"

#include <QObject>
#include <QJsonObject>
#include <QLoggingCategory>
#include <memory>

Q_DECLARE_LOGGING_CATEGORY(enhancedChatServer)

/**
 * @brief 增强版聊天服务器
 * 
 * 集成了以下增强功能：
 * 1. 线程安全增强：智能读写锁、无锁客户端管理、SSL会话缓存等
 * 2. 健壮性管理：故障恢复、内存监控、性能降级等
 * 3. 堆栈错误追踪：异常追踪、模式分析、自动修复建议
 * 4. 架构优化：集群支持、数据分片、分布式锁等
 */
class EnhancedChatServer : public ChatServer
{
    Q_OBJECT

public:
    struct EnhancementConfig {
        // 线程安全配置
        struct {
            bool enableSmartLocks{true};
            bool enableLockFreeClientManager{true};
            bool enableSSLSessionCache{true};
            bool enableBackpressureControl{true};
            int maxLockWaitTime{5000};
        } threadSafety;
        
        // 健壮性配置
        struct {
            bool enableCircuitBreaker{true};
            bool enableMemoryMonitor{true};
            bool enableThreadStarvationDetector{true};
            bool enablePerformanceDegradation{true};
            bool enableHotConfigReload{true};
            double memoryWarningThreshold{0.8};
            double memoryCriticalThreshold{0.9};
        } robustness;
        
        // 错误追踪配置
        struct {
            bool enableStackTraceCollection{true};
            bool enableExceptionPatternAnalysis{true};
            bool enableSignalHandling{true};
            int maxStackTraces{1000};
            int maxExceptions{500};
        } errorTracking;
        
        // 架构优化配置
        struct {
            bool enableClustering{false};
            bool enableSharding{false};
            bool enableServiceRegistry{false};
            bool enableAsyncLogging{true};
            bool enableDistributedLocks{false};
            QString nodeRole{"master"};
            QStringList seedNodes;
        } architecture;
    };

    explicit EnhancedChatServer(QObject *parent = nullptr);
    ~EnhancedChatServer();
    
    // 配置和初始化
    void setEnhancementConfig(const EnhancementConfig& config);
    bool initializeEnhancements();
    void shutdownEnhancements();
    
    // 重写父类方法以集成增强功能
    bool startServer(const QString& host = "0.0.0.0", int port = 8443);
    void stopServer();
    
    // 消息发送优化（使用无锁客户端管理）
    bool sendMessageToUser(qint64 userId, const QJsonObject& message);
    void broadcastMessage(const QJsonObject& message);
    void broadcastToAuthenticated(const QJsonObject& message);
    
    // 增强功能访问接口
    LockWaitMonitor* lockMonitor() const;
    ConnectionPoolEnhancer* poolEnhancer() const;
    SSLSessionManager* sslSessionManager() const;
    BackpressureController* backpressureController() const;
    
    RobustnessManager* robustnessManager() const { return m_robustnessManager; }
    StackTraceCollector* stackTraceCollector() const;
    ArchitectureOptimizer* architectureOptimizer() const { return m_architectureOptimizer; }
    
    // 统计和监控
    QJsonObject getEnhancedStatistics() const;
    QJsonObject getHealthReport() const;
    QJsonObject getPerformanceReport() const;
    QJsonObject getSecurityReport() const;
    QJsonObject collectSystemMetrics() const;
    QJsonObject collectNetworkMetrics() const;
    QJsonObject collectDatabaseMetrics() const;
    
    // 优化建议
    QStringList getOptimizationSuggestions() const;
    bool applyOptimization(const QString& optimization);
    
    // 故障处理
    void handleSystemFailure(RobustnessFailureType type, const QString& component, const QString& description);
    void triggerEmergencyMode();
    void exitEmergencyMode();

signals:
    void enhancementInitialized(const QString& enhancement);
    void systemHealthChanged(double healthScore);
    void emergencyModeActivated();
    void emergencyModeDeactivated();
    void optimizationApplied(const QString& optimization);
    void criticalErrorDetected(const QString& error);

private slots:
    // 线程安全事件处理
    void onDeadlockDetected(const QStringList& threads);
    void onLongWaitDetected(const QString& lockName, int waitTime);
    void onBackpressureLevelChanged(BackpressureController::BackpressureLevel level);
    
    // 健壮性事件处理
    void onCircuitBreakerOpened(const QString& circuitName);
    void onMemoryWarning(double usagePercent);
    void onThreadStarvationDetected(const QString& threadName);
    void onPerformanceDegradation(PerformanceDegradationManager::DegradationLevel level);
    void onConfigChanged(const QString& filePath, const QJsonObject& config);
    
    // 错误追踪事件处理
    void onCriticalExceptionDetected(const ExceptionInfo& exception);
    void onExceptionPatternDetected(const ExceptionPatternAnalyzer::ExceptionPattern& pattern);
    void onSignalCrash(const StackTrace& trace);
    
    // 架构优化事件处理
    void onNodeStatusChanged(const QString& nodeId, bool healthy);
    void onClusterStateChanged(bool healthy);
    void onShardMigrated(const QString& shardId, const QString& fromNode, const QString& toNode);

private:
    EnhancementConfig m_enhancementConfig;
    bool m_enhancementsInitialized{false};
    bool m_emergencyMode{false};
    
    // 增强组件
    RobustnessManager* m_robustnessManager;
    ArchitectureOptimizer* m_architectureOptimizer;
    
    // 无锁客户端管理器（替代原有的_clients和_userConnections）
    std::unique_ptr<LockFreeClientManager<QSslSocket*, ChatClientConnection>> m_lockFreeClients;
    std::unique_ptr<LockFreeClientManager<qint64, ChatClientConnection>> m_lockFreeUserConnections;
    
    // 智能读写锁（替代_clientsMutex）
    std::unique_ptr<SmartRWLock> m_clientsLock;
    
    // 背压控制器
    std::unique_ptr<BackpressureController> m_backpressureController;
    
    // 原子统计计数器（替代原有的统计变量）
    std::unique_ptr<AtomicStatsCounter> m_atomicStats;
    
    // 初始化方法
    bool initializeThreadSafetyEnhancements();
    bool initializeRobustnessManager();
    bool initializeErrorTracking();
    bool initializeArchitectureOptimizer();
    
    // 配置方法
    void applyThreadSafetyConfig();
    void applyRobustnessConfig();
    void applyErrorTrackingConfig();
    void applyArchitectureConfig();
    
    // 增强版客户端管理
    void addClientConnection(QSslSocket* socket, std::shared_ptr<ChatClientConnection> client);
    void removeClientConnection(QSslSocket* socket);
    void addUserConnection(qint64 userId, std::shared_ptr<ChatClientConnection> client);
    void removeUserConnection(qint64 userId);
    
    // 增强版消息处理
    void processMessageWithBackpressure(const QByteArray& message);
    void handleMessageQueueOverflow();
    
    // 故障恢复
    void setupFailureRecoveryActions();
    void registerHealthCheckers();
    
    // 性能监控
    void updatePerformanceMetrics();
    void checkSystemHealth();
    
    // 紧急模式处理
    void activateEmergencyMode();
    void deactivateEmergencyMode();
    void applyEmergencyMeasures();
    
    // 工具方法
    QString generateNodeId() const;
    
    // 优化建议实现
    bool optimizeThreadPoolConfiguration();
    bool optimizeDatabaseConnectionPool();
    bool optimizeSSLConfiguration();
    bool optimizeMemoryUsage();
    bool enableHorizontalScaling();
    
    // 连接增强版组件信号
    void connectThreadSafetySignals();
    void connectRobustnessSignals();
    void connectErrorTrackingSignals();
    void connectArchitectureSignals();
};

/**
 * @brief 系统健康度评估器
 */
class SystemHealthEvaluator
{
public:
    struct HealthMetrics {
        double cpuHealth{1.0};
        double memoryHealth{1.0};
        double networkHealth{1.0};
        double databaseHealth{1.0};
        double threadHealth{1.0};
        double overallHealth{1.0};
        QStringList issues;
    };
    
    static HealthMetrics evaluateSystemHealth(const EnhancedChatServer* server);
    static double calculateOverallHealth(const HealthMetrics& metrics);
    static QStringList generateHealthSuggestions(const HealthMetrics& metrics);
    
private:
    static double evaluateCPUHealth(double cpuUsage);
    static double evaluateMemoryHealth(double memoryUsage);
    static double evaluateNetworkHealth(const QJsonObject& networkStats);
    static double evaluateDatabaseHealth(const QJsonObject& dbStats);
    static double evaluateThreadHealth(const QJsonObject& threadStats);
};

/**
 * @brief 自动化修复建议器
 */
class AutoRepairSuggester
{
public:
    struct RepairSuggestion {
        QString issue;
        QString suggestion;
        QString action;
        int priority{1}; // 1=低, 2=中, 3=高, 4=紧急
        bool autoApplicable{false};
    };
    
    static QList<RepairSuggestion> analyzeProblem(const QString& component, 
                                                  const QString& error,
                                                  const StackTrace& stackTrace);
    
    static QList<RepairSuggestion> analyzePerformanceIssue(const QJsonObject& metrics);
    static QList<RepairSuggestion> analyzeMemoryIssue(double memoryUsage);
    static QList<RepairSuggestion> analyzeThreadIssue(const QJsonObject& threadStats);
    static QList<RepairSuggestion> analyzeDatabaseIssue(const QJsonObject& dbStats);
    
private:
    static RepairSuggestion createSuggestion(const QString& issue, const QString& suggestion,
                                            const QString& action, int priority, bool autoApplicable);
};

#endif // ENHANCED_CHAT_SERVER_H