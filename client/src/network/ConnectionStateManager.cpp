#include "ConnectionStateManager.h"
#include "../utils/LogManager.h"
#include <QSignalTransition>
#include <QEventTransition>
#include <QFinalState>
#include <QtMath>

Q_LOGGING_CATEGORY(connectionStateManager, "qkchat.client.connectionstate")

ConnectionStateManager::ConnectionStateManager(QObject *parent)
    : QObject(parent)
    , _stateMachine(new QStateMachine(this))
    , _currentState(Disconnected)
    , _connectionPort(0)
    , _maxRetryAttempts(DEFAULT_MAX_RETRY_ATTEMPTS)
    , _baseRetryInterval(DEFAULT_RETRY_INTERVAL)
    , _retryBackoffMultiplier(DEFAULT_BACKOFF_MULTIPLIER)
    , _currentRetryAttempt(0)
{
    // 创建定时器
    _retryTimer = new QTimer(this);
    _retryTimer->setSingleShot(true);
    connect(_retryTimer, &QTimer::timeout, this, &ConnectionStateManager::onRetryTimerTimeout);
    
    _connectionTimeoutTimer = new QTimer(this);
    _connectionTimeoutTimer->setSingleShot(true);
    connect(_connectionTimeoutTimer, &QTimer::timeout, this, &ConnectionStateManager::onConnectionTimeoutTimerTimeout);
    
    _authTimeoutTimer = new QTimer(this);
    _authTimeoutTimer->setSingleShot(true);
    connect(_authTimeoutTimer, &QTimer::timeout, this, &ConnectionStateManager::onAuthTimeoutTimerTimeout);
    
    setupStateMachine();
    
    _lastStateChange = QDateTime::currentDateTime();
    
    qCInfo(connectionStateManager) << "ConnectionStateManager initialized";
}

ConnectionStateManager::~ConnectionStateManager()
{
    stopAllTimers();
    if (_stateMachine->isRunning()) {
        _stateMachine->stop();
    }
}

void ConnectionStateManager::setupStateMachine()
{
    setupStates();
    setupTransitions();
    
    _stateMachine->setInitialState(_disconnectedState);
    _stateMachine->start();
    
    qCDebug(connectionStateManager) << "State machine started";
}

void ConnectionStateManager::setupStates()
{
    // 创建状态
    _disconnectedState = new QState(_stateMachine);
    _connectingState = new QState(_stateMachine);
    _sslHandshakingState = new QState(_stateMachine);
    _authenticatingState = new QState(_stateMachine);
    _connectedState = new QState(_stateMachine);
    _reconnectingState = new QState(_stateMachine);
    _errorState = new QState(_stateMachine);
    
    // 设置状态属性
    _disconnectedState->setObjectName("Disconnected");
    _connectingState->setObjectName("Connecting");
    _sslHandshakingState->setObjectName("SslHandshaking");
    _authenticatingState->setObjectName("Authenticating");
    _connectedState->setObjectName("Connected");
    _reconnectingState->setObjectName("Reconnecting");
    _errorState->setObjectName("Error");
    
    // 连接状态进入和退出信号
    connect(_disconnectedState, &QState::entered, this, &ConnectionStateManager::onStateEntered);
    connect(_connectingState, &QState::entered, this, &ConnectionStateManager::onStateEntered);
    connect(_sslHandshakingState, &QState::entered, this, &ConnectionStateManager::onStateEntered);
    connect(_authenticatingState, &QState::entered, this, &ConnectionStateManager::onStateEntered);
    connect(_connectedState, &QState::entered, this, &ConnectionStateManager::onStateEntered);
    connect(_reconnectingState, &QState::entered, this, &ConnectionStateManager::onStateEntered);
    connect(_errorState, &QState::entered, this, &ConnectionStateManager::onStateEntered);
    
    connect(_disconnectedState, &QState::exited, this, &ConnectionStateManager::onStateExited);
    connect(_connectingState, &QState::exited, this, &ConnectionStateManager::onStateExited);
    connect(_sslHandshakingState, &QState::exited, this, &ConnectionStateManager::onStateExited);
    connect(_authenticatingState, &QState::exited, this, &ConnectionStateManager::onStateExited);
    connect(_connectedState, &QState::exited, this, &ConnectionStateManager::onStateExited);
    connect(_reconnectingState, &QState::exited, this, &ConnectionStateManager::onStateExited);
    connect(_errorState, &QState::exited, this, &ConnectionStateManager::onStateExited);
}

void ConnectionStateManager::setupTransitions()
{
    // 从断开状态的转换
    _disconnectedState->addTransition(this, &ConnectionStateManager::triggerEvent, _connectingState);
    
    // 从连接中状态的转换
    _connectingState->addTransition(this, &ConnectionStateManager::triggerEvent, _sslHandshakingState);
    _connectingState->addTransition(this, &ConnectionStateManager::triggerEvent, _errorState);
    _connectingState->addTransition(this, &ConnectionStateManager::triggerEvent, _disconnectedState);
    
    // 从SSL握手状态的转换
    _sslHandshakingState->addTransition(this, &ConnectionStateManager::triggerEvent, _authenticatingState);
    _sslHandshakingState->addTransition(this, &ConnectionStateManager::triggerEvent, _errorState);
    _sslHandshakingState->addTransition(this, &ConnectionStateManager::triggerEvent, _disconnectedState);
    
    // 从认证状态的转换
    _authenticatingState->addTransition(this, &ConnectionStateManager::triggerEvent, _connectedState);
    _authenticatingState->addTransition(this, &ConnectionStateManager::triggerEvent, _errorState);
    _authenticatingState->addTransition(this, &ConnectionStateManager::triggerEvent, _disconnectedState);
    
    // 从已连接状态的转换
    _connectedState->addTransition(this, &ConnectionStateManager::triggerEvent, _disconnectedState);
    _connectedState->addTransition(this, &ConnectionStateManager::triggerEvent, _reconnectingState);
    _connectedState->addTransition(this, &ConnectionStateManager::triggerEvent, _errorState);
    
    // 从重连状态的转换
    _reconnectingState->addTransition(this, &ConnectionStateManager::triggerEvent, _connectingState);
    _reconnectingState->addTransition(this, &ConnectionStateManager::triggerEvent, _disconnectedState);
    _reconnectingState->addTransition(this, &ConnectionStateManager::triggerEvent, _errorState);
    
    // 从错误状态的转换
    _errorState->addTransition(this, &ConnectionStateManager::triggerEvent, _disconnectedState);
    _errorState->addTransition(this, &ConnectionStateManager::triggerEvent, _reconnectingState);
}

ConnectionStateManager::ConnectionState ConnectionStateManager::getCurrentState() const
{
    qCDebug(connectionStateManager) << "getCurrentState() called, returning:" << getStateString(_currentState);
    return _currentState;
}

QString ConnectionStateManager::getStateString(ConnectionState state) const
{
    switch (state) {
    case Disconnected:
        return "Disconnected";
    case Connecting:
        return "Connecting";
    case SslHandshaking:
        return "SslHandshaking";
    case Authenticating:
        return "Authenticating";
    case Connected:
        return "Connected";
    case Reconnecting:
        return "Reconnecting";
    case Error:
        return "Error";
    default:
        return "Unknown";
    }
}

bool ConnectionStateManager::isConnected() const
{
    return _currentState == Connected;
}

bool ConnectionStateManager::isConnecting() const
{
    return _currentState == Connecting || 
           _currentState == SslHandshaking || 
           _currentState == Authenticating;
}

bool ConnectionStateManager::canSendData() const
{
    return _currentState == Connected;
}

void ConnectionStateManager::triggerEvent(ConnectionEvent event)
{
    ConnectionState oldState = _currentState;
    
    // 根据当前状态和事件确定新状态
    ConnectionState newState = _currentState;
    
    switch (_currentState) {
    case Disconnected:
        if (event == StartConnection) {
            newState = Connecting;
        }
        break;
        
    case Connecting:
        if (event == SocketConnected) {
            newState = SslHandshaking;
        } else if (event == ErrorOccurred || event == DisconnectRequested) {
            newState = Disconnected;
        }
        break;
        
    case SslHandshaking:
        if (event == SslHandshakeCompleted) {
            newState = Connected;  // 直接进入Connected状态，跳过认证
        } else if (event == ErrorOccurred || event == DisconnectRequested) {
            newState = Disconnected;
        }
        break;
        
    case Authenticating:
        if (event == AuthenticationSucceeded) {
            newState = Connected;
        } else if (event == AuthenticationFailed || event == ErrorOccurred || event == DisconnectRequested) {
            newState = Disconnected;
        }
        break;
        
    case Connected:
        if (event == ConnectionLost) {
            newState = Reconnecting;
        } else if (event == DisconnectRequested || event == ErrorOccurred) {
            newState = Disconnected;
        }
        break;
        
    case Reconnecting:
        if (event == StartConnection) {
            newState = Connecting;
        } else if (event == DisconnectRequested || event == ErrorOccurred) {
            newState = Disconnected;
        }
        break;
        
    case Error:
        if (event == ReconnectRequested) {
            newState = Reconnecting;
        } else if (event == DisconnectRequested) {
            newState = Disconnected;
        }
        break;
    }
    
    if (newState != oldState) {
        _currentState = newState;
        _lastStateChange = QDateTime::currentDateTime();
        
        LogManager::instance()->writeConnectionLog("STATE_CHANGED", 
            QString("From %1 to %2").arg(getStateString(oldState), getStateString(newState)));
        
        emit stateChanged(oldState, newState);
        
        // 处理特殊状态
        if (newState == Connected && oldState != Connected) {
            resetRetryAttempts();
            _connectionStartTime = QDateTime::currentDateTime();
            emit connectionEstablished();
        } else if (oldState == Connected && newState != Connected) {
            emit connectionLost();
        } else if (newState == Authenticating) {
            // 对于邮箱验证等不需要认证的功能，直接进入Connected状态
            // 这里不需要额外处理，因为newState已经是Connected了
        } else if (newState == Reconnecting) {
            incrementRetryAttempt();
            if (_currentRetryAttempt <= _maxRetryAttempts) {
                emit retryAttemptStarted(_currentRetryAttempt, _maxRetryAttempts);
                startRetryTimer();
            } else {
                emit maxRetriesReached();
                forceState(Error);
            }
        }
    }
}

void ConnectionStateManager::forceState(ConnectionState state)
{
    ConnectionState oldState = _currentState;
    _currentState = state;
    _lastStateChange = QDateTime::currentDateTime();
    
    LogManager::instance()->writeConnectionLog("STATE_FORCED", 
        QString("From %1 to %2").arg(getStateString(oldState), getStateString(state)));
    
    emit stateChanged(oldState, state);
}

void ConnectionStateManager::setConnectionInfo(const QString &host, int port)
{
    _connectionHost = host;
    _connectionPort = port;
}

QString ConnectionStateManager::getConnectionHost() const
{
    return _connectionHost;
}

int ConnectionStateManager::getConnectionPort() const
{
    return _connectionPort;
}

QDateTime ConnectionStateManager::getLastStateChange() const
{
    return _lastStateChange;
}

qint64 ConnectionStateManager::getConnectionDuration() const
{
    if (_currentState == Connected && _connectionStartTime.isValid()) {
        return _connectionStartTime.msecsTo(QDateTime::currentDateTime());
    }
    return 0;
}

void ConnectionStateManager::setMaxRetryAttempts(int maxAttempts)
{
    _maxRetryAttempts = maxAttempts;
}

void ConnectionStateManager::setRetryInterval(int intervalMs)
{
    _baseRetryInterval = intervalMs;
}

void ConnectionStateManager::setRetryBackoffMultiplier(double multiplier)
{
    _retryBackoffMultiplier = multiplier;
}

int ConnectionStateManager::getCurrentRetryAttempt() const
{
    return _currentRetryAttempt;
}

int ConnectionStateManager::getNextRetryInterval() const
{
    return static_cast<int>(_baseRetryInterval * qPow(_retryBackoffMultiplier, _currentRetryAttempt));
}

void ConnectionStateManager::onStateEntered()
{
    QState *state = qobject_cast<QState*>(sender());
    if (state) {
        qCDebug(connectionStateManager) << "Entered state:" << state->objectName();
        
        // 根据状态启动相应的定时器
        if (state == _connectingState) {
            startConnectionTimeoutTimer();
        }
    }
}

void ConnectionStateManager::onStateExited()
{
    QState *state = qobject_cast<QState*>(sender());
    if (state) {
        qCDebug(connectionStateManager) << "Exited state:" << state->objectName();
        
        // 停止相关定时器
        if (state == _connectingState) {
            _connectionTimeoutTimer->stop();
        } else if (state == _authenticatingState) {
            _authTimeoutTimer->stop();
        }
    }
}

void ConnectionStateManager::onRetryTimerTimeout()
{
    qCInfo(connectionStateManager) << "Retry timer timeout, attempting reconnection";
    LogManager::instance()->writeConnectionLog("RETRY_TIMEOUT", 
        QString("Attempt %1/%2").arg(_currentRetryAttempt).arg(_maxRetryAttempts));
    
    triggerEvent(StartConnection);
}

void ConnectionStateManager::onConnectionTimeoutTimerTimeout()
{
    qCWarning(connectionStateManager) << "Connection timeout";
    LogManager::instance()->writeConnectionLog("CONNECTION_TIMEOUT", "Connection attempt timed out");
    
    triggerEvent(ErrorOccurred);
}

void ConnectionStateManager::onAuthTimeoutTimerTimeout()
{
    qCWarning(connectionStateManager) << "Authentication timeout";
    LogManager::instance()->writeConnectionLog("AUTH_TIMEOUT", "Authentication timed out");
    
    triggerEvent(AuthenticationFailed);
}

void ConnectionStateManager::resetRetryAttempts()
{
    _currentRetryAttempt = 0;
}

void ConnectionStateManager::incrementRetryAttempt()
{
    _currentRetryAttempt++;
}

void ConnectionStateManager::startRetryTimer()
{
    int interval = getNextRetryInterval();
    _retryTimer->start(interval);
    
    qCInfo(connectionStateManager) << "Retry timer started with interval:" << interval << "ms";
}

void ConnectionStateManager::startConnectionTimeoutTimer()
{
    _connectionTimeoutTimer->start(DEFAULT_CONNECTION_TIMEOUT);
}

void ConnectionStateManager::startAuthTimeoutTimer()
{
    _authTimeoutTimer->start(DEFAULT_AUTH_TIMEOUT);
}

void ConnectionStateManager::stopAllTimers()
{
    _retryTimer->stop();
    _connectionTimeoutTimer->stop();
    _authTimeoutTimer->stop();
}
