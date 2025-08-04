#include "DiagnosticTool.h"
#include "../utils/LogManager.h"
#include <QHostInfo>
#include <QTcpSocket>
#include <QSslSocket>
#include <QNetworkRequest>
#include <QNetworkInterface>
#include <QSysInfo>
#include <QCoreApplication>
#include <QRandomGenerator>

Q_LOGGING_CATEGORY(diagnosticTool, "qkchat.client.diagnostic")

DiagnosticTool::DiagnosticTool(QObject *parent)
    : QObject(parent)
    , _diagnosticRunning(false)
    , _targetPort(0)
    , _currentTest(NetworkConnectivity)
    , _currentTestIndex(0)
    , _timeout(DEFAULT_TIMEOUT)
    , _retryCount(DEFAULT_RETRY_COUNT)
    , _bandwidthTestSize(DEFAULT_BANDWIDTH_SIZE)
    , _latencyTestCount(DEFAULT_LATENCY_COUNT)
    , _latencyTestCurrent(0)
{
    _networkManager = new QNetworkAccessManager(this);
    connect(_networkManager, &QNetworkAccessManager::finished, this, &DiagnosticTool::onNetworkReply);
    
    _timeoutTimer = new QTimer(this);
    _timeoutTimer->setSingleShot(true);
    connect(_timeoutTimer, &QTimer::timeout, this, &DiagnosticTool::onTestTimeout);
    
    _latencyTimer = new QTimer(this);
    connect(_latencyTimer, &QTimer::timeout, this, &DiagnosticTool::onLatencyTestTimer);
    
    qCInfo(diagnosticTool) << "DiagnosticTool initialized";
}

DiagnosticTool::~DiagnosticTool()
{
    cancelDiagnostic();
}

void DiagnosticTool::runFullDiagnostic(const QString &host, int port)
{
    if (_diagnosticRunning) {
        qCWarning(diagnosticTool) << "Diagnostic already running";
        return;
    }
    
    _targetHost = host;
    _targetPort = port;
    _diagnosticRunning = true;
    _currentTestIndex = 0;
    
    // 设置测试队列
    _testQueue = {
        NetworkConnectivity,
        DnsResolution,
        PortConnectivity,
        SslCertificate,
        Latency,
        PacketLoss,
        Bandwidth
    };
    
    // 清空之前的结果
    _testResults.clear();
    
    qCInfo(diagnosticTool) << "Starting full diagnostic for" << host << ":" << port;
    LogManager::instance()->writeConnectionLog("DIAGNOSTIC_STARTED", 
        QString("Host: %1, Port: %2").arg(host).arg(port));
    
    emit diagnosticStarted();
    
    startNextTest();
}

void DiagnosticTool::runSpecificTest(TestType type, const QString &host, int port)
{
    if (_diagnosticRunning) {
        qCWarning(diagnosticTool) << "Diagnostic already running";
        return;
    }
    
    _targetHost = host;
    _targetPort = port;
    _diagnosticRunning = true;
    _currentTestIndex = 0;
    
    _testQueue = {type};
    _testResults.clear();
    
    qCInfo(diagnosticTool) << "Starting specific test:" << static_cast<int>(type) << "for" << host << ":" << port;
    
    emit diagnosticStarted();
    
    startNextTest();
}

void DiagnosticTool::cancelDiagnostic()
{
    if (!_diagnosticRunning) {
        return;
    }
    
    _diagnosticRunning = false;
    _timeoutTimer->stop();
    _latencyTimer->stop();
    
    // 取消网络请求
    _networkManager->clearConnectionCache();
    
    qCInfo(diagnosticTool) << "Diagnostic cancelled";
    LogManager::instance()->writeConnectionLog("DIAGNOSTIC_CANCELLED", "User requested cancellation");
}

bool DiagnosticTool::isDiagnosticRunning() const
{
    return _diagnosticRunning;
}

DiagnosticTool::DiagnosticReport DiagnosticTool::getLastReport() const
{
    return _lastReport;
}

QList<DiagnosticTool::TestInfo> DiagnosticTool::getTestResults() const
{
    return _testResults.values();
}

DiagnosticTool::TestInfo DiagnosticTool::getTestResult(TestType type) const
{
    return _testResults.value(type);
}

void DiagnosticTool::setTimeout(int timeoutMs)
{
    _timeout = timeoutMs;
}

void DiagnosticTool::setRetryCount(int retries)
{
    _retryCount = retries;
}

void DiagnosticTool::setBandwidthTestSize(qint64 bytes)
{
    _bandwidthTestSize = bytes;
}

void DiagnosticTool::setLatencyTestCount(int count)
{
    _latencyTestCount = count;
}

void DiagnosticTool::onNetworkReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    _timeoutTimer->stop();
    
    if (_currentTest == NetworkConnectivity) {
        if (reply->error() == QNetworkReply::NoError) {
            completeCurrentTest(Passed, "网络连通性正常");
        } else {
            completeCurrentTest(Failed, QString("网络连接失败: %1").arg(reply->errorString()));
        }
    } else if (_currentTest == Bandwidth) {
        if (reply->error() == QNetworkReply::NoError) {
            qint64 bytes = reply->bytesAvailable();
            qint64 duration = _testResults[_currentTest].startTime.msecsTo(QDateTime::currentDateTime());
            double throughput = duration > 0 ? (bytes * 1000.0 / duration) : 0;
            
            QVariantMap data;
            data["bytes"] = bytes;
            data["duration"] = duration;
            data["throughput"] = throughput;
            
            completeCurrentTest(Passed, 
                QString("带宽测试完成: %1 KB/s").arg(throughput / 1024, 0, 'f', 2), data);
        } else {
            completeCurrentTest(Failed, QString("带宽测试失败: %1").arg(reply->errorString()));
        }
    }
    
    reply->deleteLater();
}

void DiagnosticTool::onTestTimeout()
{
    QString timeoutMsg = QString("测试超时 (%1ms)").arg(_timeout);
    completeCurrentTest(Failed, timeoutMsg);
}

void DiagnosticTool::onLatencyTestTimer()
{
    if (_latencyTestCurrent >= _latencyTestCount) {
        // 延迟测试完成
        _latencyTimer->stop();
        
        if (!_latencyResults.isEmpty()) {
            qint64 totalLatency = 0;
            qint64 minLatency = LLONG_MAX;
            qint64 maxLatency = 0;
            
            for (qint64 latency : _latencyResults) {
                totalLatency += latency;
                minLatency = qMin(minLatency, latency);
                maxLatency = qMax(maxLatency, latency);
            }
            
            qint64 avgLatency = totalLatency / _latencyResults.size();
            
            QVariantMap data;
            data["average"] = avgLatency;
            data["minimum"] = minLatency;
            data["maximum"] = maxLatency;
            data["count"] = _latencyResults.size();
            
            QString result = QString("平均延迟: %1ms (最小: %2ms, 最大: %3ms)")
                           .arg(avgLatency).arg(minLatency).arg(maxLatency);
            
            TestResult testResult = avgLatency < 100 ? Passed : (avgLatency < 300 ? Warning : Failed);
            completeCurrentTest(testResult, result, data);
        } else {
            completeCurrentTest(Failed, "延迟测试失败");
        }
        return;
    }
    
    // 发送ping请求
    QNetworkRequest request{QUrl(QString("http://%1:%2").arg(_targetHost).arg(_targetPort))};
    request.setRawHeader("User-Agent", "QKChatApp-DiagnosticTool");
    
    _latencyTestStart = QDateTime::currentDateTime();
    QNetworkReply *reply = _networkManager->head(request);
    
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        qint64 latency = _latencyTestStart.msecsTo(QDateTime::currentDateTime());
        _latencyResults.append(latency);
        reply->deleteLater();
    });
    
    _latencyTestCurrent++;
}

void DiagnosticTool::startNextTest()
{
    if (_currentTestIndex >= _testQueue.size()) {
        completeDiagnostic();
        return;
    }
    
    _currentTest = _testQueue[_currentTestIndex];
    
    // 初始化测试信息
    TestInfo testInfo;
    testInfo.type = _currentTest;
    testInfo.result = InProgress;
    testInfo.name = getTestName(_currentTest);
    testInfo.description = getTestDescription(_currentTest);
    testInfo.startTime = QDateTime::currentDateTime();
    
    _testResults[_currentTest] = testInfo;
    
    qCInfo(diagnosticTool) << "Starting test:" << testInfo.name;
    LogManager::instance()->writeConnectionLog("TEST_STARTED", testInfo.name);
    
    emit testStarted(_currentTest);
    updateProgress();
    
    // 启动超时定时器
    _timeoutTimer->start(_timeout);
    
    // 执行具体测试
    switch (_currentTest) {
    case NetworkConnectivity:
        testNetworkConnectivity();
        break;
    case DnsResolution:
        testDnsResolution();
        break;
    case PortConnectivity:
        testPortConnectivity();
        break;
    case SslCertificate:
        testSslCertificate();
        break;
    case Bandwidth:
        testBandwidth();
        break;
    case Latency:
        testLatency();
        break;
    case PacketLoss:
        testPacketLoss();
        break;
    }
}

void DiagnosticTool::completeCurrentTest(TestResult result, const QString &details, const QVariantMap &data)
{
    _timeoutTimer->stop();
    
    if (_testResults.contains(_currentTest)) {
        TestInfo &testInfo = _testResults[_currentTest];
        testInfo.result = result;
        testInfo.details = details;
        testInfo.endTime = QDateTime::currentDateTime();
        testInfo.duration = testInfo.startTime.msecsTo(testInfo.endTime);
        testInfo.data = data;
        
        QString resultStr;
        switch (result) {
        case Passed: resultStr = "PASSED"; break;
        case Failed: resultStr = "FAILED"; break;
        case Warning: resultStr = "WARNING"; break;
        default: resultStr = "UNKNOWN"; break;
        }
        
        qCInfo(diagnosticTool) << "Test completed:" << testInfo.name << resultStr << details;
        LogManager::instance()->writeConnectionLog("TEST_COMPLETED", 
            QString("%1: %2 - %3").arg(testInfo.name, resultStr, details));
        
        emit testCompleted(_currentTest, result);
    }
    
    _currentTestIndex++;
    startNextTest();
}

void DiagnosticTool::completeDiagnostic()
{
    _diagnosticRunning = false;
    
    // 生成诊断报告
    _lastReport.timestamp = QDateTime::currentDateTime();
    _lastReport.tests = _testResults.values();
    _lastReport.recommendations = generateRecommendations();
    _lastReport.systemInfo = collectSystemInfo();
    
    // 计算总体成功率
    int passedTests = 0;
    int totalTests = _lastReport.tests.size();
    
    for (const TestInfo &test : _lastReport.tests) {
        if (test.result == Passed) {
            passedTests++;
        }
    }
    
    _lastReport.overallSuccess = (passedTests == totalTests);
    _lastReport.summary = QString("诊断完成: %1/%2 项测试通过").arg(passedTests).arg(totalTests);
    
    qCInfo(diagnosticTool) << "Diagnostic completed:" << _lastReport.summary;
    LogManager::instance()->writeConnectionLog("DIAGNOSTIC_COMPLETED", _lastReport.summary);
    
    emit diagnosticCompleted(_lastReport);
}

void DiagnosticTool::testNetworkConnectivity()
{
    // 测试基本网络连通性
    QNetworkRequest request{QUrl("http://www.baidu.com")};
    request.setRawHeader("User-Agent", "QKChatApp-DiagnosticTool");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    _networkManager->get(request);
}

void DiagnosticTool::testDnsResolution()
{
    // DNS解析测试
    QHostInfo::lookupHost(_targetHost, [this](const QHostInfo &info) {
        _timeoutTimer->stop();

        if (info.error() == QHostInfo::NoError && !info.addresses().isEmpty()) {
            QStringList addresses;
            for (const QHostAddress &addr : info.addresses()) {
                addresses << addr.toString();
            }

            QVariantMap data;
            data["addresses"] = addresses;
            data["hostname"] = info.hostName();

            completeCurrentTest(Passed,
                QString("DNS解析成功: %1 -> %2").arg(_targetHost, addresses.join(", ")), data);
        } else {
            completeCurrentTest(Failed,
                QString("DNS解析失败: %1").arg(info.errorString()));
        }
    });
}

void DiagnosticTool::testPortConnectivity()
{
    // 端口连通性测试
    QTcpSocket *socket = new QTcpSocket(this);

    connect(socket, &QTcpSocket::connected, [this, socket]() {
        _timeoutTimer->stop();

        QVariantMap data;
        data["localAddress"] = socket->localAddress().toString();
        data["localPort"] = socket->localPort();
        data["peerAddress"] = socket->peerAddress().toString();
        data["peerPort"] = socket->peerPort();

        completeCurrentTest(Passed,
            QString("端口 %1:%2 连接成功").arg(_targetHost).arg(_targetPort), data);

        socket->disconnectFromHost();
        socket->deleteLater();
    });

    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            [this, socket](QAbstractSocket::SocketError error) {
        _timeoutTimer->stop();

        completeCurrentTest(Failed,
            QString("端口连接失败: %1").arg(socket->errorString()));

        socket->deleteLater();
    });

    socket->connectToHost(_targetHost, _targetPort);
}

void DiagnosticTool::testSslCertificate()
{
    // SSL证书测试
    QSslSocket *sslSocket = new QSslSocket(this);

    connect(sslSocket, &QSslSocket::encrypted, [this, sslSocket]() {
        _timeoutTimer->stop();

        QSslCertificate cert = sslSocket->peerCertificate();
        QVariantMap data;
        data["subject"] = cert.subjectInfo(QSslCertificate::CommonName).join(", ");
        data["issuer"] = cert.issuerInfo(QSslCertificate::CommonName).join(", ");
        data["validFrom"] = cert.effectiveDate();
        data["validTo"] = cert.expiryDate();
        data["serialNumber"] = cert.serialNumber();

        QString details = QString("SSL证书有效 - 颁发给: %1, 颁发者: %2")
                         .arg(data["subject"].toString(), data["issuer"].toString());

        QDateTime now = QDateTime::currentDateTime();
        bool isValid = !cert.isNull() &&
                      cert.effectiveDate() <= now &&
                      cert.expiryDate() >= now;
        TestResult result = isValid ? Passed : Warning;
        completeCurrentTest(result, details, data);

        sslSocket->disconnectFromHost();
        sslSocket->deleteLater();
    });

    connect(sslSocket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors),
            [this, sslSocket](const QList<QSslError> &errors) {
        _timeoutTimer->stop();

        QStringList errorStrings;
        for (const QSslError &error : errors) {
            errorStrings << error.errorString();
        }

        QVariantMap data;
        data["errors"] = errorStrings;

        completeCurrentTest(Failed,
            QString("SSL证书错误: %1").arg(errorStrings.join("; ")), data);

        sslSocket->deleteLater();
    });

    connect(sslSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            [this, sslSocket](QAbstractSocket::SocketError error) {
        _timeoutTimer->stop();

        completeCurrentTest(Failed,
            QString("SSL连接失败: %1").arg(sslSocket->errorString()));

        sslSocket->deleteLater();
    });

    sslSocket->connectToHostEncrypted(_targetHost, _targetPort);
}

void DiagnosticTool::testBandwidth()
{
    // 带宽测试 - 下载测试文件
    QString testUrl = QString("http://%1:%2/test_data").arg(_targetHost).arg(_targetPort);
    QNetworkRequest request{QUrl(testUrl)};
    request.setRawHeader("User-Agent", "QKChatApp-DiagnosticTool");

    // 如果服务器不支持测试文件，使用公共测试服务
    if (_targetHost == "localhost" || _targetHost == "127.0.0.1") {
        request.setUrl(QUrl("http://httpbin.org/bytes/1048576")); // 1MB测试数据
    }

    _networkManager->get(request);
}

void DiagnosticTool::testLatency()
{
    // 延迟测试
    _latencyTestCurrent = 0;
    _latencyResults.clear();

    _latencyTimer->start(100); // 每100ms发送一次请求
}

void DiagnosticTool::testPacketLoss()
{
    // 丢包测试 - 简化版本，发送多个请求检查响应率
    int testCount = 10;
    int successCount = 0;
    int completedCount = 0;

    for (int i = 0; i < testCount; ++i) {
        QNetworkRequest request{QUrl(QString("http://%1:%2").arg(_targetHost).arg(_targetPort))};
        request.setRawHeader("User-Agent", "QKChatApp-DiagnosticTool");

        QNetworkReply *reply = _networkManager->head(request);

        connect(reply, &QNetworkReply::finished, [this, reply, &successCount, &completedCount, testCount]() {
            completedCount++;

            if (reply->error() == QNetworkReply::NoError) {
                successCount++;
            }

            reply->deleteLater();

            if (completedCount == testCount) {
                _timeoutTimer->stop();

                double lossRate = (1.0 - static_cast<double>(successCount) / testCount) * 100.0;

                QVariantMap data;
                data["totalPackets"] = testCount;
                data["successfulPackets"] = successCount;
                data["lossRate"] = lossRate;

                QString details = QString("丢包率: %1% (%2/%3 成功)")
                                .arg(lossRate, 0, 'f', 1).arg(successCount).arg(testCount);

                TestResult result = lossRate < 1.0 ? Passed : (lossRate < 5.0 ? Warning : Failed);
                completeCurrentTest(result, details, data);
            }
        });
    }
}

QString DiagnosticTool::getTestName(TestType type) const
{
    switch (type) {
    case NetworkConnectivity:
        return "网络连通性测试";
    case DnsResolution:
        return "DNS解析测试";
    case PortConnectivity:
        return "端口连通性测试";
    case SslCertificate:
        return "SSL证书测试";
    case Bandwidth:
        return "带宽测试";
    case Latency:
        return "延迟测试";
    case PacketLoss:
        return "丢包测试";
    default:
        return "未知测试";
    }
}

QString DiagnosticTool::getTestDescription(TestType type) const
{
    switch (type) {
    case NetworkConnectivity:
        return "检查基本网络连接是否正常";
    case DnsResolution:
        return "检查DNS域名解析是否正常";
    case PortConnectivity:
        return "检查目标端口是否可以连接";
    case SslCertificate:
        return "检查SSL证书是否有效";
    case Bandwidth:
        return "测试网络带宽和传输速度";
    case Latency:
        return "测试网络延迟和响应时间";
    case PacketLoss:
        return "测试网络丢包率";
    default:
        return "未知测试类型";
    }
}

QStringList DiagnosticTool::generateRecommendations() const
{
    QStringList recommendations;

    for (const TestInfo &test : _testResults.values()) {
        if (test.result == Failed) {
            switch (test.type) {
            case NetworkConnectivity:
                recommendations << "检查网络连接，确保设备已连接到互联网";
                break;
            case DnsResolution:
                recommendations << "检查DNS设置，尝试使用不同的DNS服务器";
                break;
            case PortConnectivity:
                recommendations << "检查防火墙设置，确保目标端口未被阻止";
                break;
            case SslCertificate:
                recommendations << "检查SSL证书配置，确保证书有效且未过期";
                break;
            case Bandwidth:
                recommendations << "检查网络带宽，考虑升级网络连接";
                break;
            case Latency:
                recommendations << "检查网络延迟，考虑使用更近的服务器";
                break;
            case PacketLoss:
                recommendations << "检查网络稳定性，可能需要更换网络环境";
                break;
            }
        }
    }

    if (recommendations.isEmpty()) {
        recommendations << "所有测试都通过，网络连接状态良好";
    }

    return recommendations;
}

QVariantMap DiagnosticTool::collectSystemInfo() const
{
    QVariantMap info;

    // 系统信息
    info["os"] = QSysInfo::prettyProductName();
    info["architecture"] = QSysInfo::currentCpuArchitecture();
    info["kernel"] = QSysInfo::kernelVersion();
    info["hostname"] = QSysInfo::machineHostName();

    // 应用信息
    info["appName"] = QCoreApplication::applicationName();
    info["appVersion"] = QCoreApplication::applicationVersion();
    info["qtVersion"] = qVersion();

    // 网络接口信息
    QStringList interfaces;
    for (const QNetworkInterface &interface : QNetworkInterface::allInterfaces()) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) &&
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            interfaces << QString("%1 (%2)").arg(interface.name(), interface.humanReadableName());
        }
    }
    info["networkInterfaces"] = interfaces;

    return info;
}

void DiagnosticTool::updateProgress()
{
    if (_testQueue.isEmpty()) {
        return;
    }

    int percentage = (_currentTestIndex * 100) / _testQueue.size();
    emit progressUpdated(percentage);
}
