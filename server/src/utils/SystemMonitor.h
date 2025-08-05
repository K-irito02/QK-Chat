#ifndef SYSTEMMONITOR_H
#define SYSTEMMONITOR_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <QMutex>
#include <QHash>
#include <QJsonObject>
#include <memory>
#include "../utils/LockFreeStructures.h"

Q_DECLARE_LOGGING_CATEGORY(systemMonitor)

/**
 * @brief 性能指标类型
 */
enum class MetricType {
    Counter = 0,        // 计数器（累加）
    Gauge = 1,          // 仪表（当前值）
    Histogram = 2,      // 直方图（分布）
    Timer = 3           // 计时器（耗时）
};

/**
 * @brief 性能指标数据
 */
struct MetricData {
    MetricType type;
    QString name;
    QString category;
    QVariant value;
    QDateTime timestamp;
    QHash<QString, QString> tags;
    
    MetricData() : type(MetricType::Counter), timestamp(QDateTime::currentDateTime()) {}
    MetricData(MetricType t, const QString& n, const QVariant& v) 
        : type(t), name(n), value(v), timestamp(QDateTime::currentDateTime()) {}
};

/**
 * @brief 系统健康状态
 */
enum class HealthStatus {
    Healthy = 0,
    Warning = 1,
    Critical = 2,
    Unknown = 3
};

/**
 * @brief 健康检查结果
 */
struct HealthCheck {
    QString component;
    HealthStatus status;
    QString message;
    QDateTime timestamp;
    QJsonObject details;
    
    HealthCheck() : status(HealthStatus::Unknown), timestamp(QDateTime::currentDateTime()) {}
    HealthCheck(const QString& comp, HealthStatus stat, const QString& msg)
        : component(comp), status(stat), message(msg), timestamp(QDateTime::currentDateTime()) {}
};

/**
 * @brief 性能警报
 */
struct PerformanceAlert {
    QString alertId;
    QString component;
    QString metric;
    QString condition;
    QVariant threshold;
    QVariant currentValue;
    QDateTime triggeredAt;
    bool resolved;
    
    PerformanceAlert() : resolved(false), triggeredAt(QDateTime::currentDateTime()) {}
};

/**
 * @brief 系统监控器
 * 
 * 功能：
 * - 实时性能指标收集
 * - 系统健康检查
 * - 性能警报管理
 * - 死锁检测
 * - 自动恢复机制
 */
class SystemMonitor : public QObject
{
    Q_OBJECT

public:
    struct MonitorConfig {
        int metricsCollectionInterval;  // 指标收集间隔（毫秒）
        int healthCheckInterval;        // 健康检查间隔（毫秒）
        int alertCheckInterval;         // 警报检查间隔（毫秒）
        int deadlockCheckInterval;      // 死锁检查间隔（毫秒）
        
        // 阈值配置
        double cpuThreshold;            // CPU使用率阈值
        double memoryThreshold;         // 内存使用率阈值
        int connectionThreshold;        // 连接数阈值
        int queueSizeThreshold;         // 队列大小阈值
        int responseTimeThreshold;      // 响应时间阈值（毫秒）
        
        // 自动恢复配置
        bool enableAutoRecovery;        // 启用自动恢复
        int maxRecoveryAttempts;        // 最大恢复尝试次数
        int recoveryInterval;           // 恢复间隔（毫秒）
        
        MonitorConfig()
            : metricsCollectionInterval(1000)
            , healthCheckInterval(5000)
            , alertCheckInterval(2000)
            , deadlockCheckInterval(10000)
            , cpuThreshold(80.0)
            , memoryThreshold(85.0)
            , connectionThreshold(1000)
            , queueSizeThreshold(10000)
            , responseTimeThreshold(1000)
            , enableAutoRecovery(true)
            , maxRecoveryAttempts(3)
            , recoveryInterval(30000)
        {}
    };

    struct SystemMetrics {
        // 系统资源
        double cpuUsage;
        double memoryUsage;
        qint64 diskUsage;
        qint64 networkIn;
        qint64 networkOut;
        
        // 应用指标
        int activeConnections;
        int totalConnections;
        int queueSize;
        int threadPoolUsage;
        
        // 性能指标
        int averageResponseTime;
        int maxResponseTime;
        int throughput;
        double errorRate;
        
        // 数据库指标
        int dbConnections;
        int dbQueueSize;
        int dbResponseTime;
        
        QDateTime timestamp;
        
        SystemMetrics() : timestamp(QDateTime::currentDateTime()) {
            cpuUsage = memoryUsage = diskUsage = 0;
            networkIn = networkOut = 0;
            activeConnections = totalConnections = queueSize = threadPoolUsage = 0;
            averageResponseTime = maxResponseTime = throughput = 0;
            errorRate = 0.0;
            dbConnections = dbQueueSize = dbResponseTime = 0;
        }
    };

    static SystemMonitor* instance();
    
    // 初始化和配置
    bool initialize(const MonitorConfig& config = MonitorConfig());
    void shutdown();
    bool isRunning() const;
    
    // 指标收集
    void recordMetric(const QString& name, const QVariant& value, MetricType type = MetricType::Gauge);
    void recordMetric(const QString& name, const QVariant& value, const QString& category, 
                     const QHash<QString, QString>& tags = QHash<QString, QString>());
    void incrementCounter(const QString& name, int value = 1);
    void recordTimer(const QString& name, int milliseconds);
    void recordHistogram(const QString& name, double value);
    
    // 批量指标
    void recordMetrics(const QList<MetricData>& metrics);
    
    // 健康检查
    void registerHealthCheck(const QString& component, std::function<HealthCheck()> checkFunction);
    void unregisterHealthCheck(const QString& component);
    HealthStatus getComponentHealth(const QString& component) const;
    HealthStatus getOverallHealth() const;
    QList<HealthCheck> getAllHealthChecks() const;
    
    // 警报管理
    void registerAlert(const QString& alertId, const QString& component, const QString& metric,
                      const QString& condition, const QVariant& threshold);
    void unregisterAlert(const QString& alertId);
    QList<PerformanceAlert> getActiveAlerts() const;
    QList<PerformanceAlert> getResolvedAlerts() const;
    
    // 系统指标获取
    SystemMetrics getCurrentMetrics() const;
    QList<MetricData> getMetricHistory(const QString& name, const QDateTime& since = QDateTime()) const;
    QJsonObject getMetricsSnapshot() const;
    
    // 死锁检测
    void enableDeadlockDetection(bool enabled);
    bool isDeadlockDetected() const;
    QStringList getDeadlockReport() const;
    
    // 自动恢复
    void enableAutoRecovery(bool enabled);
    void triggerRecovery(const QString& component, const QString& reason);
    int getRecoveryAttempts(const QString& component) const;

signals:
    void metricRecorded(const MetricData& metric);
    void healthStatusChanged(const QString& component, HealthStatus oldStatus, HealthStatus newStatus);
    void alertTriggered(const PerformanceAlert& alert);
    void alertResolved(const PerformanceAlert& alert);
    void deadlockDetected(const QStringList& report);
    void recoveryTriggered(const QString& component, const QString& reason);
    void recoveryCompleted(const QString& component, bool success);
    void systemOverloaded();
    void systemRecovered();

private slots:
    void collectMetrics();
    void performHealthChecks();
    void checkAlerts();
    void detectDeadlocks();
    void performRecovery();

private:
    explicit SystemMonitor(QObject *parent = nullptr);
    ~SystemMonitor();
    
    static SystemMonitor* s_instance;
    static QMutex s_instanceMutex;
    
    // 配置
    MonitorConfig m_config;
    QAtomicInt m_running{0};
    
    // 定时器
    QTimer* m_metricsTimer;
    QTimer* m_healthTimer;
    QTimer* m_alertTimer;
    QTimer* m_deadlockTimer;
    QTimer* m_recoveryTimer;
    
    // 数据存储
    ConcurrentMap<QString, MetricData> m_currentMetrics;
    ConcurrentMap<QString, QList<MetricData>> m_metricHistory;
    ConcurrentMap<QString, std::function<HealthCheck()>> m_healthChecks;
    ConcurrentMap<QString, HealthCheck> m_healthResults;
    ConcurrentMap<QString, PerformanceAlert> m_alerts;
    ConcurrentMap<QString, PerformanceAlert> m_activeAlerts;
    
    // 恢复管理
    ConcurrentMap<QString, int> m_recoveryAttempts;
    ConcurrentMap<QString, QDateTime> m_lastRecoveryTime;
    
    // 死锁检测
    QAtomicInt m_deadlockDetectionEnabled{0};
    QAtomicInt m_deadlockDetected{0};
    QStringList m_deadlockReport;
    mutable QMutex m_deadlockMutex;
    
    // 内部方法
    void collectSystemMetrics();
    void collectApplicationMetrics();
    void collectDatabaseMetrics();
    
    void checkMetricAlerts();
    void checkSystemAlerts();
    void resolveAlert(const QString& alertId);
    
    bool evaluateCondition(const QString& condition, const QVariant& value, const QVariant& threshold);
    
    void performSystemRecovery();
    void performComponentRecovery(const QString& component);
    bool canAttemptRecovery(const QString& component);
    
    void cleanupOldMetrics();
    void cleanupOldAlerts();
    
    // 系统资源获取
    double getCpuUsage() const;
    double getMemoryUsage() const;
    qint64 getDiskUsage() const;
    void getNetworkUsage(qint64& bytesIn, qint64& bytesOut) const;
    
    // 工具方法
    QString generateAlertId() const;
    void logMonitorEvent(const QString& event, const QString& details = QString()) const;
};

#endif // SYSTEMMONITOR_H
