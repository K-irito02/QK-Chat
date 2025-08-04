#ifndef CONNECTIONMONITOR_H
#define CONNECTIONMONITOR_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QQueue>
#include <QHash>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(connectionMonitor)

/**
 * @brief 连接质量监控系统
 * 
 * 监控和分析网络连接质量，包括：
 * - 连接成功率
 * - 网络延迟
 * - 丢包率
 * - 吞吐量
 * - 连接稳定性
 */
class ConnectionMonitor : public QObject
{
    Q_OBJECT
    
public:
    enum MetricType {
        ConnectionSuccess,  // 连接成功
        ConnectionFailure,  // 连接失败
        Latency,           // 延迟
        PacketLoss,        // 丢包
        Throughput,        // 吞吐量
        Reconnection,      // 重连
        Error              // 错误
    };
    Q_ENUM(MetricType)
    
    enum AlertLevel {
        Info,              // 信息
        Warning,           // 警告
        Critical           // 严重
    };
    Q_ENUM(AlertLevel)
    
    struct MetricData {
        MetricType type;
        QDateTime timestamp;
        double value;
        QString unit;
        QString description;
        QVariantMap metadata;
    };
    
    struct ConnectionStats {
        qint64 totalConnections;
        qint64 successfulConnections;
        qint64 failedConnections;
        double successRate;
        qint64 totalReconnections;
        qint64 averageLatency;
        qint64 minLatency;
        qint64 maxLatency;
        double packetLossRate;
        qint64 totalBytesTransferred;
        qint64 totalPacketsTransferred;
        QDateTime lastConnectionTime;
        QDateTime lastDisconnectionTime;
        qint64 totalUptime;
        qint64 totalDowntime;
    };
    
    struct Alert {
        AlertLevel level;
        QString message;
        QDateTime timestamp;
        MetricType relatedMetric;
        QVariantMap data;
    };
    
    explicit ConnectionMonitor(QObject *parent = nullptr);
    ~ConnectionMonitor();
    
    // 监控控制
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    
    // 指标记录
    void recordConnectionAttempt();
    void recordConnectionSuccess(qint64 latency = 0);
    void recordConnectionFailure(const QString &reason = "");
    void recordLatency(qint64 latency);
    void recordPacketLoss(int lostPackets, int totalPackets);
    void recordThroughput(qint64 bytes, qint64 timeMs);
    void recordReconnection(const QString &reason = "");
    void recordError(const QString &error, const QString &category = "");
    
    // 统计信息
    ConnectionStats getConnectionStats() const;
    QList<MetricData> getMetrics(MetricType type, int maxCount = 100) const;
    QList<MetricData> getRecentMetrics(int minutes = 60) const;
    QList<Alert> getAlerts(AlertLevel minLevel = Info, int maxCount = 50) const;
    
    // 质量评估
    enum QualityLevel {
        Excellent,         // 优秀
        Good,             // 良好
        Fair,             // 一般
        Poor,             // 较差
        Bad               // 很差
    };
    Q_ENUM(QualityLevel)
    
    QualityLevel getConnectionQuality() const;
    QString getQualityDescription() const;
    double getQualityScore() const; // 0-100分
    
    // 诊断功能
    QStringList diagnoseConnection() const;
    QStringList getRecommendations() const;
    bool isConnectionStable() const;
    
    // 配置
    void setMonitoringInterval(int intervalMs);
    void setMetricRetentionTime(int hours);
    void setAlertThresholds(const QHash<MetricType, double> &thresholds);
    void enableAlert(MetricType type, bool enabled = true);
    
signals:
    void metricRecorded(const MetricData &metric);
    void alertGenerated(const Alert &alert);
    void qualityChanged(QualityLevel quality);
    void connectionStabilityChanged(bool stable);
    void statisticsUpdated(const ConnectionStats &stats);
    
private slots:
    void onMonitoringTimer();
    void onCleanupTimer();
    
private:
    void addMetric(MetricType type, double value, const QString &unit = "", 
                   const QString &description = "", const QVariantMap &metadata = QVariantMap());
    void updateStatistics();
    void checkAlerts();
    void generateAlert(AlertLevel level, const QString &message, MetricType relatedMetric = Error, 
                      const QVariantMap &data = QVariantMap());
    void cleanupOldData();
    QualityLevel calculateQuality() const;
    double calculateQualityScore() const;
    
    bool _isMonitoring;
    QTimer *_monitoringTimer;
    QTimer *_cleanupTimer;
    
    // 指标数据
    QHash<MetricType, QQueue<MetricData>> _metrics;
    QQueue<Alert> _alerts;
    ConnectionStats _stats;
    QualityLevel _currentQuality;
    
    // 配置
    int _monitoringInterval;
    int _metricRetentionHours;
    QHash<MetricType, double> _alertThresholds;
    QHash<MetricType, bool> _alertEnabled;
    
    // 临时统计
    QDateTime _monitoringStartTime;
    QDateTime _lastConnectionAttempt;
    QDateTime _lastSuccessfulConnection;
    QDateTime _lastFailedConnection;
    
    // 默认配置
    static constexpr int DEFAULT_MONITORING_INTERVAL = 5000;  // 5秒
    static constexpr int DEFAULT_RETENTION_HOURS = 24;        // 24小时
    static constexpr int CLEANUP_INTERVAL = 300000;           // 5分钟
    static constexpr int MAX_METRICS_PER_TYPE = 1000;
    static constexpr int MAX_ALERTS = 100;
};

#endif // CONNECTIONMONITOR_H
