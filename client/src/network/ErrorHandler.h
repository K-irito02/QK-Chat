#ifndef ERRORHANDLER_H
#define ERRORHANDLER_H

#include <QObject>
#include <QAbstractSocket>
#include <QSslError>
#include <QTimer>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(errorHandler)

/**
 * @brief 分层错误处理器
 * 
 * 负责处理不同类型的错误：
 * - 网络错误
 * - SSL错误
 * - 协议错误
 * - 认证错误
 */
class ErrorHandler : public QObject
{
    Q_OBJECT
    
public:
    enum ErrorType {
        NetworkError,       // 网络错误
        SslError,          // SSL错误
        ProtocolError,     // 协议错误
        AuthenticationError, // 认证错误
        TimeoutError,      // 超时错误
        UnknownError       // 未知错误
    };
    Q_ENUM(ErrorType)
    
    enum ErrorSeverity {
        Critical,          // 严重错误，需要断开连接
        Warning,           // 警告，可以尝试恢复
        Info              // 信息，仅记录
    };
    Q_ENUM(ErrorSeverity)
    
    enum RecoveryStrategy {
        NoRecovery,        // 不恢复
        Retry,             // 重试
        Reconnect,         // 重连
        Fallback,          // 降级处理
        UserIntervention   // 需要用户干预
    };
    Q_ENUM(RecoveryStrategy)
    
    struct ErrorInfo {
        ErrorType type;
        ErrorSeverity severity;
        QString message;
        QString details;
        QString solution;
        RecoveryStrategy strategy;
        QDateTime timestamp;
        int errorCode;
    };
    
    explicit ErrorHandler(QObject *parent = nullptr);
    ~ErrorHandler();
    
    // 错误处理
    void handleNetworkError(QAbstractSocket::SocketError error, const QString &details = "");
    void handleSslError(const QList<QSslError> &errors);
    void handleProtocolError(const QString &message, const QString &details = "");
    void handleAuthenticationError(const QString &message, const QString &details = "");
    void handleTimeoutError(const QString &operation, const QString &details = "");
    void handleCustomError(ErrorType type, const QString &message, const QString &details = "");
    
    // 错误分析
    ErrorInfo analyzeNetworkError(QAbstractSocket::SocketError error) const;
    ErrorInfo analyzeSslError(const QSslError &error) const;
    QString getErrorSolution(ErrorType type, int errorCode) const;
    RecoveryStrategy getRecoveryStrategy(ErrorType type, ErrorSeverity severity) const;
    
    // 恢复策略
    bool canRecover(const ErrorInfo &error) const;
    void executeRecoveryStrategy(const ErrorInfo &error);
    
    // 错误统计
    int getErrorCount(ErrorType type) const;
    QList<ErrorInfo> getRecentErrors(int maxCount = 10) const;
    void clearErrorHistory();
    
    // 配置
    void setMaxRetryAttempts(int maxAttempts);
    void setRetryDelay(int delayMs);
    void setDevelopmentMode(bool enabled);
    
signals:
    void errorOccurred(const ErrorInfo &error);
    void recoveryAttempted(const ErrorInfo &error, RecoveryStrategy strategy);
    void recoverySucceeded(const ErrorInfo &error);
    void recoveryFailed(const ErrorInfo &error);
    void criticalErrorOccurred(const ErrorInfo &error);
    void userInterventionRequired(const ErrorInfo &error);
    
private slots:
    void onRetryTimerTimeout();
    
private:
    QString getErrorTypeString(ErrorType type) const;
    QString getErrorSeverityString(ErrorSeverity severity) const;
    QString getRecoveryStrategyString(RecoveryStrategy strategy) const;
    
    void logError(const ErrorInfo &error);
    void addToHistory(const ErrorInfo &error);
    
    QList<ErrorInfo> _errorHistory;
    QHash<ErrorType, int> _errorCounts;
    
    QTimer *_retryTimer;
    int _maxRetryAttempts;
    int _retryDelay;
    bool _developmentMode;
    
    static const int MAX_ERROR_HISTORY = 100;
    static const int DEFAULT_MAX_RETRY_ATTEMPTS = 3;
    static const int DEFAULT_RETRY_DELAY = 1000; // 1秒
};

#endif // ERRORHANDLER_H
