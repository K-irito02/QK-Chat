#ifndef CACHESYSTEMTEST_H
#define CACHESYSTEMTEST_H

#include <QObject>
#include <QTest>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonObject>
#include <QLoggingCategory>
#include <memory>
#include "../src/cache/CacheManagerV2.h"
#include "../src/cache/MultiLevelCache.h"

Q_DECLARE_LOGGING_CATEGORY(cacheSystemTest)

/**
 * @brief 缓存系统集成测试
 * 
 * 测试内容：
 * - 多级缓存功能测试
 * - 性能基准测试
 * - 并发安全测试
 * - 策略管理测试
 * - 预加载功能测试
 */
class CacheSystemTest : public QObject
{
    Q_OBJECT

public:
    explicit CacheSystemTest(QObject *parent = nullptr);
    ~CacheSystemTest();

private slots:
    // 初始化和清理
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 基本功能测试
    void testBasicOperations();
    void testMultiLevelCaching();
    void testCacheEviction();
    void testTTLExpiration();
    void testCategoryManagement();

    // 兼容性测试
    void testLegacyAPI();
    void testUserCaching();
    void testGroupCaching();
    void testMessageCaching();
    void testSessionCaching();
    void testQueryCaching();

    // 高级功能测试
    void testTypedOperations();
    void testAsyncOperations();
    void testBatchOperations();
    void testPreloading();
    void testWarmup();

    // 策略测试
    void testLRUStrategy();
    void testLFUStrategy();
    void testAdaptiveStrategy();
    void testPrediction();

    // 性能测试
    void testPerformanceBasic();
    void testPerformanceConcurrent();
    void testPerformanceMemoryUsage();
    void testPerformanceLatency();

    // 并发测试
    void testConcurrentAccess();
    void testConcurrentEviction();
    void testThreadSafety();

    // 故障测试
    void testMemoryPressure();
    void testLevelFailure();
    void testRecovery();

    // 监控测试
    void testMetricsCollection();
    void testPerformanceAlerts();
    void testStatistics();

private:
    // 测试辅助方法
    void setupTestData();
    void verifyBasicFunctionality();
    void verifyPerformanceMetrics();
    void simulateHighLoad();
    void simulateMemoryPressure();
    
    // 性能测试辅助
    struct PerformanceResult {
        QString testName;
        int operations;
        qint64 totalTime;
        double averageTime;
        double throughput;
        qint64 memoryUsed;
        bool success;
    };
    
    PerformanceResult runPerformanceTest(const QString& testName, 
                                       std::function<void()> testFunction,
                                       int iterations = 1000);
    
    // 并发测试辅助
    void runConcurrentTest(const QString& testName,
                          std::function<void()> testFunction,
                          int threadCount = 10,
                          int operationsPerThread = 100);
    
    // 数据生成
    QVariantMap generateUserData(qint64 userId);
    QVariantMap generateGroupData(qint64 groupId);
    QVariantList generateMessages(qint64 chatId, int count = 10);
    QByteArray generateTestData(int size);
    
    // 验证方法
    void verifyDataIntegrity(const QString& key, const QVariant& expectedValue);
    void verifyLevelDistribution();
    void verifyCacheHitRates();
    void verifyMemoryUsage();
    
    // 成员变量
    std::unique_ptr<CacheManagerV2> m_cacheManager;
    CacheManagerV2::CacheConfig m_testConfig;
    
    // 测试数据
    QHash<QString, QVariant> m_testData;
    QStringList m_testKeys;
    
    // 性能基准
    struct PerformanceBenchmarks {
        double maxAcceptableLatency = 1.0;      // 1ms
        double minAcceptableHitRate = 0.8;      // 80%
        double maxMemoryOverhead = 0.2;         // 20%
        int minThroughput = 10000;              // 10K ops/sec
    } m_benchmarks;
    
    // 测试统计
    int m_totalTests = 0;
    int m_passedTests = 0;
    int m_failedTests = 0;
    QList<PerformanceResult> m_performanceResults;
};

#endif // CACHESYSTEMTEST_H
