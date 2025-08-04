#ifndef CACHESTRATEGYMANAGER_H
#define CACHESTRATEGYMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <QJsonObject>
#include <QHash>
#include <memory>
#include <functional>
#include "MultiLevelCache.h"

Q_DECLARE_LOGGING_CATEGORY(cacheStrategyManager)

/**
 * @brief 缓存访问模式
 */
enum class AccessPattern {
    Sequential = 0,     // 顺序访问
    Random = 1,         // 随机访问
    Temporal = 2,       // 时间局部性
    Spatial = 3,        // 空间局部性
    Burst = 4,          // 突发访问
    Periodic = 5        // 周期性访问
};

/**
 * @brief 缓存预测模型
 */
enum class PredictionModel {
    LRU_K = 0,         // LRU-K算法
    ARC = 1,           // 自适应替换缓存
    LIRS = 2,          // 低干扰度回收集
    CLOCK_Pro = 3,     // CLOCK-Pro算法
    ML_Based = 4       // 机器学习模型
};

/**
 * @brief 访问统计信息
 */
struct AccessStats {
    QDateTime timestamp{};
    QString key{};
    QString category{};
    CacheLevel level{CacheLevel::L1_Memory};
    bool hit{false};
    qint64 latency{0};
    int frequency{0};
    double score{0.0};
    
    AccessStats() : hit(false), latency(0), frequency(0), score(0.0) {}
};

/**
 * @brief 缓存策略配置
 */
struct StrategyConfig {
    PredictionModel model;
    AccessPattern expectedPattern;
    
    // 预测参数
    int historyWindow;
    double learningRate;
    int predictionHorizon;
    
    // 自适应参数
    bool enableAdaptive;
    int adaptiveInterval;
    double adaptiveThreshold;
    
    // 预加载参数
    bool enablePrefetch;
    int prefetchDistance;
    double prefetchConfidence;
    
    // 压缩和加密
    bool enableCompression;
    bool enableEncryption;
    QString encryptionKey;
    
    // 默认构造函数
    StrategyConfig()
        : model(PredictionModel::ARC)
        , expectedPattern(AccessPattern::Random)
        , historyWindow(1000)
        , learningRate(0.1)
        , predictionHorizon(100)
        , enableAdaptive(true)
        , adaptiveInterval(300)
        , adaptiveThreshold(0.1)
        , enablePrefetch(true)
        , prefetchDistance(5)
        , prefetchConfidence(0.8)
        , enableCompression(false)
        , enableEncryption(false)
        , encryptionKey()
    {}
};

/**
 * @brief 智能缓存策略管理器
 * 
 * 功能：
 * - 访问模式分析和预测
 * - 自适应缓存策略调整
 * - 智能预加载和预取
 * - 缓存性能优化建议
 * - 机器学习模型训练
 */
class CacheStrategyManager : public QObject
{
    Q_OBJECT

public:
    struct PerformanceMetrics {
        double hitRate{0.0};
        double l1HitRate{0.0};
        double l2HitRate{0.0};
        double l3HitRate{0.0};
        qint64 averageLatency{0};
        qint64 maxLatency{0};
        int totalRequests{0};
        int evictions{0};
        int promotions{0};
        QDateTime lastUpdate{};
        
        PerformanceMetrics() 
            : hitRate(0), l1HitRate(0), l2HitRate(0), l3HitRate(0)
            , averageLatency(0), maxLatency(0), totalRequests(0)
            , evictions(0), promotions(0), lastUpdate(QDateTime::currentDateTime()) {}
    };

    explicit CacheStrategyManager(MultiLevelCache* cache, QObject *parent = nullptr);
    ~CacheStrategyManager();

    // 初始化和配置
    bool initialize(const StrategyConfig& config = StrategyConfig());
    void shutdown();
    bool isEnabled() const;
    
    // 策略配置
    void updateConfig(const StrategyConfig& config);
    StrategyConfig getCurrentConfig() const;
    void setModel(PredictionModel model);
    void setAccessPattern(AccessPattern pattern);
    
    // 访问记录
    void recordAccess(const QString& key, const QString& category, 
                     CacheLevel level, bool hit, qint64 latency = 0);
    void recordBatchAccess(const QList<AccessStats>& accesses);
    
    // 预测和建议
    QStringList predictNextAccess(int count = 10) const;
    QStringList recommendPrefetch(const QString& key, int count = 5) const;
    CacheStrategy recommendStrategy(const QString& category = QString()) const;
    QJsonObject getOptimizationSuggestions() const;
    
    // 自适应优化
    void enableAdaptiveOptimization(bool enabled);
    void triggerOptimization();
    void analyzeAccessPatterns();
    
    // 预加载管理
    void enablePrefetching(bool enabled);
    void prefetchData(const QStringList& keys);
    void schedulePrefetch(const QString& key, std::function<QVariant()> loader);
    
    // 性能分析
    PerformanceMetrics getPerformanceMetrics() const;
    QJsonObject getDetailedAnalysis() const;
    QStringList getPerformanceRecommendations() const;
    
    // 机器学习
    void trainModel();
    void updateModel(const QList<AccessStats>& trainingData);
    double predictAccessProbability(const QString& key) const;
    
    // 数据导出
    QJsonObject exportAccessHistory() const;
    QJsonObject exportModelData() const;
    bool importModelData(const QJsonObject& data);

signals:
    void accessPatternChanged(AccessPattern oldPattern, AccessPattern newPattern);
    void optimizationCompleted(const QJsonObject& results);
    void prefetchCompleted(const QStringList& keys, int successCount);
    void modelTrained(double accuracy);
    void performanceAlert(const QString& message);

private slots:
    void performAdaptiveOptimization();
    void analyzePeriodically();
    void cleanupOldData();

private:
    // 核心组件
    MultiLevelCache* m_cache;
    StrategyConfig m_config;
    QAtomicInt m_enabled{0};
    
    // 访问历史
    QList<AccessStats> m_accessHistory;
    QHash<QString, QList<AccessStats>> m_keyHistory;
    QHash<QString, QList<AccessStats>> m_categoryHistory;
    mutable QMutex m_historyMutex;
    
    // 预测模型
    QHash<QString, double> m_accessProbabilities;
    QHash<QString, AccessPattern> m_keyPatterns;
    QHash<QString, double> m_modelWeights;
    
    // 性能指标
    PerformanceMetrics m_metrics;
    QList<PerformanceMetrics> m_metricsHistory;
    
    // 定时器
    QTimer* m_optimizationTimer;
    QTimer* m_analysisTimer;
    QTimer* m_cleanupTimer;
    
    // 内部方法
    AccessPattern detectAccessPattern(const QList<AccessStats>& history) const;
    PredictionModel selectBestModel(const QString& category = QString()) const;
    
    // 模式分析
    bool isSequentialPattern(const QList<AccessStats>& history) const;
    bool isTemporalPattern(const QList<AccessStats>& history) const;
    bool isBurstPattern(const QList<AccessStats>& history) const;
    bool isPeriodicPattern(const QList<AccessStats>& history) const;
    
    // 预测算法
    QStringList predictLRU_K(const QString& key, int k = 2, int count = 10) const;
    QStringList predictARC(const QString& key, int count = 10) const;
    QStringList predictLIRS(const QString& key, int count = 10) const;
    QStringList predictML(const QString& key, int count = 10) const;
    
    // 机器学习
    void trainLRU_K();
    void trainARC();
    void trainMLModel();
    double calculateModelAccuracy() const;
    
    // 优化建议
    QStringList generateCacheOptimizations() const;
    QStringList generatePrefetchOptimizations() const;
    QStringList generateStrategyOptimizations() const;
    
    // 数据清理
    void cleanupAccessHistory();
    void cleanupModelData();
    
    // 工具方法
    double calculateEntropy(const QList<AccessStats>& history) const;
    double calculateCorrelation(const QList<AccessStats>& a, const QList<AccessStats>& b) const;
    QStringList extractSequentialKeys(const QList<AccessStats>& history) const;
    QStringList identifyHotKeys() const;
    QStringList identifyColdKeys() const;
    
    void logStrategyEvent(const QString& event, const QString& details = QString()) const;
};

#endif // CACHESTRATEGYMANAGER_H
