#ifndef PERFORMANCETEST_H
#define PERFORMANCETEST_H

#include <QObject>
#include <QTest>
#include <QTimer>
#include <QElapsedTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QLoggingCategory>
#include <QAtomicInt>
#include <QMutex>
#include <QThread>
#include <memory>
#include <vector>

Q_DECLARE_LOGGING_CATEGORY(performanceTest)

/**
 * @brief 性能测试结果
 */
struct PerformanceResult {
    QString testName;
    QString category;
    QDateTime startTime;
    QDateTime endTime;
    qint64 duration;            // 毫秒
    int iterations;
    double averageTime;         // 毫秒/次
    double minTime;             // 毫秒
    double maxTime;             // 毫秒
    double throughput;          // 次/秒
    qint64 memoryUsed;          // 字节
    bool success;
    QString errorMessage;
    QJsonObject metadata;
    
    PerformanceResult() 
        : duration(0), iterations(0), averageTime(0), minTime(0), maxTime(0)
        , throughput(0), memoryUsed(0), success(false) {}
};

/**
 * @brief 并发测试配置
 */
struct ConcurrencyConfig {
    int threadCount;            // 线程数量
    int connectionsPerThread;   // 每线程连接数
    int messagesPerConnection;  // 每连接消息数
    int messageSize;            // 消息大小（字节）
    int testDuration;           // 测试持续时间（秒）
    bool rampUp;                // 是否渐进增加负载
    int rampUpTime;             // 渐进时间（秒）
    
    ConcurrencyConfig()
        : threadCount(10), connectionsPerThread(10), messagesPerConnection(100)
        , messageSize(1024), testDuration(60), rampUp(true), rampUpTime(10) {}
};

/**
 * @brief 性能测试框架
 */
class PerformanceTest : public QObject
{
    Q_OBJECT

public:
    explicit PerformanceTest(QObject *parent = nullptr);
    ~PerformanceTest();

    // 基础性能测试
    PerformanceResult testConnectionPerformance(int connectionCount = 1000);
    PerformanceResult testMessageThroughput(int messageCount = 10000, int messageSize = 1024);
    PerformanceResult testDatabasePerformance(int queryCount = 1000);
    PerformanceResult testCachePerformance(int operationCount = 10000);
    
    // 并发性能测试
    PerformanceResult testConcurrentConnections(const ConcurrencyConfig& config);
    PerformanceResult testConcurrentMessages(const ConcurrencyConfig& config);
    PerformanceResult testConcurrentDatabase(const ConcurrencyConfig& config);
    
    // 压力测试
    PerformanceResult stressTestConnections(int maxConnections = 10000);
    PerformanceResult stressTestMessages(int duration = 300); // 5分钟
    PerformanceResult stressTestMemory(int duration = 600);   // 10分钟
    
    // 稳定性测试
    PerformanceResult stabilityTest(int duration = 3600);    // 1小时
    PerformanceResult memoryLeakTest(int duration = 1800);   // 30分钟
    PerformanceResult longRunningTest(int duration = 86400); // 24小时
    
    // 基准测试
    PerformanceResult benchmarkThreadPool();
    PerformanceResult benchmarkLockFreeQueue();
    PerformanceResult benchmarkConcurrentMap();
    PerformanceResult benchmarkMessageEngine();
    PerformanceResult benchmarkDatabasePool();
    
    // 对比测试
    PerformanceResult compareWithOldVersion();
    PerformanceResult compareThreadingModels();
    PerformanceResult compareLockingStrategies();
    
    // 报告生成
    QJsonObject generateReport(const QList<PerformanceResult>& results);
    QString generateTextReport(const QList<PerformanceResult>& results);
    void saveReport(const QList<PerformanceResult>& results, const QString& filename);
    
    // 测试套件
    QList<PerformanceResult> runBasicTestSuite();
    QList<PerformanceResult> runConcurrencyTestSuite();
    QList<PerformanceResult> runStressTestSuite();
    QList<PerformanceResult> runStabilityTestSuite();
    QList<PerformanceResult> runBenchmarkSuite();
    QList<PerformanceResult> runFullTestSuite();

signals:
    void testStarted(const QString& testName);
    void testCompleted(const PerformanceResult& result);
    void testProgress(const QString& testName, int percentage);
    void testFailed(const QString& testName, const QString& error);

private slots:
    void onTestProgress();

private:
    // 测试工具类
    class TestClient;
    class TestWorker;
    
    // 辅助方法
    PerformanceResult runTest(const QString& testName, const QString& category,
                             std::function<bool()> testFunction, int iterations = 1);
    
    void setupTestEnvironment();
    void cleanupTestEnvironment();
    
    qint64 getCurrentMemoryUsage() const;
    void startMemoryMonitoring();
    void stopMemoryMonitoring();
    
    // 具体测试实现
    bool performConnectionTest(int connectionCount);
    bool performMessageTest(int messageCount, int messageSize);
    bool performDatabaseTest(int queryCount);
    bool performCacheTest(int operationCount);
    
    // 并发测试实现
    bool performConcurrentConnectionTest(const ConcurrencyConfig& config);
    bool performConcurrentMessageTest(const ConcurrencyConfig& config);
    bool performConcurrentDatabaseTest(const ConcurrencyConfig& config);
    
    // 压力测试实现
    bool performStressConnectionTest(int maxConnections);
    bool performStressMessageTest(int duration);
    bool performStressMemoryTest(int duration);
    
    // 基准测试实现
    bool benchmarkThreadPoolPerformance();
    bool benchmarkQueuePerformance();
    bool benchmarkMapPerformance();
    bool benchmarkEnginePerformance();
    bool benchmarkPoolPerformance();
    
    // 统计计算
    void calculateStatistics(QList<double>& times, PerformanceResult& result);
    double calculateThroughput(int operations, qint64 duration);
    
    // 测试数据生成
    QByteArray generateTestMessage(int size);
    QList<QByteArray> generateTestMessages(int count, int size);
    
    // 结果验证
    bool validateTestResult(const PerformanceResult& result);
    void logTestResult(const PerformanceResult& result);
    
    // 成员变量
    QTimer* m_progressTimer;
    QElapsedTimer m_testTimer;
    QAtomicInt m_currentProgress{0};
    QString m_currentTestName;
    
    // 内存监控
    QTimer* m_memoryTimer;
    qint64 m_initialMemory;
    qint64 m_peakMemory;
    QMutex m_memoryMutex;
    
    // 测试配置
    QString m_serverHost;
    int m_serverPort;
    bool m_testEnvironmentReady;
    
    // 测试客户端池
    std::vector<std::unique_ptr<TestClient>> m_testClients;
    
    // 统计数据
    QAtomicInt m_totalTests{0};
    QAtomicInt m_passedTests{0};
    QAtomicInt m_failedTests{0};
};

/**
 * @brief 测试客户端
 */
class PerformanceTest::TestClient : public QObject
{
    Q_OBJECT

public:
    explicit TestClient(const QString& host, int port, QObject* parent = nullptr);
    ~TestClient();
    
    bool connectToServer();
    void disconnectFromServer();
    bool isConnected() const;
    
    bool sendMessage(const QByteArray& message);
    QByteArray receiveMessage();
    
    bool authenticate(const QString& username, const QString& password);
    bool sendChatMessage(const QString& message, qint64 toUserId);
    
    // 统计信息
    int getMessagesSent() const;
    int getMessagesReceived() const;
    qint64 getTotalBytesSent() const;
    qint64 getTotalBytesReceived() const;

signals:
    void connected();
    void disconnected();
    void messageReceived(const QByteArray& message);
    void error(const QString& error);

private:
    class Private;
    std::unique_ptr<Private> d;
};

/**
 * @brief 测试工作线程
 */
class PerformanceTest::TestWorker : public QThread
{
    Q_OBJECT

public:
    explicit TestWorker(const ConcurrencyConfig& config, QObject* parent = nullptr);
    ~TestWorker();
    
    void setTestFunction(std::function<bool()> testFunction);
    PerformanceResult getResult() const;

signals:
    void progress(int percentage);
    void finished(bool success);

protected:
    void run() override;

private:
    ConcurrencyConfig m_config;
    std::function<bool()> m_testFunction;
    PerformanceResult m_result;
    QAtomicInt m_progress{0};
};

#endif // PERFORMANCETEST_H
