#ifndef CONNECTIONSTATEMANAGER_H
#define CONNECTIONSTATEMANAGER_H

#include <QObject>
#include <QStateMachine>
#include <QState>
#include <QTimer>
#include <QDateTime>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(connectionStateManager)

/**
 * @brief 连接状态管理器
 * 
 * 使用状态机模式管理网络连接的各个阶段：
 * - 断开状态
 * - 连接中状态
 * - SSL握手状态
 * - 认证状态
 * - 已连接状态
 * - 重连状态
 */
class ConnectionStateManager : public QObject
{
    Q_OBJECT
    
public:
    enum ConnectionState {
        Disconnected,       // 断开连接
        Connecting,         // 正在连接
        SslHandshaking,     // SSL握手中
        Authenticating,     // 认证中
        Connected,          // 已连接
        Reconnecting,       // 重连中
        Error              // 错误状态
    };
    Q_ENUM(ConnectionState)
    
    enum ConnectionEvent {
        StartConnection,    // 开始连接
        SocketConnected,    // Socket连接成功
        SslHandshakeCompleted, // SSL握手完成
        AuthenticationSucceeded, // 认证成功
        AuthenticationFailed,    // 认证失败
        ConnectionLost,     // 连接丢失
        ReconnectRequested, // 请求重连
        DisconnectRequested, // 请求断开
        ErrorOccurred       // 发生错误
    };
    Q_ENUM(ConnectionEvent)
    
    explicit ConnectionStateManager(QObject *parent = nullptr);
    ~ConnectionStateManager();
    
    // 状态查询
    ConnectionState getCurrentState() const;
    QString getStateString(ConnectionState state) const;
    bool isConnected() const;
    bool isConnecting() const;
    bool canSendData() const;
    
    // 状态转换
    void triggerEvent(ConnectionEvent event);
    void forceState(ConnectionState state);
    
    // 连接信息
    void setConnectionInfo(const QString &host, int port);
    QString getConnectionHost() const;
    int getConnectionPort() const;
    QDateTime getLastStateChange() const;
    qint64 getConnectionDuration() const;
    
    // 重连配置
    void setMaxRetryAttempts(int maxAttempts);
    void setRetryInterval(int intervalMs);
    void setRetryBackoffMultiplier(double multiplier);
    int getCurrentRetryAttempt() const;
    int getNextRetryInterval() const;
    
signals:
    void stateChanged(ConnectionState oldState, ConnectionState newState);
    void connectionEstablished();
    void connectionLost();
    void authenticationRequired();
    void retryAttemptStarted(int attempt, int maxAttempts);
    void maxRetriesReached();
    void errorStateEntered(const QString &error);
    
private slots:
    void onStateEntered();
    void onStateExited();
    void onRetryTimerTimeout();
    void onConnectionTimeoutTimerTimeout();
    void onAuthTimeoutTimerTimeout();
    
private:
    void setupStateMachine();
    void setupStates();
    void setupTransitions();
    void resetRetryAttempts();
    void incrementRetryAttempt();
    void startRetryTimer();
    void startConnectionTimeoutTimer();
    void startAuthTimeoutTimer();
    void stopAllTimers();
    
    QStateMachine *_stateMachine;
    
    // 状态对象
    QState *_disconnectedState;
    QState *_connectingState;
    QState *_sslHandshakingState;
    QState *_authenticatingState;
    QState *_connectedState;
    QState *_reconnectingState;
    QState *_errorState;
    
    // 定时器
    QTimer *_retryTimer;
    QTimer *_connectionTimeoutTimer;
    QTimer *_authTimeoutTimer;
    
    // 状态信息
    ConnectionState _currentState;
    QDateTime _lastStateChange;
    QDateTime _connectionStartTime;
    
    // 连接信息
    QString _connectionHost;
    int _connectionPort;
    
    // 重连配置
    int _maxRetryAttempts;
    int _baseRetryInterval;
    double _retryBackoffMultiplier;
    int _currentRetryAttempt;
    
    // 超时配置
    static constexpr int DEFAULT_CONNECTION_TIMEOUT = 30000; // 30秒
    static constexpr int DEFAULT_AUTH_TIMEOUT = 15000;       // 15秒
    static constexpr int DEFAULT_RETRY_INTERVAL = 5000;      // 5秒
    static constexpr int DEFAULT_MAX_RETRY_ATTEMPTS = 5;
    static constexpr double DEFAULT_BACKOFF_MULTIPLIER = 1.5;
};

#endif // CONNECTIONSTATEMANAGER_H
