#include "NetworkClient.h"
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

Q_LOGGING_CATEGORY(networkClient, "qkchat.client.network")

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
    , _sslSocket(new QSslSocket(this))
    , _heartbeatTimer(new QTimer(this))
    , _networkManager(new QNetworkAccessManager(this))
    , _isConnected(false)
    , _serverPort(0)
{
    setupSslSocket();
    
    // 配置心跳定时器
    _heartbeatTimer->setInterval(HEARTBEAT_INTERVAL);
    connect(_heartbeatTimer, &QTimer::timeout, this, &NetworkClient::sendHeartbeat);
    
    qCInfo(networkClient) << "NetworkClient created";
}

NetworkClient::~NetworkClient()
{
    disconnect();
    qCInfo(networkClient) << "NetworkClient destroyed";
}

// 连接管理
bool NetworkClient::connectToServer(const QString &host, int port)
{
    if (_isConnected) {
        qCWarning(networkClient) << "Already connected to server";
        return true;
    }
    
    _serverHost = host;
    _serverPort = port;
    
    qCInfo(networkClient) << "Connecting to server:" << host << ":" << port;
    
    _sslSocket->connectToHostEncrypted(host, port);
    
    return true;
}

void NetworkClient::disconnect()
{
    if (_sslSocket->state() != QAbstractSocket::UnconnectedState) {
        _sslSocket->disconnectFromHost();
    }
    
    stopHeartbeat();
    _isConnected = false;
    _readBuffer.clear();
    _authToken.clear();
    
    qCInfo(networkClient) << "Disconnected from server";
}

bool NetworkClient::isConnected() const
{
    return _isConnected && _sslSocket->state() == QAbstractSocket::ConnectedState;
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

void NetworkClient::registerUser(const QString &username, const QString &email, const QString &password, const QUrl &avatar)
{
    QVariantMap data;
    data["type"] = "register";
    data["username"] = username;
    data["email"] = email;
    data["password"] = password;
    if (!avatar.isEmpty()) {
        data["avatar"] = avatar.toString();
    }
    
    QByteArray packet = createPacket("auth", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Register request sent for:" << username;
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
    if (!_heartbeatTimer->isActive()) {
        _heartbeatTimer->start();
        qCInfo(networkClient) << "Heartbeat started";
    }
}

void NetworkClient::stopHeartbeat()
{
    if (_heartbeatTimer->isActive()) {
        _heartbeatTimer->stop();
        qCInfo(networkClient) << "Heartbeat stopped";
    }
}

void NetworkClient::sendHeartbeat()
{
    if (!isConnected()) {
        return;
    }
    
    QVariantMap data;
    data["type"] = "heartbeat";
    data["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    
    QByteArray packet = createPacket("heartbeat", data);
    sendData(packet);
    
    qCDebug(networkClient) << "Heartbeat sent";
}

// 私有槽函数
void NetworkClient::onConnected()
{
    _isConnected = true;
    _readBuffer.clear();
    
    qCInfo(networkClient) << "Connected to server successfully";
    emit connected();
    
    // 启动心跳
    startHeartbeat();
}

void NetworkClient::onDisconnected()
{
    _isConnected = false;
    stopHeartbeat();
    
    qCInfo(networkClient) << "Disconnected from server";
    emit disconnected();
}

void NetworkClient::onSslErrors(const QList<QSslError> &errors)
{
    QString errorMsg;
    for (const QSslError &error : errors) {
        errorMsg += error.errorString() + "; ";
    }
    
    qCWarning(networkClient) << "SSL errors:" << errorMsg;
    emit connectionError("SSL连接错误: " + errorMsg);
}

void NetworkClient::onReadyRead()
{
    QByteArray newData = _sslSocket->readAll();
    _readBuffer.append(newData);
    
    processIncomingData();
}

void NetworkClient::onSocketError(QAbstractSocket::SocketError socketError)
{
    QString error;
    switch (socketError) {
    case QAbstractSocket::ConnectionRefusedError:
        error = "连接被拒绝";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        error = "远程主机关闭连接";
        break;
    case QAbstractSocket::HostNotFoundError:
        error = "找不到主机";
        break;
    case QAbstractSocket::SocketTimeoutError:
        error = "连接超时";
        break;
    case QAbstractSocket::NetworkError:
        error = "网络错误";
        break;
    default:
        error = "未知网络错误";
        break;
    }
    
    qCWarning(networkClient) << "Socket error:" << error;
    emit connectionError(error);
}

void NetworkClient::onHeartbeatTimeout()
{
    qCWarning(networkClient) << "Heartbeat timeout, connection may be lost";
    emit connectionError("心跳超时，连接可能已断开");
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
    connect(_sslSocket, &QSslSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(_sslSocket, &QSslSocket::errorOccurred, this, &NetworkClient::onSocketError);
    connect(_sslSocket, &QSslSocket::sslErrors, this, &NetworkClient::onSslErrors);
    
    // 忽略SSL错误（开发环境）
    _sslSocket->ignoreSslErrors();
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
    if (!isConnected()) {
        qCWarning(networkClient) << "Not connected, cannot send data";
        return;
    }
    
    qint64 bytesWritten = _sslSocket->write(data);
    if (bytesWritten == -1) {
        qCWarning(networkClient) << "Failed to send data:" << _sslSocket->errorString();
        return;
    }
    
    _sslSocket->flush();
    qCDebug(networkClient) << "Data sent, bytes:" << bytesWritten;
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
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(packet, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qCWarning(networkClient) << "JSON parse error:" << parseError.errorString();
        return;
    }
    
    if (!doc.isObject()) {
        qCWarning(networkClient) << "Invalid packet format";
        return;
    }
    
    QVariantMap packetMap = doc.toVariant().toMap();
    QString type = packetMap["type"].toString();
    QVariantMap data = packetMap["data"].toMap();
    
    qCDebug(networkClient) << "Packet received, type:" << type;
    
    // 处理不同类型的数据包
    if (type == "auth") {
        handleAuthResponse(data);
    } else if (type == "message") {
        handleMessageResponse(data);
    } else if (type == "validation") {
        handleValidationResponse(data);
    } else if (type == "heartbeat") {
        handleHeartbeatResponse(data);
    } else if (type == "error") {
        handleErrorResponse(data);
    } else {
        qCWarning(networkClient) << "Unknown packet type:" << type;
    }
}

void NetworkClient::handleAuthResponse(const QVariantMap &data)
{
    QString authType = data["type"].toString();
    
    if (authType == "login") {
        bool success = data["success"].toBool();
        QString message = data["message"].toString();
        QString token = data["token"].toString();
        
        emit loginResponse(success, message, token);
    } else if (authType == "register") {
        bool success = data["success"].toBool();
        QString message = data["message"].toString();
        
        emit registerResponse(success, message);
    } else if (authType == "logout") {
        bool success = data["success"].toBool();
        emit logoutResponse(success);
    } else if (authType == "captcha") {
        QString captchaImage = data["captcha"].toString();
        emit captchaReceived(captchaImage);
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
    QString validationType = data["type"].toString();
    
    if (validationType == "check_username") {
        bool available = data["available"].toBool();
        emit usernameAvailability(available);
    } else if (validationType == "check_email") {
        bool available = data["available"].toBool();
        emit emailAvailability(available);
    }
}

void NetworkClient::handleHeartbeatResponse(const QVariantMap &data)
{
    Q_UNUSED(data)
    // 心跳响应，保持连接活跃
    qCDebug(networkClient) << "Heartbeat response received";
}

void NetworkClient::handleErrorResponse(const QVariantMap &data)
{
    QString error = data["message"].toString();
    qCWarning(networkClient) << "Server error:" << error;
    emit networkError(error);
} 