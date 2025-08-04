#ifndef RECONNECTMANAGER_H
#define RECONNECTMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(reconnectManager)

/**
 * @brief 智能重连管理器
 * 
 * 实现智能重连机制，包括：
 * - 指数退避算法
 * - 最大重试次数限制
 * - 连接超时处理
 * - 网络状态检测
 */
class ReconnectManager : public QObject
{
    Q_OBJECT
    
public:
    enum ReconnectStrategy {
        FixedInterval,      // 固定间隔
        ExponentialBackoff, // 指数退避
        LinearBackoff,      // 线性退避
        AdaptiveBackoff     // 自适应退避
    };
    Q_ENUM(ReconnectStrategy)
    
    enum ReconnectTrigger {
        Manual,             // 手动触发
        ConnectionLost,     // 连接丢失
        NetworkError,       // 网络错误
        AuthenticationFailed, // 认证失败
        Timeout            // 超时
    };
    Q_ENUM(ReconnectTrigger)
    
    struct ReconnectAttempt {
        int attemptNumber;
        QDateTime timestamp;
        ReconnectTrigger trigger;
        QString reason;
        int delayMs;
        bool successful;
    };
    
    explicit ReconnectManager(QObject *parent = nullptr);
    ~ReconnectManager();
    
    // 重连控制
    void startReconnect(ReconnectTrigger trigger, const QString &reason = "");
    void stopReconnect();
    void resetReconnectState();
    bool isReconnecting() const;
    
    // 配置
    void setMaxAttempts(int maxAttempts);
    void setBaseInterval(int intervalMs);
    void setMaxInterval(int maxIntervalMs);
    void setBackoffMultiplier(double multiplier);
    void setStrategy(ReconnectStrategy strategy);
    void setConnectionTimeout(int timeoutMs);
    
    // 状态查询
    int getCurrentAttempt() const;
    int getMaxAttempts() const;
    int getNextInterval() const;
    QDateTime getLastAttemptTime() const;
    QList<ReconnectAttempt> getAttemptHistory() const;
    
    // 网络状态
    void setNetworkAvailable(bool available);
    bool isNetworkAvailable() const;
    
    // 统计信息
    int getTotalAttempts() const;
    int getSuccessfulAttempts() const;
    double getSuccessRate() const;
    qint64 getTotalReconnectTime() const;
    
signals:
    void reconnectStarted(ReconnectTrigger trigger, const QString &reason);
    void reconnectAttempt(int attempt, int maxAttempts, int delayMs);
    void reconnectSucceeded(int attempt, qint64 totalTime);
    void reconnectFailed(int attempt, const QString &reason);
    void maxAttemptsReached();
    void reconnectStopped();
    void networkStatusChanged(bool available);
    
private slots:
    void onReconnectTimer();
    void onConnectionTimeout();
    void onNetworkStatusCheck();
    
private:
    int calculateNextInterval() const;
    int calculateExponentialBackoff() const;
    int calculateLinearBackoff() const;
    int calculateAdaptiveBackoff() const;
    void addAttemptToHistory(const ReconnectAttempt &attempt);
    void checkNetworkStatus();
    void startConnectionTimeout();
    void stopConnectionTimeout();
    
    QTimer *_reconnectTimer;
    QTimer *_connectionTimeoutTimer;
    QTimer *_networkStatusTimer;
    
    // 重连状态
    bool _isReconnecting;
    int _currentAttempt;
    QDateTime _reconnectStartTime;
    QDateTime _lastAttemptTime;
    ReconnectTrigger _currentTrigger;
    QString _currentReason;
    
    // 配置参数
    int _maxAttempts;
    int _baseInterval;
    int _maxInterval;
    double _backoffMultiplier;
    ReconnectStrategy _strategy;
    int _connectionTimeout;
    
    // 网络状态
    bool _networkAvailable;
    
    // 历史记录
    QList<ReconnectAttempt> _attemptHistory;
    int _totalAttempts;
    int _successfulAttempts;
    
    // 默认配置
    static constexpr int DEFAULT_MAX_ATTEMPTS = 10;
    static constexpr int DEFAULT_BASE_INTERVAL = 1000;      // 1秒
    static constexpr int DEFAULT_MAX_INTERVAL = 60000;      // 60秒
    static constexpr double DEFAULT_BACKOFF_MULTIPLIER = 1.5;
    static constexpr int DEFAULT_CONNECTION_TIMEOUT = 30000; // 30秒
    static constexpr int NETWORK_STATUS_CHECK_INTERVAL = 5000; // 5秒
    static constexpr int MAX_ATTEMPT_HISTORY = 50;
};

#endif // RECONNECTMANAGER_H
