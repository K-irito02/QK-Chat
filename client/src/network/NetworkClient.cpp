#include "NetworkClient.h"
#include "SSLConfigManager.h"
#include "../utils/LogManager.h"
#include "../config/DevelopmentConfig.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDir>
#include <QLoggingCategory>
#include <QDebug>
#include <QtEndian>
#include <QSslCipher>
#include <QTimer>
#include <QThread>
#include <QElapsedTimer>
#include <QMutex>

Q_LOGGING_CATEGORY(networkClient, "qkchat.client.network")

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
    , _sslSocket(new QSslSocket(this))
    , _heartbeatTimer(new QTimer(this))
    , _networkManager(new QNetworkAccessManager(this))
    , _stateManager(new ConnectionStateManager(this))
    , _errorHandler(new ErrorHandler(this))
    , _reconnectManager(new ReconnectManager(this))
    , _heartbeatManager(new HeartbeatManager(this))
    , _isConnected(false)
    , _serverPort(0)
    , _sendMutex(new QMutex())
{
    setupSslSocket();
    setupStateManager();
    setupErrorHandler();
    setupReconnectManager();
    setupHeartbeatManager();

    // 旧的心跳定时器已被HeartbeatManager替代

    qCInfo(networkClient) << "NetworkClient created";
    LogManager::instance()->writeDiagnosticLog("NetworkClient", "Initialized", "Network client created successfully");
}

NetworkClient::~NetworkClient()
{
    disconnect();
    delete _sendMutex; // 清理互斥锁
    qCInfo(networkClient) << "NetworkClient destroyed";
    LogManager::instance()->writeDiagnosticLog("NetworkClient", "Destroyed", "Network client destroyed");
}

// 连接管理
bool NetworkClient::connectToServer(const QString &host, int port)
{
    if (_stateManager->isConnected()) {
            qCWarning(networkClient) << "Already connected to server";
            LogManager::instance()->writeConnectionLog("Already Connected", 
            QString("Host: %1, Port: %2").arg(host).arg(port), LogManager::WarningLevel);
        return true;
    }

    if (_stateManager->isConnecting()) {
            qCWarning(networkClient) << "Connection already in progress";
            LogManager::instance()->writeConnectionLog("Connection In Progress",
            QString("Host: %1, Port: %2").arg(host).arg(port), LogManager::WarningLevel);
        return true;
    }

    _serverHost = host;
    _serverPort = port;

    qCInfo(networkClient) << "Connecting to server:" << host << ":" << port;
            LogManager::instance()->writeConnectionLog("Connecting",
            QString("Host: %1, Port: %2").arg(host).arg(port), LogManager::InfoLevel);

    // 设置连接信息到状态管理器
    _stateManager->setConnectionInfo(host, port);

    // 清空之前的缓冲区
    _readBuffer.clear();

    // 使用最新的SSL配置
    QSslConfiguration sslConfig = SSLConfigManager::instance()->createSslConfiguration();
    _sslSocket->setSslConfiguration(sslConfig);

    // 触发状态管理器开始连接
    _stateManager->triggerEvent(ConnectionStateManager::StartConnection);

    _sslSocket->connectToHostEncrypted(host, port);

    return true;
}

void NetworkClient::disconnect()
{
    // 停止重连
    _reconnectManager->stopReconnect();

    if (_sslSocket->state() != QAbstractSocket::UnconnectedState) {
        _sslSocket->disconnectFromHost();
    }

    stopHeartbeat();
    _isConnected = false;
    _readBuffer.clear();
    _authToken.clear();

    qCInfo(networkClient) << "Disconnected from server";
    LogManager::instance()->writeConnectionLog("MANUAL_DISCONNECT", "User requested disconnect");
}

bool NetworkClient::isConnected() const
{
    if (!_sslSocket) {
        return false;
    }
    
    bool socketConnected = _sslSocket->state() == QAbstractSocket::ConnectedState;
    bool sslEncrypted = _sslSocket->isEncrypted();
    bool socketConnectedFinal = _sslSocket->isValid();
    
    qCInfo(networkClient) << "isConnected() check - State:" << _stateManager->getCurrentState() 
                         << "Socket state:" << _sslSocket->state() 
                         << "Encrypted:" << sslEncrypted 
                         << "Socket connected:" << socketConnected 
                         << "Final result:" << (socketConnected && sslEncrypted);
    
    return socketConnected && sslEncrypted;
}

bool NetworkClient::isSslEncrypted() const
{
    return _sslSocket && _sslSocket->isEncrypted();
}

// 用户认证
void NetworkClient::login(const QString &usernameOrEmail, const QString &password, const QString &captcha)
{
    QVariantMap data;
    data["type"] = "login";
    data["username"] = usernameOrEmail;
    data["password"] = password;
    if (!captcha.isEmpty()) {
        data["captcha"] = captcha;
    }
    
    QByteArray packet = createPacket("auth", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Login request sent for:" << usernameOrEmail;
}

void NetworkClient::registerUser(const QString &username, const QString &email, const QString &verificationCode, const QString &password, const QUrl &avatar)
{
    QVariantMap data;
    data["type"] = "register";
    data["username"] = username;
    data["email"] = email;
    data["verificationCode"] = verificationCode;
    data["password"] = password;
    
    if (!avatar.isEmpty()) {
        data["avatar"] = avatar.toString();
    }
    
    QByteArray packet = createPacket("auth", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Register request sent for:" << username << "email:" << email;
}

void NetworkClient::logout()
{
    QVariantMap data;
    data["type"] = "logout";
    data["token"] = _authToken;
    
    QByteArray packet = createPacket("auth", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Logout request sent";
}

// 验证码
void NetworkClient::requestCaptcha()
{
    QVariantMap data;
    data["type"] = "request_captcha";
    
    QByteArray packet = createPacket("auth", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Captcha request sent";
}

// 用户名/邮箱可用性检查
void NetworkClient::checkUsernameAvailability(const QString &username)
{
    QVariantMap data;
    data["type"] = "check_username";
    data["username"] = username;
    
    QByteArray packet = createPacket("validation", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Username availability check sent for:" << username;
}

void NetworkClient::checkEmailAvailability(const QString &email)
{
    QVariantMap data;
    data["type"] = "check_email";
    data["email"] = email;
    
    QByteArray packet = createPacket("validation", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Email availability check sent for:" << email;
}

// 邮箱验证
void NetworkClient::sendEmailVerificationCode(const QString &email)
{
    qCInfo(networkClient) << "=== SENDING EMAIL VERIFICATION CODE ===";
    qCInfo(networkClient) << "Email:" << email;
    qCInfo(networkClient) << "Network client state:" << (_sslSocket ? "socket exists" : "socket null");
    qCInfo(networkClient) << "Connection state:" << (_sslSocket ? (_sslSocket->state() == QAbstractSocket::ConnectedState ? "connected" : "not connected") : "no socket");
    qCInfo(networkClient) << "SSL socket state:" << (_sslSocket ? QString::number(static_cast<int>(_sslSocket->state())) : "no socket");
    qCInfo(networkClient) << "SSL socket encrypted:" << (_sslSocket ? (_sslSocket->isEncrypted() ? "true" : "false") : "no socket");
    qCInfo(networkClient) << "Connection state manager:" << (_stateManager ? _stateManager->getStateString(_stateManager->getCurrentState()) : "no state manager");
    
    if (!_sslSocket) {
        qCWarning(networkClient) << "SSL socket is null, cannot send email verification code";
        return;
    }
    
    if (!isConnected()) {
        qCWarning(networkClient) << "Not connected to server, cannot send email verification code";
        return;
    }
    
    QVariantMap data;
    data["action"] = "sendCode";
    data["email"] = email;
    
    qCInfo(networkClient) << "Email verification data:" << data;
    qCInfo(networkClient) << "Data as JSON:" << QJsonDocument(QJsonObject::fromVariantMap(data)).toJson();
    
    QByteArray packet = createPacket("emailVerification", data);
    qCInfo(networkClient) << "Email verification packet created, size:" << packet.size();
    qCInfo(networkClient) << "Packet preview:" << packet.left(50).toHex();
    qCInfo(networkClient) << "Full packet hex:" << packet.toHex();
    qCInfo(networkClient) << "Full packet content:" << packet;
    qCInfo(networkClient) << "Packet as string:" << QString::fromUtf8(packet);
    qCInfo(networkClient) << "Packet length header:" << packet.left(4).toHex();
    qCInfo(networkClient) << "Packet JSON part:" << packet.mid(4);
    
    sendData(packet);
    
    qCInfo(networkClient) << "Email verification code request sent for:" << email;
    qCInfo(networkClient) << "=== END EMAIL VERIFICATION CODE ===";
}

void NetworkClient::verifyEmailCode(const QString &email, const QString &code)
{
    QVariantMap data;
    data["action"] = "verifyCode";
    data["email"] = email;
    data["code"] = code;
    
    QByteArray packet = createPacket("emailVerification", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Email verification code verification sent for:" << email;
}



// 头像上传
void NetworkClient::uploadAvatar(const QUrl &filePath)
{
    QString localPath = filePath.toLocalFile();
    QFile file(localPath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(networkClient) << "Cannot open avatar file:" << localPath;
        emit avatarUploaded(false, QUrl());
        return;
    }
    
    QFileInfo fileInfo(localPath);
    
    // 创建multipart请求
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                       QVariant(QString("form-data; name=\"avatar\"; filename=\"%1\"").arg(fileInfo.fileName())));
    imagePart.setBodyDevice(&file);
    multiPart->append(imagePart);
    
    // 发送HTTP请求
    QNetworkRequest request;
    request.setUrl(QUrl(QString("https://%1:%2/api/upload/avatar").arg(_serverHost).arg(_serverPort + 1)));
    request.setRawHeader("User-Agent", "QKChat Client 1.0");
    
    QNetworkReply *reply = _networkManager->post(request, multiPart);
    multiPart->setParent(reply);
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject obj = doc.object();
            
            if (obj["success"].toBool()) {
                QString avatarUrl = obj["avatarUrl"].toString();
                emit avatarUploaded(true, QUrl(avatarUrl));
                qCInfo(networkClient) << "Avatar uploaded successfully:" << avatarUrl;
            } else {
                emit avatarUploaded(false, QUrl());
                qCWarning(networkClient) << "Avatar upload failed:" << obj["message"].toString();
            }
        } else {
            emit avatarUploaded(false, QUrl());
            qCWarning(networkClient) << "Avatar upload network error:" << reply->errorString();
        }
    });
    
    qCInfo(networkClient) << "Avatar upload started";
}

// 消息发送
void NetworkClient::sendMessage(const QString &receiver, const QString &content, const QString &messageType)
{
    QVariantMap data;
    data["type"] = "send_message";
    data["receiver"] = receiver;
    data["content"] = content;
    data["messageType"] = messageType;
    data["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    
    QByteArray packet = createPacket("message", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Message sent to:" << receiver;
}

// 心跳检测
void NetworkClient::startHeartbeat()
{
    _heartbeatManager->start();
    qCInfo(networkClient) << "Heartbeat started";
    LogManager::instance()->writeHeartbeatLog("MANAGER_STARTED");
}

void NetworkClient::stopHeartbeat()
{
    _heartbeatManager->stop();
    qCInfo(networkClient) << "Heartbeat stopped";
    LogManager::instance()->writeHeartbeatLog("MANAGER_STOPPED");
}

void NetworkClient::sendHeartbeat()
{
    // 这个方法现在由HeartbeatManager内部调用
    // 保留为兼容性，但实际工作由HeartbeatManager完成
    if (!isConnected()) {
        return;
    }

    QVariantMap data;
    data["type"] = "heartbeat";
    data["timestamp"] = QDateTime::currentMSecsSinceEpoch();

    QByteArray packet = createPacket("heartbeat", data);
    sendData(packet);

    qCDebug(networkClient) << "Heartbeat sent (legacy method)";
}

// 私有槽函数
void NetworkClient::onConnected()
{
    _isConnected = true;
    _readBuffer.clear();

    // 通知状态管理器Socket已连接
    _stateManager->triggerEvent(ConnectionStateManager::SocketConnected);

    emit connected();

    // 如果是重连成功，通知重连管理器
    if (_reconnectManager->isReconnecting()) {
        int attempt = _reconnectManager->getCurrentAttempt();
        qint64 totalTime = _reconnectManager->getTotalReconnectTime();
        _reconnectManager->stopReconnect();
        emit _reconnectManager->reconnectSucceeded(attempt, totalTime);
    }

    // 延迟启动心跳
    QTimer::singleShot(2000, this, &NetworkClient::startHeartbeat);
}

void NetworkClient::onDisconnected()
{
    qCInfo(networkClient) << "Disconnection detected";
    qCInfo(networkClient) << "Previous connection status - _isConnected:" << _isConnected;
    qCInfo(networkClient) << "Socket state:" << _sslSocket->state();
    qCInfo(networkClient) << "Socket error:" << _sslSocket->errorString();

    // 记录断开连接的详细信息
    QString disconnectionDetails = QString("PrevConnected: %1, State: %2, Error: %3")
        .arg(_isConnected)
        .arg(_sslSocket->state())
        .arg(_sslSocket->errorString());

    LogManager::instance()->writeConnectionLog("DISCONNECTED", disconnectionDetails);

    // 通知状态管理器连接丢失
    if (_stateManager->isConnected()) {
        _stateManager->triggerEvent(ConnectionStateManager::ConnectionLost);

        // 启动重连
        if (!_reconnectManager->isReconnecting()) {
            _reconnectManager->startReconnect(ReconnectManager::ConnectionLost, "Connection lost");
        }
    } else {
        _stateManager->triggerEvent(ConnectionStateManager::DisconnectRequested);
    }

    _isConnected = false;
    stopHeartbeat();

    qCInfo(networkClient) << "Disconnected from server";
    emit disconnected();
}

void NetworkClient::onSslErrors(const QList<QSslError> &errors)
{
    qCWarning(networkClient) << "SSL errors detected, count:" << errors.size();

    // 使用错误处理器处理SSL错误
    _errorHandler->handleSslError(errors);

    SSLConfigManager* sslManager = SSLConfigManager::instance();

    // 检查是否应该忽略这些错误
    if (sslManager->shouldIgnoreSslErrors(errors)) {
        qCInfo(networkClient) << "Ignoring SSL errors in development mode";
        LogManager::instance()->writeSslLog("ERRORS_IGNORED",
            QString("Ignored %1 SSL errors in development mode").arg(errors.size()));

        _sslSocket->ignoreSslErrors(errors);
    } else {
        // 通知状态管理器发生SSL错误
        _stateManager->triggerEvent(ConnectionStateManager::ErrorOccurred);

        // 构建错误消息
        QString errorMsg;
        for (const QSslError &error : errors) {
            errorMsg += error.errorString() + "; ";
        }

        qCCritical(networkClient) << "SSL errors cannot be ignored:" << errorMsg;
        LogManager::instance()->writeSslLog("SSL Errors Critical", errorMsg, LogManager::CriticalLevel);
        emit connectionError("SSL连接错误: " + errorMsg);
    }
}

void NetworkClient::onSocketReadyRead()
{
    QByteArray newData = _sslSocket->readAll();
    _readBuffer.append(newData);
    
    // 处理完整的数据包
    while (_readBuffer.size() >= 4) {
        // 读取长度前缀
        qint32 packetLength = qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(_readBuffer.constData()));
        
        // 检查是否有完整的数据包
        if (_readBuffer.size() < packetLength + 4) {
            break;
        }
        
        // 提取数据包
        QByteArray packet = _readBuffer.mid(4, packetLength);
        _readBuffer.remove(0, packetLength + 4);
        
        // 解析数据包
        parsePacket(packet);
    }
}

void NetworkClient::onSocketError(QAbstractSocket::SocketError socketError)
{
    QString errorDetails = _sslSocket->errorString();

    qCWarning(networkClient) << "Socket error detected:" << socketError << "-" << errorDetails;

    // 使用错误处理器处理网络错误
    _errorHandler->handleNetworkError(socketError, errorDetails);

    // 通知状态管理器发生错误
    _stateManager->triggerEvent(ConnectionStateManager::ErrorOccurred);

    // 关键修复：当发生套接字错误时，我们不能再信任这个连接。
    // 立即中止连接，这将触发onDisconnected()，从而启动重连逻辑。
    // 这可以防止客户端在连接已损坏但未完全断开时卡住。
    qCInfo(networkClient) << "Aborting socket connection due to error to force reconnection.";
    _sslSocket->abort();
}

void NetworkClient::onHeartbeatTimeout()
{
    qCWarning(networkClient) << "Heartbeat timeout, connection may be lost";

    // 使用错误处理器处理超时错误
    _errorHandler->handleTimeoutError("心跳检测", "心跳响应超时，连接可能已断开");

    // 通知状态管理器连接丢失
    _stateManager->triggerEvent(ConnectionStateManager::ConnectionLost);

    // 启动重连
    if (!_reconnectManager->isReconnecting()) {
        _reconnectManager->startReconnect(ReconnectManager::Timeout, "Heartbeat timeout");
    }
}

void NetworkClient::onNetworkReplyFinished()
{
    // HTTP请求完成的处理
    qCDebug(networkClient) << "Network reply finished";
}

// 私有方法
void NetworkClient::setupSslSocket()
{
    connect(_sslSocket, &QSslSocket::connected, this, &NetworkClient::onConnected);
    connect(_sslSocket, &QSslSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(_sslSocket, &QSslSocket::readyRead, this, &NetworkClient::onSocketReadyRead);
    connect(_sslSocket, &QSslSocket::errorOccurred, this, &NetworkClient::onSocketError);
    connect(_sslSocket, &QSslSocket::sslErrors, this, &NetworkClient::onSslErrors);
    connect(_sslSocket, &QSslSocket::encrypted, this, [this]() {
        qCInfo(networkClient) << "SSL handshake completed successfully";
        qCInfo(networkClient) << "SSL connection established";
        LogManager::instance()->writeSslLog("HANDSHAKE_COMPLETED", "SSL connection established successfully");

        // 通知状态管理器SSL握手完成
        _stateManager->triggerEvent(ConnectionStateManager::SslHandshakeCompleted);
    });

    // 使用SSL配置管理器设置SSL配置
    QSslConfiguration sslConfig = SSLConfigManager::instance()->createSslConfiguration();
    _sslSocket->setSslConfiguration(sslConfig);

    LogManager::instance()->writeSslLog("SOCKET_CONFIGURED",
        QString("SSL socket configured with verification mode: %1")
        .arg(sslConfig.peerVerifyMode()));
}

void NetworkClient::setupStateManager()
{
    // 连接状态管理器的信号
    connect(_stateManager, &ConnectionStateManager::stateChanged,
            this, [this](ConnectionStateManager::ConnectionState oldState, ConnectionStateManager::ConnectionState newState) {
        qCInfo(networkClient) << "Connection state changed from"
                              << _stateManager->getStateString(oldState)
                              << "to" << _stateManager->getStateString(newState);

        LogManager::instance()->writeConnectionLog("STATE_MANAGER_CHANGED",
            QString("From %1 to %2").arg(_stateManager->getStateString(oldState),
                                        _stateManager->getStateString(newState)));
    });

    connect(_stateManager, &ConnectionStateManager::connectionEstablished,
            this, [this]() {
        qCInfo(networkClient) << "Connection established by state manager";
        LogManager::instance()->writeConnectionLog("STATE_MANAGER_ESTABLISHED", "Connection established");
    });

    connect(_stateManager, &ConnectionStateManager::connectionLost,
            this, [this]() {
        qCWarning(networkClient) << "Connection lost detected by state manager";
        LogManager::instance()->writeConnectionLog("STATE_MANAGER_LOST", "Connection lost");
    });

    connect(_stateManager, &ConnectionStateManager::retryAttemptStarted,
            this, [this](int attempt, int maxAttempts) {
        qCInfo(networkClient) << "Retry attempt" << attempt << "of" << maxAttempts;
        LogManager::instance()->writeConnectionLog("RETRY_ATTEMPT",
            QString("Attempt %1/%2").arg(attempt).arg(maxAttempts));
    });

    connect(_stateManager, &ConnectionStateManager::maxRetriesReached,
            this, [this]() {
        qCWarning(networkClient) << "Maximum retry attempts reached";
        LogManager::instance()->writeErrorLog("Maximum retry attempts reached", "NetworkClient");
        emit connectionError("连接失败：已达到最大重试次数");
    });
}

void NetworkClient::setupErrorHandler()
{
    // 连接错误处理器的信号
    connect(_errorHandler, &ErrorHandler::errorOccurred,
            this, [this](const ErrorHandler::ErrorInfo &error) {
        qCWarning(networkClient) << "Error occurred:" << error.message;
        LogManager::instance()->writeErrorLog(
            QString("[%1] %2 - %3").arg(error.type).arg(error.message).arg(error.details),
            "ErrorHandler"
        );
    });

    connect(_errorHandler, &ErrorHandler::criticalErrorOccurred,
            this, [this](const ErrorHandler::ErrorInfo &error) {
        qCCritical(networkClient) << "Critical error occurred:" << error.message;
        emit connectionError(QString("严重错误: %1").arg(error.message));
    });

    connect(_errorHandler, &ErrorHandler::userInterventionRequired,
            this, [this](const ErrorHandler::ErrorInfo &error) {
        qCWarning(networkClient) << "User intervention required:" << error.message;
        emit connectionError(QString("需要用户处理: %1 - %2").arg(error.message, error.solution));
    });

    connect(_errorHandler, &ErrorHandler::recoveryAttempted,
            this, [this](const ErrorHandler::ErrorInfo &error, ErrorHandler::RecoveryStrategy strategy) {
        Q_UNUSED(strategy)
        qCInfo(networkClient) << "Recovery attempted for error:" << error.message;
        LogManager::instance()->writeConnectionLog("RECOVERY_ATTEMPTED", error.message);
    });

    connect(_errorHandler, &ErrorHandler::recoverySucceeded,
            this, [this](const ErrorHandler::ErrorInfo &error) {
        qCInfo(networkClient) << "Recovery succeeded for error:" << error.message;
        LogManager::instance()->writeConnectionLog("RECOVERY_SUCCEEDED", error.message);
    });

    connect(_errorHandler, &ErrorHandler::recoveryFailed,
            this, [this](const ErrorHandler::ErrorInfo &error) {
        qCWarning(networkClient) << "Recovery failed for error:" << error.message;
        LogManager::instance()->writeConnectionLog("RECOVERY_FAILED", error.message);
    });

    // 设置开发模式
    _errorHandler->setDevelopmentMode(SSLConfigManager::instance()->isDevelopmentModeEnabled());
}

void NetworkClient::setupReconnectManager()
{
    // 连接重连管理器的信号
    connect(_reconnectManager, &ReconnectManager::reconnectStarted,
            this, [this](ReconnectManager::ReconnectTrigger trigger, const QString &reason) {
        Q_UNUSED(trigger)
        qCInfo(networkClient) << "Reconnect started:" << reason;
        LogManager::instance()->writeConnectionLog("RECONNECT_STARTED", reason);
    });

    connect(_reconnectManager, &ReconnectManager::reconnectAttempt,
            this, [this](int attempt, int maxAttempts, int delayMs) {
        qCInfo(networkClient) << "Reconnect attempt" << attempt << "of" << maxAttempts
                              << "next delay:" << delayMs << "ms";
        LogManager::instance()->writeConnectionLog("RECONNECT_ATTEMPT",
            QString("Attempt %1/%2, Delay: %3ms").arg(attempt).arg(maxAttempts).arg(delayMs));

        // 实际执行重连
        if (!_stateManager->isConnecting() && !_stateManager->isConnected()) {
            connectToServer(_serverHost, _serverPort);
        }
    });

    connect(_reconnectManager, &ReconnectManager::reconnectSucceeded,
            this, [this](int attempt, qint64 totalTime) {
        qCInfo(networkClient) << "Reconnect succeeded after" << attempt << "attempts in" << totalTime << "ms";
        LogManager::instance()->writeConnectionLog("RECONNECT_SUCCEEDED",
            QString("Attempts: %1, Time: %2ms").arg(attempt).arg(totalTime));
    });

    connect(_reconnectManager, &ReconnectManager::reconnectFailed,
            this, [this](int attempt, const QString &reason) {
        qCWarning(networkClient) << "Reconnect attempt" << attempt << "failed:" << reason;
        LogManager::instance()->writeConnectionLog("RECONNECT_FAILED",
            QString("Attempt %1: %2").arg(attempt).arg(reason));
    });

    connect(_reconnectManager, &ReconnectManager::maxAttemptsReached,
            this, [this]() {
        qCWarning(networkClient) << "Maximum reconnect attempts reached";
        LogManager::instance()->writeErrorLog("Maximum reconnect attempts reached", "NetworkClient");
        emit connectionError("重连失败：已达到最大重试次数");
    });

    connect(_reconnectManager, &ReconnectManager::networkStatusChanged,
            this, [this](bool available) {
        qCInfo(networkClient) << "Network status changed:" << (available ? "Available" : "Unavailable");
        LogManager::instance()->writeConnectionLog("NETWORK_STATUS",
            available ? "Available" : "Unavailable");

        if (available && !_stateManager->isConnected() && !_reconnectManager->isReconnecting()) {
            // 网络恢复且未连接，启动重连
            _reconnectManager->startReconnect(ReconnectManager::NetworkError, "Network recovered");
        }
    });

    // 从开发环境配置获取重连参数
    DevelopmentConfig* devConfig = DevelopmentConfig::instance();
    _reconnectManager->setMaxAttempts(devConfig->getMaxRetryAttempts());
    _reconnectManager->setBaseInterval(devConfig->getRetryInterval());
    _reconnectManager->setMaxInterval(60000);  // 60秒
    _reconnectManager->setBackoffMultiplier(1.5);
    _reconnectManager->setStrategy(ReconnectManager::ExponentialBackoff);

    // 监听配置变化
    connect(devConfig, &DevelopmentConfig::configurationChanged,
            this, [this]() {
        updateFromDevelopmentConfig();
    });
}

void NetworkClient::setupHeartbeatManager()
{
    // 连接心跳管理器的信号
    connect(_heartbeatManager, &HeartbeatManager::heartbeatSent,
            this, [this](const QDateTime &timestamp) {
        Q_UNUSED(timestamp)
        // 发送心跳数据包 - 使用简单的heartbeat格式
        QVariantMap heartbeatData;
        heartbeatData["timestamp"] = QDateTime::currentMSecsSinceEpoch();
        heartbeatData["client_id"] = "client_001"; // 可以从配置获取

        QByteArray packet = createPacket("heartbeat", heartbeatData);
        sendData(packet);

        qCDebug(networkClient) << "Heartbeat packet sent";
    });

    connect(_heartbeatManager, &HeartbeatManager::heartbeatReceived,
            this, [this](const QDateTime &timestamp, qint64 latency) {
        Q_UNUSED(timestamp)
        qCDebug(networkClient) << "Heartbeat response received, latency:" << latency << "ms";
        LogManager::instance()->writeHeartbeatLog("RESPONSE_RECEIVED", latency);
    });

    connect(_heartbeatManager, &HeartbeatManager::heartbeatTimeout,
            this, [this]() {
        qCWarning(networkClient) << "Heartbeat timeout detected";
        LogManager::instance()->writeHeartbeatLog("TIMEOUT_DETECTED");

        // 检查是否达到最大超时次数
        if (_heartbeatManager->getMissedBeats() >= _heartbeatManager->getMaxMissedBeats()) {
            onHeartbeatTimeout();
        }
    });

    connect(_heartbeatManager, &HeartbeatManager::maxMissedBeatsReached,
            this, [this]() {
        qCCritical(networkClient) << "Maximum missed heartbeats reached";
        onHeartbeatTimeout();
    });

    connect(_heartbeatManager, &HeartbeatManager::connectionQualityChanged,
            this, [this](HeartbeatManager::ConnectionQuality quality) {
        qCInfo(networkClient) << "Connection quality changed to:" << static_cast<int>(quality);
        LogManager::instance()->writeHeartbeatLog("QUALITY_CHANGED", static_cast<int>(quality));
    });

    connect(_heartbeatManager, &HeartbeatManager::latencyChanged,
            this, [this](qint64 latency) {
        // 可以根据延迟调整其他参数
        if (latency > 1000) { // 1秒
            qCWarning(networkClient) << "High latency detected:" << latency << "ms";
        }
    });

    // 从开发环境配置获取心跳参数
    DevelopmentConfig* devConfig = DevelopmentConfig::instance();
    _heartbeatManager->setInterval(devConfig->getHeartbeatInterval());
    _heartbeatManager->setTimeout(devConfig->getConnectionTimeout() / 3);   // 超时时间为连接超时的1/3
    _heartbeatManager->setMaxMissedBeats(3); // 最多3次超时
    _heartbeatManager->setAdaptiveMode(true); // 启用自适应模式
    _heartbeatManager->setLatencyThreshold(200); // 200ms延迟阈值
}

void NetworkClient::updateFromDevelopmentConfig()
{
    DevelopmentConfig* devConfig = DevelopmentConfig::instance();

    // 更新重连参数
    if (_reconnectManager) {
        _reconnectManager->setMaxAttempts(devConfig->getMaxRetryAttempts());
        _reconnectManager->setBaseInterval(devConfig->getRetryInterval());

        qCInfo(networkClient) << "Reconnect parameters updated from config";
        LogManager::instance()->writeConnectionLog("CONFIG_UPDATED",
            QString("MaxAttempts: %1, RetryInterval: %2")
            .arg(devConfig->getMaxRetryAttempts())
            .arg(devConfig->getRetryInterval()));
    }

    // 更新心跳参数
    if (_heartbeatManager) {
        _heartbeatManager->setInterval(devConfig->getHeartbeatInterval());
        _heartbeatManager->setTimeout(devConfig->getConnectionTimeout() / 3);

        qCInfo(networkClient) << "Heartbeat parameters updated from config";
        LogManager::instance()->writeHeartbeatLog("CONFIG_UPDATED", devConfig->getHeartbeatInterval());
    }

    // 更新错误处理器的开发模式
    if (_errorHandler) {
        _errorHandler->setDevelopmentMode(devConfig->isDevelopmentMode());

        qCInfo(networkClient) << "Error handler development mode updated:" << devConfig->isDevelopmentMode();
    }
}

void NetworkClient::processIncomingData()
{
    while (_readBuffer.size() >= 4) {
        // 协议格式：4字节长度 + JSON数据
        QByteArray lengthBytes = _readBuffer.left(4);
        qint32 packetLength = qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(lengthBytes.data()));
        
        if (packetLength <= 0 || packetLength > 1024 * 1024) { // 最大1MB
            qCWarning(networkClient) << "Invalid packet length:" << packetLength;
            _readBuffer.clear();
            return;
        }
        
        if (_readBuffer.size() < 4 + packetLength) {
            // 数据还不完整，等待更多数据
            break;
        }
        
        // 提取完整的数据包
        QByteArray packetData = _readBuffer.mid(4, packetLength);
        _readBuffer.remove(0, 4 + packetLength);
        
        // 解析数据包
        parsePacket(packetData);
    }
}

void NetworkClient::sendData(const QByteArray &data)
{
    // 添加线程安全保护
    _sendMutex->lock();
    
    qCInfo(networkClient) << "=== SENDING DATA ===";
    qCInfo(networkClient) << "Data size:" << data.size() << "bytes";
    qCInfo(networkClient) << "Data preview:" << data.left(100).toHex();
    
    if (!isConnected()) {
        qCWarning(networkClient) << "Cannot send data - socket not connected or not encrypted.";
        qCInfo(networkClient) << "Connection state:" << _stateManager->getCurrentState();
        qCInfo(networkClient) << "Socket state:" << _sslSocket->state();
        qCInfo(networkClient) << "Socket encrypted:" << _sslSocket->isEncrypted();
        qCInfo(networkClient) << "=== END SENDING DATA (FAILED) ===";
        _sendMutex->unlock();
        return;
    }

    qCInfo(networkClient) << "Socket connected and encrypted, writing data...";
    qCInfo(networkClient) << "Socket bytes to write before:" << _sslSocket->bytesToWrite();

    // 写入数据到socket缓冲区
    qint64 bytesWritten = _sslSocket->write(data);
    
    if (bytesWritten == -1) {
        qCWarning(networkClient) << "Failed to write data to socket:" << _sslSocket->errorString();
        qCInfo(networkClient) << "=== END SENDING DATA (FAILED) ===";
        _sendMutex->unlock();
        return;
    }
    
    // 等待数据被发送（最多等待1秒）
    if (!_sslSocket->waitForBytesWritten(1000)) {
        qCWarning(networkClient) << "Timeout waiting for data to be written";
        qCInfo(networkClient) << "=== END SENDING DATA (TIMEOUT) ===";
        _sendMutex->unlock();
        return;
    }
    
    qCInfo(networkClient) << "Data written to socket buffer successfully. Total bytes written:" << bytesWritten;
    qCInfo(networkClient) << "Socket bytes to write after:" << _sslSocket->bytesToWrite();
    qCInfo(networkClient) << "=== END SENDING DATA (SUCCESS) ===";
    
    _sendMutex->unlock();
}

QByteArray NetworkClient::createPacket(const QString &type, const QVariantMap &data)
{
    QVariantMap packet;
    packet["type"] = type;
    packet["data"] = data;
    packet["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    
    QJsonDocument doc = QJsonDocument::fromVariant(packet);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 添加长度前缀
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), reinterpret_cast<uchar*>(lengthBytes.data()));
    
    return lengthBytes + jsonData;
}

void NetworkClient::parsePacket(const QByteArray &packet)
{
    qCInfo(networkClient) << "Parsing packet, size:" << packet.size() << "bytes";
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(packet, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(networkClient) << "JSON parse error:" << parseError.errorString();
        return;
    }
    
    if (!doc.isObject()) {
        qCWarning(networkClient) << "Packet is not a JSON object";
        return;
    }
    
    QVariantMap packetMap = doc.toVariant().toMap();
    QString type = packetMap["type"].toString();
    
    qCInfo(networkClient) << "Parsed packet type:" << type;
    qCInfo(networkClient) << "Full packet data:" << packetMap;
    
    // 简化处理逻辑：直接根据类型处理
    if (type == "validation") {
        // 统一处理validation类型的响应
        qCInfo(networkClient) << "Processing validation response";
        handleValidationResponse(packetMap);
    } else if (type == "register" || type == "login") {
        // 认证相关响应
        qCInfo(networkClient) << "Processing auth response for type:" << type;
        QJsonObject authData = QJsonObject::fromVariantMap(packetMap);
        handleAuthResponse(authData);
    } else if (type == "heartbeat") {
        // 心跳响应
        qCInfo(networkClient) << "Processing heartbeat response";
        handleHeartbeatResponse(packetMap);
    } else if (type == "auth") {
        // 包装的认证响应（兼容旧格式）
        qCInfo(networkClient) << "Processing wrapped auth response";
        QVariantMap data = packetMap["data"].toMap();
        QJsonObject authData = QJsonObject::fromVariantMap(data);
        handleAuthResponse(authData);
    } else if (type == "message") {
        qCInfo(networkClient) << "Processing message response";
        handleMessageResponse(packetMap);
    } else if (type == "emailVerification") {
        qCInfo(networkClient) << "Processing email verification response";
        handleEmailVerificationResponse(packetMap);
    } else if (type == "emailCodeSent") {
        qCInfo(networkClient) << "Processing email code sent response";
        handleEmailCodeSentResponse(packetMap);
    } else if (type == "error") {
        qCInfo(networkClient) << "Processing error response";
        handleErrorResponse(packetMap);
    } else {
        qCWarning(networkClient) << "Unknown packet type:" << type;
    }
}

void NetworkClient::handleAuthResponse(const QJsonObject& response)
{
    QString type = response["type"].toString();
    
    if (type == "register") {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        emit registerResponse(success, message);
    } else if (type == "login") {
        bool success = response["success"].toBool();
        QString message = response["message"].toString();
        emit loginResponse(success, message);
    }
}

void NetworkClient::handleMessageResponse(const QVariantMap &data)
{
    QString msgType = data["type"].toString();
    
    if (msgType == "message_received") {
        QString sender = data["sender"].toString();
        QString content = data["content"].toString();
        QString messageType = data["messageType"].toString();
        qint64 timestamp = data["timestamp"].toLongLong();
        
        emit messageReceived(sender, content, messageType, timestamp);
    } else if (msgType == "message_sent") {
        QString messageId = data["messageId"].toString();
        emit messageSent(messageId);
    } else if (msgType == "message_delivered") {
        QString messageId = data["messageId"].toString();
        emit messageDelivered(messageId);
    }
}

void NetworkClient::handleValidationResponse(const QVariantMap &data)
{
    qCInfo(networkClient) << "Handling validation response:" << data;
    
    QString validationType = data["validationType"].toString();
    qCInfo(networkClient) << "Validation type:" << validationType;
    
    if (validationType == "username") {
        bool available = data["available"].toBool();
        qCInfo(networkClient) << "Username availability result:" << available;
        emit usernameAvailability(available);
    } else if (validationType == "email") {
        bool available = data["available"].toBool();
        qCInfo(networkClient) << "Email availability result:" << available;
        emit emailAvailability(available);
    } else {
        qCWarning(networkClient) << "Unknown validation type:" << validationType;
    }
}

void NetworkClient::handleHeartbeatResponse(const QVariantMap &data)
{
    // 提取服务器时间戳（如果有）
    QDateTime serverTime;
    if (data.contains("server_timestamp")) {
        qint64 timestamp = data["server_timestamp"].toLongLong();
        serverTime = QDateTime::fromMSecsSinceEpoch(timestamp);
    }

    // 通知心跳管理器收到响应
    _heartbeatManager->handleHeartbeatResponse(serverTime);

    qCDebug(networkClient) << "Heartbeat response processed by HeartbeatManager";
}

void NetworkClient::handleErrorResponse(const QVariantMap &data)
{
    QString error = data["message"].toString();
    qCWarning(networkClient) << "Server error:" << error;
    emit networkError(error);
}

void NetworkClient::handleEmailVerificationResponse(const QVariantMap &data)
{
    bool success = data["success"].toBool();
    QString message = data["message"].toString();
    
    qCInfo(networkClient) << "Email verification response:" << success << message;
    emit emailVerificationCodeVerified(success, message);
}

void NetworkClient::handleEmailCodeSentResponse(const QVariantMap &data)
{
    bool success = data["success"].toBool();
    QString message = data["message"].toString();
    
    qCInfo(networkClient) << "Email code sent response:" << success << message;
    
    // 改进错误处理，提供更友好的错误信息
    if (!success) {
        if (message.contains("连接失败") || message.contains("超时")) {
            message = "网络连接失败，请检查网络设置后重试";
        } else if (message.contains("认证失败")) {
            message = "邮箱认证失败，请检查邮箱配置";
        } else if (message.contains("存储失败")) {
            message = "系统繁忙，请稍后重试";
        }
    }
    
    emit emailVerificationCodeSent(success, message);
}

