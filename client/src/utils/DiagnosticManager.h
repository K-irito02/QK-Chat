#ifndef DIAGNOSTICMANAGER_H
#define DIAGNOSTICMANAGER_H

#include <QObject>
#include <QTimer>
#include <QHash>
#include <QMutex>
#include <QString>
#include <QDateTime>
#include <QProcess>
#include <QSysInfo>

/**
 * @brief 客户端诊断管理器
 * 
 * 负责诊断客户端运行问题，包括：
 * - 网络连接诊断
 * - SSL证书诊断
 * - 系统环境诊断
 * - 性能问题诊断
 * - 错误分析诊断
 */
class DiagnosticManager : public QObject
{
    Q_OBJECT
    
public:
    enum DiagnosticType {
        Network,        // 网络诊断
        SSL,           // SSL诊断
        System,        // 系统诊断
        Performance,   // 性能诊断
        Error,         // 错误诊断
        Database,      // 数据库诊断
        FileSystem     // 文件系统诊断
    };
    Q_ENUM(DiagnosticType)
    
    enum DiagnosticLevel {
        Info,
        Warning,
        Error,
        Critical
    };
    Q_ENUM(DiagnosticLevel)
    
    struct DiagnosticResult {
        DiagnosticType type;
        DiagnosticLevel level;
        QString component;
        QString message;
        QString details;
        QString solution;
        QDateTime timestamp;
    };
    
    struct DiagnosticSession {
        QString sessionId;
        QDateTime startTime;
        QDateTime endTime;
        QList<DiagnosticResult> results;
        QString summary;
        bool isComplete;
    };
    
    static DiagnosticManager* instance();
    
    // 诊断控制
    void startDiagnosticSession(const QString &sessionId);
    void endDiagnosticSession(const QString &sessionId);
    void runDiagnostic(DiagnosticType type, const QString &component = "");
    void runAllDiagnostics(const QString &sessionId);
    
    // 诊断结果
    QList<DiagnosticResult> getDiagnosticResults(const QString &sessionId) const;
    QString generateDiagnosticReport(const QString &sessionId) const;
    QString generateSummaryReport() const;
    
    // 特定诊断
    void diagnoseNetworkConnection(const QString &host, int port);
    void diagnoseSSLCertificate(const QString &certPath);
    void diagnoseSystemEnvironment();
    void diagnosePerformanceIssues();
    void diagnoseErrorPatterns();
    
    // 配置
    void setDiagnosticLevel(DiagnosticLevel level);
    void enableDiagnosticType(DiagnosticType type, bool enabled = true);
    void setAutoDiagnostic(bool enabled);
    
    // 统计
    int getTotalDiagnosticSessions() const;
    int getActiveDiagnosticSessions() const;
    QHash<DiagnosticType, int> getDiagnosticTypeStatistics() const;
    
signals:
    void diagnosticStarted(const QString &sessionId, DiagnosticType type);
    void diagnosticCompleted(const QString &sessionId, DiagnosticType type, const DiagnosticResult &result);
    void diagnosticSessionCompleted(const QString &sessionId);
    void diagnosticError(const QString &sessionId, const QString &error);
    void diagnosticAlert(DiagnosticLevel level, const QString &message);
    
private slots:
    void onNetworkDiagnosticComplete();
    void onSSLDiagnosticComplete();
    void onSystemDiagnosticComplete();
    void onPerformanceDiagnosticComplete();
    void onErrorDiagnosticComplete();
    void autoDiagnostic();
    
private:
    explicit DiagnosticManager(QObject *parent = nullptr);
    ~DiagnosticManager();
    
    void initializeDiagnostics();
    void setupTimers();
    DiagnosticResult createResult(DiagnosticType type, DiagnosticLevel level, 
                                 const QString &component, const QString &message,
                                 const QString &details = "", const QString &solution = "");
    void addResult(const QString &sessionId, const DiagnosticResult &result);
    void saveDiagnosticSession(const DiagnosticSession &session);
    void loadDiagnosticSessions();
    
    // 具体诊断方法
    DiagnosticResult diagnoseNetworkConnectivity(const QString &host, int port);
    DiagnosticResult diagnoseSSLCertificateValidity(const QString &certPath);
    DiagnosticResult diagnoseSystemResources();
    DiagnosticResult diagnosePerformanceMetrics();
    DiagnosticResult diagnoseErrorLogs();
    DiagnosticResult diagnoseFilePermissions();
    DiagnosticResult diagnoseDatabaseConnection();
    
    static DiagnosticManager* _instance;
    static QMutex _instanceMutex;
    
    QTimer *_autoDiagnosticTimer;
    QHash<QString, DiagnosticSession> _sessions;
    QHash<DiagnosticType, bool> _enabledTypes;
    QHash<DiagnosticType, int> _typeStatistics;
    
    DiagnosticLevel _diagnosticLevel;
    bool _autoDiagnostic;
    int _maxSessions;
    
    QMutex _dataMutex;
    
    static const int AUTO_DIAGNOSTIC_INTERVAL = 300000; // 5分钟
    static const int MAX_SESSIONS = 100;
    static const int MAX_RESULTS_PER_SESSION = 1000;
};

#endif // DIAGNOSTICMANAGER_H 