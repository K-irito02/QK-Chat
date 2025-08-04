#ifndef MONITORMANAGER_H
#define MONITORMANAGER_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QMutex>
#include <QQueue>
#include <QDateTime>
#include <QProcess>
#include <QSysInfo>

/**
 * @brief 客户端监控管理器
 * 
 * 负责监控客户端运行状态，包括：
 * - 系统资源监控
 * - 网络连接监控
 * - 性能指标监控
 * - 错误统计监控
 */
class MonitorManager : public QObject
{
    Q_OBJECT
    
public:
    enum MetricType {
        System,         // 系统指标
        Network,        // 网络指标
        Performance,    // 性能指标
        Error,          // 错误指标
        Memory,         // 内存指标
        CPU            // CPU指标
    };
    Q_ENUM(MetricType)
    
    struct MetricData {
        QString name;
        double value;
        QString unit;
        QDateTime timestamp;
        MetricType type;
    };
    
    struct SystemInfo {
        QString osName;
        QString osVersion;
        QString cpuArchitecture;
        qint64 totalMemory;
        qint64 availableMemory;
        int cpuCount;
        QString hostName;
        QString userName;
    };
    
    static MonitorManager* instance();
    
    // 监控控制
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    
    // 指标记录
    void recordMetric(const QString &name, double value, const QString &unit = "", MetricType type = Performance);
    void recordEvent(const QString &event, const QString &category = "");
    void recordError(const QString &error, const QString &component = "");
    
    // 系统信息
    SystemInfo getSystemInfo() const;
    double getCpuUsage() const;
    qint64 getMemoryUsage() const;
    qint64 getAvailableMemory() const;
    
    // 网络监控
    void setNetworkStatus(bool connected);
    bool isNetworkConnected() const;
    void recordNetworkLatency(qint64 latency);
    qint64 getAverageLatency() const;
    
    // 性能监控
    void recordResponseTime(const QString &operation, qint64 timeMs);
    qint64 getAverageResponseTime(const QString &operation) const;
    
    // 统计信息
    QHash<QString, double> getMetrics() const;
    QHash<QString, int> getEventCounts() const;
    QHash<QString, int> getErrorCounts() const;
    
    // 报告生成
    QString generateSystemReport() const;
    QString generatePerformanceReport() const;
    QString generateErrorReport() const;
    
    // 配置
    void setMonitoringInterval(int intervalMs);
    void setMaxHistorySize(int size);
    void enableMetricType(MetricType type, bool enabled = true);
    
signals:
    void metricUpdated(const QString &name, double value, const QString &unit);
    void systemInfoUpdated(const SystemInfo &info);
    void networkStatusChanged(bool connected);
    void errorThresholdExceeded(const QString &error, int count);
    void performanceAlert(const QString &operation, qint64 responseTime);
    
private slots:
    void updateSystemMetrics();
    void updateNetworkMetrics();
    void updatePerformanceMetrics();
    void checkThresholds();
    void saveMetrics();
    
private:
    explicit MonitorManager(QObject *parent = nullptr);
    ~MonitorManager();
    
    void initializeMonitoring();
    void setupTimers();
    void loadSystemInfo();
    void calculateCpuUsage();
    void calculateMemoryUsage();
    void checkErrorThresholds();
    void checkPerformanceThresholds();
    
    static MonitorManager* _instance;
    static QMutex _instanceMutex;
    
    QTimer *_monitoringTimer;
    QTimer *_saveTimer;
    QTimer *_thresholdTimer;
    
    bool _isMonitoring;
    int _monitoringInterval;
    int _maxHistorySize;
    
    // 系统信息
    SystemInfo _systemInfo;
    double _cpuUsage;
    qint64 _memoryUsage;
    qint64 _availableMemory;
    
    // 网络状态
    bool _networkConnected;
    QQueue<qint64> _networkLatencies;
    qint64 _totalLatency;
    int _latencyCount;
    
    // 性能指标
    QHash<QString, QQueue<qint64>> _responseTimes;
    QHash<QString, qint64> _totalResponseTimes;
    QHash<QString, int> _responseCounts;
    
    // 统计数据
    QHash<QString, QQueue<MetricData>> _metrics;
    QHash<QString, int> _eventCounts;
    QHash<QString, int> _errorCounts;
    
    // 配置
    QHash<MetricType, bool> _enabledMetrics;
    
    QMutex _dataMutex;
    
    static const int DEFAULT_MONITORING_INTERVAL = 5000; // 5秒
    static const int DEFAULT_SAVE_INTERVAL = 60000; // 1分钟
    static const int DEFAULT_THRESHOLD_CHECK_INTERVAL = 10000; // 10秒
    static const int DEFAULT_MAX_HISTORY_SIZE = 100;
    static const int ERROR_THRESHOLD = 10;
    static const int PERFORMANCE_THRESHOLD = 1000; // 1秒
};

#endif // MONITORMANAGER_H 