#include <QtTest>
#include <QSignalSpy>
#include "../src/monitoring/DiagnosticTool.h"

class DiagnosticToolTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testNetworkConnectivity();
    void testDnsResolution();
    void testPortConnectivity();
    void testFullDiagnostic();

private:
    DiagnosticTool *diagnosticTool;
};

void DiagnosticToolTest::initTestCase()
{
    diagnosticTool = new DiagnosticTool(this);
}

void DiagnosticToolTest::cleanupTestCase()
{
    delete diagnosticTool;
}

void DiagnosticToolTest::testNetworkConnectivity()
{
    QSignalSpy spy(diagnosticTool, &DiagnosticTool::testCompleted);
    
    diagnosticTool->runSpecificTest(DiagnosticTool::NetworkConnectivity, "www.baidu.com", 80);
    
    // 等待测试完成
    QVERIFY(spy.wait(10000)); // 10秒超时
    
    // 检查测试结果
    DiagnosticTool::TestInfo result = diagnosticTool->getTestResult(DiagnosticTool::NetworkConnectivity);
    QCOMPARE(result.type, DiagnosticTool::NetworkConnectivity);
    QVERIFY(result.result != DiagnosticTool::NotRun);
}

void DiagnosticToolTest::testDnsResolution()
{
    QSignalSpy spy(diagnosticTool, &DiagnosticTool::testCompleted);
    
    diagnosticTool->runSpecificTest(DiagnosticTool::DnsResolution, "www.baidu.com", 80);
    
    QVERIFY(spy.wait(10000));
    
    DiagnosticTool::TestInfo result = diagnosticTool->getTestResult(DiagnosticTool::DnsResolution);
    QCOMPARE(result.type, DiagnosticTool::DnsResolution);
    QVERIFY(result.result != DiagnosticTool::NotRun);
}

void DiagnosticToolTest::testPortConnectivity()
{
    QSignalSpy spy(diagnosticTool, &DiagnosticTool::testCompleted);
    
    diagnosticTool->runSpecificTest(DiagnosticTool::PortConnectivity, "www.baidu.com", 80);
    
    QVERIFY(spy.wait(10000));
    
    DiagnosticTool::TestInfo result = diagnosticTool->getTestResult(DiagnosticTool::PortConnectivity);
    QCOMPARE(result.type, DiagnosticTool::PortConnectivity);
    QVERIFY(result.result != DiagnosticTool::NotRun);
}

void DiagnosticToolTest::testFullDiagnostic()
{
    QSignalSpy spy(diagnosticTool, &DiagnosticTool::diagnosticCompleted);
    
    diagnosticTool->runFullDiagnostic("www.baidu.com", 80);
    
    QVERIFY(spy.wait(30000)); // 30秒超时，因为要运行多个测试
    
    DiagnosticTool::DiagnosticReport report = diagnosticTool->getLastReport();
    QVERIFY(!report.tests.isEmpty());
    QVERIFY(!report.summary.isEmpty());
    QVERIFY(!report.recommendations.isEmpty());
}

QTEST_MAIN(DiagnosticToolTest)
#include "DiagnosticToolTest.moc"
