#include "NetworkClient.h"
#include "../config/ConfigManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QDataStream>
#include <QDateTime>
#include <QVariantMap>
#include <QUuid>

Q_LOGGING_CATEGORY(networkClient, "qkchat.client.networkclient")

const int NetworkClient::HEARTBEAT_INTERVAL;
const int NetworkClient::CONNECTION_TIMEOUT;

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
    , _sslSocket(nullptr)
    , _isConnected(false)
{
    _sslSocket = new QSslSocket(this);
    _heartbeatTimer = new QTimer(this);
    _networkManager = new QNetworkAccessManager(this);
    
    setupSslSocket();
    
    // 配置心跳计时器
    _heartbeatTimer->setSingleShot(false);
    _heartbeatTimer->setInterval(HEARTBEAT_INTERVAL);
    
    connect(_heartbeatTimer, &QTimer::timeout, this, &NetworkClient::onHeartbeatTimeout);
    
    qCInfo(networkClient) << "NetworkClient initialized";
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

bool NetworkClient::connectToServer(const QString &host, int port)
{
    if (_isConnected) {
        qCWarning(networkClient) << "Already connected to server";
        return true;
    }
    
    _serverHost = host;
    _serverPort = port;
    
    qCInfo(networkClient) << "Connecting to server:" << host << ":" << port;
    
    // 连接到服务器
    _sslSocket->connectToHostEncrypted(host, port);
    
    // 设置连接超时
    if (!_sslSocket->waitForConnected(CONNECTION_TIMEOUT)) {
        QString error = QString("Failed to connect to server: %1").arg(_sslSocket->errorString());
        qCWarning(networkClient) << error;
        emit connectionError(error);
        return false;
    }
    
    return true;
}

void NetworkClient::disconnect()
{
    if (_isConnected) {
        stopHeartbeat();
        _sslSocket->disconnectFromHost();
        if (_sslSocket->state() != QAbstractSocket::UnconnectedState) {
            _sslSocket->waitForDisconnected(3000);
        }
        _isConnected = false;
        emit disconnected();
        qCInfo(networkClient) << "Disconnected from server";
    }
}

bool NetworkClient::isConnected() const
{
    return _isConnected && _sslSocket->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::login(const QString &usernameOrEmail, const QString &password, const QString &captcha)
{
    QVariantMap data;
    data["username_or_email"] = usernameOrEmail;
    data["password"] = password;
    if (!captcha.isEmpty()) {
        data["captcha"] = captcha;
    }
    
    QByteArray packet = createPacket("login", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Login request sent";
}

void NetworkClient::registerUser(const QString &username, const QString &email, const QString &password, const QUrl &avatar)
{
    QVariantMap data;
    data["username"] = username;
    data["email"] = email;
    data["password"] = password;
    data["avatar"] = avatar.toString();
    
    QByteArray packet = createPacket("register", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Register request sent";
}

void NetworkClient::logout()
{
    QVariantMap data;
    QByteArray packet = createPacket("logout", data);
    sendData(packet);
    
    disconnect();
    
    qCInfo(networkClient) << "Logout request sent";
}

void NetworkClient::requestCaptcha()
{
    QVariantMap data;
    QByteArray packet = createPacket("request_captcha", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Captcha request sent";
}

void NetworkClient::checkUsernameAvailability(const QString &username)
{
    QVariantMap data;
    data["username"] = username;
    
    QByteArray packet = createPacket("check_username", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Username availability check sent:" << username;
}

void NetworkClient::checkEmailAvailability(const QString &email)
{
    QVariantMap data;
    data["email"] = email;
    
    QByteArray packet = createPacket("check_email", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Email availability check sent:" << email;
}

void NetworkClient::uploadAvatar(const QUrl &filePath)
{
    QString localPath = filePath.toLocalFile();
    QFileInfo fileInfo(localPath);
    
    if (!fileInfo.exists()) {
        emit avatarUploaded(false, QUrl());
        return;
    }
    
    // 创建multipart form data
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("image/jpeg"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                       QVariant(QString("form-data; name=\"avatar\"; filename=\"%1\"").arg(fileInfo.fileName())));
    
    QFile *file = new QFile(localPath);
    file->open(QIODevice::ReadOnly);
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    
    multiPart->append(filePart);
    
    // 发送上传请求
    QString uploadUrl = QString("https://%1:%2/api/upload/avatar").arg(_serverHost).arg(_serverPort + 1);
    QNetworkRequest request(uploadUrl);
    
    QNetworkReply *reply = _networkManager->post(request, multiPart);
    multiPart->setParent(reply);
    
    connect(reply, &QNetworkReply::finished, this, &NetworkClient::onNetworkReplyFinished);
    
    qCInfo(networkClient) << "Avatar upload started:" << localPath;
}

void NetworkClient::sendMessage(const QString &receiver, const QString &content, const QString &messageType)
{
    QVariantMap data;
    data["receiver"] = receiver;
    data["content"] = content;
    data["message_type"] = messageType;
    data["message_id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    data["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    QByteArray packet = createPacket("send_message", data);
    sendData(packet);
    
    qCInfo(networkClient) << "Message sent to:" << receiver;
}

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
    if (isConnected()) {
        QVariantMap data;
        data["timestamp"] = QDateTime::currentSecsSinceEpoch();
        
        QByteArray packet = createPacket("heartbeat", data);
        sendData(packet);
    }
}

void NetworkClient::onConnected()
{
    _isConnected = true;
    startHeartbeat();
    emit connected();
    qCInfo(networkClient) << "Connected to server successfully";
}

void NetworkClient::onDisconnected()
{
    _isConnected = false;
    stopHeartbeat();
    emit disconnected();
    qCInfo(networkClient) << "Disconnected from server";
}

void NetworkClient::onSslErrors(const QList<QSslError> &errors)
{
    // 在开发环境中忽略自签名证书错误
    qCWarning(networkClient) << "SSL Errors occurred:";
    for (const QSslError &error : errors) {
        qCWarning(networkClient) << error.errorString();
    }
    
    // 在生产环境中应该验证证书
    _sslSocket->ignoreSslErrors();
}

void NetworkClient::onReadyRead()
{
    QByteArray data = _sslSocket->readAll();
    _readBuffer.append(data);
    
    processIncomingData();
}

void NetworkClient::onSocketError(QAbstractSocket::SocketError socketError)
{
    QString errorString = _sslSocket->errorString();
    qCWarning(networkClient) << "Socket error:" << socketError << errorString;
    
    _isConnected = false;
    stopHeartbeat();
    emit networkError(errorString);
}

void NetworkClient::onHeartbeatTimeout()
{
    sendHeartbeat();
}

void NetworkClient::onNetworkReplyFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;
    
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject obj = doc.object();
        
        if (obj["success"].toBool()) {
            QString avatarUrl = obj["avatar_url"].toString();
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
    
    reply->deleteLater();
}

void NetworkClient::setupSslSocket()
{
    // 连接信号槽
    connect(_sslSocket, &QSslSocket::connected, this, &NetworkClient::onConnected);
    connect(_sslSocket, &QSslSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(_sslSocket, &QSslSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(_sslSocket, &QSslSocket::errorOccurred,
            this, &NetworkClient::onSocketError);
    connect(_sslSocket, &QSslSocket::sslErrors,
            this, &NetworkClient::onSslErrors);
    
    // 配置SSL
    QSslConfiguration sslConfig = _sslSocket->sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone); // 开发环境
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    _sslSocket->setSslConfiguration(sslConfig);
}

void NetworkClient::processIncomingData()
{
    while (_readBuffer.size() >= 8) { // 至少需要包头（8字节）
        // 解析包头：消息类型(4字节) + 消息长度(4字节)
        QByteArray header = _readBuffer.left(8);
        QDataStream headerStream(&header, QIODevice::ReadOnly);
        headerStream.setByteOrder(QDataStream::BigEndian);
        
        quint32 messageType;
        quint32 messageLength;
        headerStream >> messageType >> messageLength;
        
        // 检查是否接收到完整的数据包
        if (_readBuffer.size() < static_cast<int>(8 + messageLength)) {
            break; // 等待更多数据
        }
        
        // 提取消息体
        QByteArray messageBody = _readBuffer.mid(8, messageLength);
        _readBuffer.remove(0, 8 + messageLength);
        
        // 解析消息
        parsePacket(messageBody);
    }
}

void NetworkClient::sendData(const QByteArray &data)
{
    if (isConnected()) {
        qint64 bytesWritten = _sslSocket->write(data);
        if (bytesWritten == -1) {
            qCWarning(networkClient) << "Failed to send data:" << _sslSocket->errorString();
            emit networkError(_sslSocket->errorString());
        } else {
            _sslSocket->flush();
        }
    } else {
        qCWarning(networkClient) << "Cannot send data: not connected";
        emit networkError("未连接到服务器");
    }
}

QByteArray NetworkClient::createPacket(const QString &type, const QVariantMap &data)
{
    QJsonObject jsonObj;
    jsonObj["type"] = type;
    jsonObj["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    for (auto it = data.begin(); it != data.end(); ++it) {
        jsonObj[it.key()] = QJsonValue::fromVariant(it.value());
    }
    
    QJsonDocument doc(jsonObj);
    QByteArray messageBody = doc.toJson(QJsonDocument::Compact);
    
    // 创建包头：消息类型(4字节) + 消息长度(4字节)
    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint32 messageType = qHash(type);
    quint32 messageLength = messageBody.length();
    
    stream << messageType << messageLength;
    packet.append(messageBody);
    
    return packet;
}

void NetworkClient::parsePacket(const QByteArray &packet)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(packet, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qCWarning(networkClient) << "Failed to parse JSON packet:" << error.errorString();
        return;
    }
    
    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    
    if (type == "login_response") {
        bool success = obj["success"].toBool();
        QString message = obj["message"].toString();
        QString token = obj["token"].toString();
        emit loginResponse(success, message, token);
        
    } else if (type == "register_response") {
        bool success = obj["success"].toBool();
        QString message = obj["message"].toString();
        emit registerResponse(success, message);
        
    } else if (type == "logout_response") {
        bool success = obj["success"].toBool();
        emit logoutResponse(success);
        
    } else if (type == "captcha_response") {
        QString captchaImage = obj["captcha_image"].toString();
        emit captchaReceived(captchaImage);
        
    } else if (type == "username_availability") {
        bool available = obj["available"].toBool();
        emit usernameAvailability(available);
        
    } else if (type == "email_availability") {
        bool available = obj["available"].toBool();
        emit emailAvailability(available);
        
    } else if (type == "message_received") {
        QString sender = obj["sender"].toString();
        QString content = obj["content"].toString();
        QString messageType = obj["message_type"].toString();
        qint64 timestamp = obj["timestamp"].toVariant().toLongLong();
        emit messageReceived(sender, content, messageType, timestamp);
        
    } else if (type == "message_sent") {
        QString messageId = obj["message_id"].toString();
        emit messageSent(messageId);
        
    } else if (type == "message_delivered") {
        QString messageId = obj["message_id"].toString();
        emit messageDelivered(messageId);
        
    } else if (type == "heartbeat_response") {
        // 心跳响应，无需特殊处理
        
    } else {
        qCWarning(networkClient) << "Unknown packet type:" << type;
    }
} 