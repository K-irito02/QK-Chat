#ifndef DIAGNOSTICTOOL_H
#define DIAGNOSTICTOOL_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(diagnosticTool)

/**
 * @brief 连接诊断工具
 * 
 * 提供网络连接诊断功能，包括：
 * - 网络连通性测试
 * - DNS解析测试
 * - 端口连通性测试
 * - SSL证书验证测试
 * - 带宽测试
 */
class DiagnosticTool : public QObject
{
    Q_OBJECT
    
public:
    enum TestType {
        NetworkConnectivity,  // 网络连通性
        DnsResolution,       // DNS解析
        PortConnectivity,    // 端口连通性
        SslCertificate,      // SSL证书
        Bandwidth,           // 带宽测试
        Latency,            // 延迟测试
        PacketLoss          // 丢包测试
    };
    Q_ENUM(TestType)
    
    enum TestResult {
        Passed,             // 通过
        Failed,             // 失败
        Warning,            // 警告
        InProgress,         // 进行中
        NotRun              // 未运行
    };
    Q_ENUM(TestResult)
    
    struct TestInfo {
        TestType type;
        TestResult result;
        QString name;
        QString description;
        QString details;
        QDateTime startTime;
        QDateTime endTime;
        qint64 duration;
        QVariantMap data;
    };
    
    struct DiagnosticReport {
        QDateTime timestamp;
        QString summary;
        QList<TestInfo> tests;
        QStringList recommendations;
        QVariantMap systemInfo;
        bool overallSuccess;
    };
    
    explicit DiagnosticTool(QObject *parent = nullptr);
    ~DiagnosticTool();
    
    // 诊断控制
    void runFullDiagnostic(const QString &host, int port);
    void runSpecificTest(TestType type, const QString &host, int port);
    void cancelDiagnostic();
    bool isDiagnosticRunning() const;
    
    // 结果获取
    DiagnosticReport getLastReport() const;
    QList<TestInfo> getTestResults() const;
    TestInfo getTestResult(TestType type) const;
    
    // 配置
    void setTimeout(int timeoutMs);
    void setRetryCount(int retries);
    void setBandwidthTestSize(qint64 bytes);
    void setLatencyTestCount(int count);
    
signals:
    void diagnosticStarted();
    void diagnosticCompleted(const DiagnosticReport &report);
    void testStarted(TestType type);
    void testCompleted(TestType type, TestResult result);
    void progressUpdated(int percentage);
    
private slots:
    void onNetworkReply();
    void onTestTimeout();
    void onLatencyTestTimer();
    
private:
    void startNextTest();
    void completeCurrentTest(TestResult result, const QString &details = "", const QVariantMap &data = QVariantMap());
    void completeDiagnostic();
    
    // 具体测试方法
    void testNetworkConnectivity();
    void testDnsResolution();
    void testPortConnectivity();
    void testSslCertificate();
    void testBandwidth();
    void testLatency();
    void testPacketLoss();
    
    // 辅助方法
    QString getTestName(TestType type) const;
    QString getTestDescription(TestType type) const;
    QStringList generateRecommendations() const;
    QVariantMap collectSystemInfo() const;
    void updateProgress();
    
    QNetworkAccessManager *_networkManager;
    QTimer *_timeoutTimer;
    QTimer *_latencyTimer;
    
    // 诊断状态
    bool _diagnosticRunning;
    QString _targetHost;
    int _targetPort;
    QList<TestType> _testQueue;
    TestType _currentTest;
    int _currentTestIndex;
    
    // 测试结果
    QHash<TestType, TestInfo> _testResults;
    DiagnosticReport _lastReport;
    
    // 配置参数
    int _timeout;
    int _retryCount;
    qint64 _bandwidthTestSize;
    int _latencyTestCount;
    
    // 延迟测试状态
    int _latencyTestCurrent;
    QList<qint64> _latencyResults;
    QDateTime _latencyTestStart;
    
    // 默认配置
    static constexpr int DEFAULT_TIMEOUT = 10000;        // 10秒
    static constexpr int DEFAULT_RETRY_COUNT = 3;
    static constexpr qint64 DEFAULT_BANDWIDTH_SIZE = 1024 * 1024; // 1MB
    static constexpr int DEFAULT_LATENCY_COUNT = 10;
};

#endif // DIAGNOSTICTOOL_H
