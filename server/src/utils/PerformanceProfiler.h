#ifndef PERFORMANCEPROFILER_H
#define PERFORMANCEPROFILER_H

#include <QObject>
#include <QElapsedTimer>
#include <QDateTime>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <QMutex>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>
#include <chrono>

Q_DECLARE_LOGGING_CATEGORY(performanceProfiler)

/**
 * @brief 性能测量点
 */
struct ProfilePoint {
    QString name;
    QString category;
    QDateTime startTime;
    QDateTime endTime;
    qint64 duration;        // 微秒
    qint64 memoryBefore;    // 字节
    qint64 memoryAfter;     // 字节
    QHash<QString, QVariant> metadata;
    
    ProfilePoint() : duration(0), memoryBefore(0), memoryAfter(0) {}
    
    bool isValid() const {
        return !name.isEmpty() && startTime.isValid() && endTime.isValid();
    }
    
    double durationMs() const {
        return duration / 1000.0;
    }
    
    qint64 memoryDelta() const {
        return memoryAfter - memoryBefore;
    }
};

/**
 * @brief 性能统计信息
 */
struct ProfileStats {
    QString name;
    int callCount;
    qint64 totalDuration;       // 微秒
    qint64 minDuration;         // 微秒
    qint64 maxDuration;         // 微秒
    qint64 avgDuration;         // 微秒
    qint64 totalMemoryDelta;    // 字节
    QDateTime firstCall;
    QDateTime lastCall;
    
    ProfileStats() : callCount(0), totalDuration(0), minDuration(LLONG_MAX), 
                    maxDuration(0), avgDuration(0), totalMemoryDelta(0) {}
    
    void addMeasurement(const ProfilePoint& point) {
        callCount++;
        totalDuration += point.duration;
        totalMemoryDelta += point.memoryDelta();
        
        if (point.duration < minDuration) {
            minDuration = point.duration;
        }
        if (point.duration > maxDuration) {
            maxDuration = point.duration;
        }
        
        avgDuration = totalDuration / callCount;
        
        if (!firstCall.isValid() || point.startTime < firstCall) {
            firstCall = point.startTime;
        }
        if (!lastCall.isValid() || point.endTime > lastCall) {
            lastCall = point.endTime;
        }
    }
    
    double totalDurationMs() const { return totalDuration / 1000.0; }
    double minDurationMs() const { return minDuration / 1000.0; }
    double maxDurationMs() const { return maxDuration / 1000.0; }
    double avgDurationMs() const { return avgDuration / 1000.0; }
};

/**
 * @brief 性能分析器
 * 
 * 功能：
 * - 代码执行时间测量
 * - 内存使用分析
 * - 调用频率统计
 * - 性能瓶颈识别
 * - 热点函数分析
 */
class PerformanceProfiler : public QObject
{
    Q_OBJECT

public:
    enum ProfilingMode {
        Disabled = 0,
        Basic = 1,          // 基本时间测量
        Detailed = 2,       // 详细分析（包括内存）
        Sampling = 3        // 采样分析（降低开销）
    };

    struct ProfilerConfig {
        ProfilingMode mode;
        bool enableMemoryProfiling;
        bool enableCallStack;
        int maxProfilePoints;
        int samplingRate;           // 采样率（1-100）
        int reportInterval;         // 报告间隔（毫秒）
        QStringList enabledCategories;
        QStringList disabledFunctions;
        
        ProfilerConfig()
            : mode(Basic)
            , enableMemoryProfiling(false)
            , enableCallStack(false)
            , maxProfilePoints(10000)
            , samplingRate(100)
            , reportInterval(60000)
        {}
    };

    static PerformanceProfiler* instance();
    
    // 初始化和配置
    bool initialize(const ProfilerConfig& config = ProfilerConfig());
    void shutdown();
    bool isEnabled() const;
    
    // 配置管理
    void setProfilingMode(ProfilingMode mode);
    void enableCategory(const QString& category);
    void disableCategory(const QString& category);
    void enableFunction(const QString& function);
    void disableFunction(const QString& function);
    
    // 性能测量
    void startProfiling(const QString& name, const QString& category = QString());
    void endProfiling(const QString& name);
    void profileFunction(const QString& name, std::function<void()> func, const QString& category = QString());
    
    // 内存分析
    void recordMemoryUsage(const QString& name, qint64 bytes);
    void recordMemoryAllocation(const QString& name, qint64 bytes);
    void recordMemoryDeallocation(const QString& name, qint64 bytes);
    
    // 统计信息
    ProfileStats getFunctionStats(const QString& name) const;
    QList<ProfileStats> getAllStats() const;
    QList<ProfileStats> getCategoryStats(const QString& category) const;
    QList<ProfileStats> getTopFunctions(int count = 10) const;
    QList<ProfileStats> getSlowestFunctions(int count = 10) const;
    
    // 报告生成
    QJsonObject generateReport() const;
    QJsonObject generateSummaryReport() const;
    QString generateTextReport() const;
    void saveReport(const QString& filename) const;
    
    // 数据管理
    void clearData();
    void clearFunction(const QString& name);
    void clearCategory(const QString& category);
    
    // 实时监控
    void enableRealTimeMonitoring(bool enabled);
    QStringList getActiveProfiles() const;
    int getActiveProfileCount() const;

signals:
    void profileCompleted(const ProfilePoint& point);
    void slowFunctionDetected(const QString& name, qint64 duration);
    void memoryLeakDetected(const QString& name, qint64 leakSize);
    void performanceAlert(const QString& message);

private slots:
    void generatePeriodicReport();
    void checkPerformanceThresholds();

private:
    explicit PerformanceProfiler(QObject *parent = nullptr);
    ~PerformanceProfiler();
    
    static PerformanceProfiler* s_instance;
    static QMutex s_instanceMutex;
    
    // 配置
    ProfilerConfig m_config;
    QAtomicInt m_enabled{0};
    
    // 数据存储
    QMutex m_dataMutex;
    QHash<QString, ProfilePoint> m_activeProfiles;
    QHash<QString, ProfileStats> m_functionStats;
    QList<ProfilePoint> m_profileHistory;
    
    // 内存跟踪
    QHash<QString, qint64> m_memoryUsage;
    QHash<QString, qint64> m_memoryAllocations;
    
    // 定时器
    QTimer* m_reportTimer;
    QTimer* m_thresholdTimer;
    
    // 内部方法
    bool shouldProfile(const QString& name, const QString& category) const;
    bool shouldSample() const;
    qint64 getCurrentMemoryUsage() const;
    void addProfilePoint(const ProfilePoint& point);
    void updateFunctionStats(const ProfilePoint& point);
    void checkMemoryLeaks();
    void checkSlowFunctions();
    void cleanupOldData();
    
    // 报告生成辅助
    QJsonObject pointToJson(const ProfilePoint& point) const;
    QJsonObject statsToJson(const ProfileStats& stats) const;
    QString formatDuration(qint64 microseconds) const;
    QString formatMemory(qint64 bytes) const;
};

/**
 * @brief RAII性能测量类
 */
class ProfileScope
{
public:
    explicit ProfileScope(const QString& name, const QString& category = QString())
        : m_name(name), m_category(category)
    {
        if (PerformanceProfiler::instance()->isEnabled()) {
            PerformanceProfiler::instance()->startProfiling(m_name, m_category);
        }
    }
    
    ~ProfileScope()
    {
        if (PerformanceProfiler::instance()->isEnabled()) {
            PerformanceProfiler::instance()->endProfiling(m_name);
        }
    }

private:
    QString m_name;
    QString m_category;
};

// 便利宏定义
#define PROFILE_FUNCTION() ProfileScope __prof(__FUNCTION__)
#define PROFILE_SCOPE(name) ProfileScope __prof(name)
#define PROFILE_CATEGORY(name, category) ProfileScope __prof(name, category)

#ifdef QT_DEBUG
    #define PROFILE_DEBUG(name) ProfileScope __prof(name)
#else
    #define PROFILE_DEBUG(name)
#endif

#endif // PERFORMANCEPROFILER_H
