#include "ChatServer.h"
#include "../database/Database.h"
#include "../services/EmailService.h"
#include "../services/EmailTemplate.h"
#include "../network/QSslServer.h"
#include "../core/SessionManager.h"
#include "../network/ProtocolParser.h"
#include "../config/ServerConfig.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QLoggingCategory>
#include <QDateTime>
#include <QByteArray>
#include <QDataStream>
#include <QSqlQuery>
#include <QCryptographicHash>
#include <QtEndian>
#include <QHostAddress>
#include <QTimer>
#include "../utils/ThreadPool.h"
#include <QJsonParseError>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>
#include <QFile>
#include <QCoreApplication>
#include <QRandomGenerator>

Q_DECLARE_LOGGING_CATEGORY(chatServer)
Q_LOGGING_CATEGORY(chatServer, "qkchat.server.chatserver")

// 构造函数
ChatServer::ChatServer(QObject *parent)
    : QObject(parent)
    , _sslServer(nullptr)
    , _database(nullptr)
    , _sessionManager(nullptr)
    , _protocolParser(nullptr)
    , _threadPool(nullptr)
    , _cleanupTimer(nullptr)
    , _host("0.0.0.0")
    , _port(8443)
    , _isRunning(false)
    , _totalMessages(0)
{
    _startTime = QDateTime::currentDateTime();
    setupCleanupTimer();
    qCInfo(chatServer) << "ChatServer initialized";
}

// 析构函数
ChatServer::~ChatServer()
{
    qDebug() << "[ChatServer] Destructor called";
    stopServer();
    if (_threadPool) {
        _threadPool->shutdown();
        delete _threadPool;
        _threadPool = nullptr;
    }
    
    if (_cleanupTimer) {
        _cleanupTimer->stop();
        delete _cleanupTimer;
        _cleanupTimer = nullptr;
    }
    
    qCInfo(chatServer) << "ChatServer destroyed";
}

// 初始化数据库
bool ChatServer::initializeDatabase()
{
    if (!_database) {
        _database = new Database(this);
        if (!_database->initialize()) {
            qCCritical(chatServer) << "Failed to initialize database";
            return false;
        }
    }
    return true;
}

// 启动服务器
bool ChatServer::startServer()
{
    if (_isRunning) {
        qCWarning(chatServer) << "Server is already running";
        return true;
    }
    
    if (!initializeDatabase()) {
        return false;
    }
    
    if (!_sessionManager) {
        _sessionManager = new SessionManager(this);
    }
    
    if (!_protocolParser) {
        _protocolParser = new ProtocolParser(this);
    }
    
    if (!_threadPool) {
        _threadPool = new ThreadPool(10, this);
    }
    
    setupSslServer();
    
    if (!_sslServer->listen(QHostAddress::Any, _port)) {
        qCCritical(chatServer) << "Failed to start SSL server on port" << _port;
        return false;
    }
    
    _isRunning = true;
    _startTime = QDateTime::currentDateTime();
    
    qCInfo(chatServer) << "ChatServer started on port" << _port;
    emit serverStarted();
    
    return true;
}

// 停止服务器
void ChatServer::stopServer()
{
    qDebug() << "[ChatServer] stopServer called";
    if (!_isRunning) {
        return;
    }

    // 1. 停止接受新连接
    if (_sslServer) {
        _sslServer->close();
    }

    // 2. 强制关闭所有客户端连接并清理资源
    QMutexLocker locker(&_clientsMutex);
    qDebug() << "[ChatServer] Forcefully closing" << _clients.size() << "client connections.";

    // 断开所有信号，防止 onClientDisconnected 与我们的手动清理冲突
    for (ClientConnection* client : _clients.values()) {
        if (client && client->socket) {
            client->socket->disconnect(this); // 断开与 ChatServer 对象的所有连接
            client->socket->close();          // 强制关闭 socket
        }
    }

    // 安全地删除所有 ClientConnection 对象
    qDeleteAll(_clients.values());
    _clients.clear();
    _userConnections.clear();

    _isRunning = false;

    qCInfo(chatServer) << "ChatServer stopped";
    emit serverStopped();
}

// 重启服务器
void ChatServer::restartServer()
{
    stopServer();
    startServer();
}

// 检查服务器是否运行
bool ChatServer::isRunning() const
{
    return _isRunning;
}

// 获取在线用户数量
int ChatServer::getOnlineUserCount() const
{
    QMutexLocker locker(&_clientsMutex);
    return _userConnections.size();
}

// 获取连接数量
int ChatServer::getConnectionCount() const
{
    QMutexLocker locker(&_clientsMutex);
    return _clients.size();
}

// 获取已连接用户列表
QStringList ChatServer::getConnectedUsers() const
{
    QMutexLocker locker(&_clientsMutex);
    QStringList users;
    for (auto it = _userConnections.begin(); it != _userConnections.end(); ++it) {
        if (it.value() && it.value()->userId > 0) {
            users << QString::number(it.value()->userId);
        }
    }
    return users;
}

// 获取总用户数量
int ChatServer::getTotalUserCount() const
{
    if (_database) {
        return _database->getTotalUserCount();
    }
    return 0;
}

// 获取运行时间
QString ChatServer::getUptime() const
{
    if (!_isRunning) {
        return "0:00:00";
    }
    
    QDateTime now = QDateTime::currentDateTime();
    qint64 seconds = _startTime.secsTo(now);
    
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    return QString("%1:%2:%3")
           .arg(hours, 2, 10, QChar('0'))
           .arg(minutes, 2, 10, QChar('0'))
           .arg(secs, 2, 10, QChar('0'));
}

// 获取CPU使用率
int ChatServer::getCpuUsage() const
{
    // 简化实现，实际应该读取系统CPU使用率
    return 0;
}

// 获取内存使用率
int ChatServer::getMemoryUsage() const
{
    // 简化实现，实际应该读取系统内存使用率
    return 0;
}

// 发送消息给用户
bool ChatServer::sendMessageToUser(qint64 userId, const QByteArray &message)
{
    QMutexLocker locker(&_clientsMutex);
    
    auto it = _userConnections.find(userId);
    if (it != _userConnections.end() && it.value()) {
        ClientConnection *client = it.value();
        if (client->socket && client->socket->state() == QAbstractSocket::ConnectedState) {
            client->socket->write(message);
            return true;
        }
    }
    return false;
}

// 广播消息
void ChatServer::broadcastMessage(const QByteArray &message)
{
    QMutexLocker locker(&_clientsMutex);
    
    for (auto it = _clients.begin(); it != _clients.end(); ++it) {
        if (it.value() && it.value()->socket && 
            it.value()->socket->state() == QAbstractSocket::ConnectedState) {
            it.value()->socket->write(message);
        }
    }
}

// 设置SSL服务器
void ChatServer::setupSslServer()
{
    if (!_sslServer) {
        _sslServer = new CustomSslServer(this);
        connect(_sslServer, &CustomSslServer::newConnection, 
                this, &ChatServer::onNewConnection);
    }
    
    // 配置SSL
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    
    // 获取应用程序路径
    QString appPath = QCoreApplication::applicationDirPath();

    // 加载证书
    QString certPath = ServerConfig::instance()->getSslCertificateFile();
    QList<QSslCertificate> certificates = QSslCertificate::fromPath(certPath);
    if (certificates.isEmpty()) {
        qCCritical(chatServer) << "Failed to load SSL certificate from" << certPath;
        return;
    }
    sslConfig.setLocalCertificate(certificates.first());

    // 加载私钥
    QString keyPath = ServerConfig::instance()->getSslPrivateKeyFile();
    QFile keyFile(keyPath);
    if (!keyFile.open(QIODevice::ReadOnly)) {
        qCCritical(chatServer) << "Failed to open SSL key file from" << keyPath;
        return;
    }
    QSslKey privateKey(keyFile.readAll(), QSsl::Rsa);
    keyFile.close();

    if (privateKey.isNull()) {
        qCCritical(chatServer) << "Failed to load SSL private key from" << keyPath;
        return;
    }
    sslConfig.setPrivateKey(privateKey);
    
    _sslServer->setSslConfiguration(sslConfig);
}

// 设置清理定时器
void ChatServer::setupCleanupTimer()
{
    if (!_cleanupTimer) {
        _cleanupTimer = new QTimer(this);
        connect(_cleanupTimer, &QTimer::timeout, this, &ChatServer::cleanupConnections);
        _cleanupTimer->start(CLEANUP_INTERVAL);
    }
}

// 处理新连接
void ChatServer::onNewConnection()
{
    qDebug() << "[ChatServer] onNewConnection triggered";
    QSslSocket *socket = qobject_cast<QSslSocket*>(_sslServer->nextPendingConnection());
    if (!socket) {
        qCWarning(chatServer) << "Failed to get SSL socket from pending connection";
        return;
    }
    
    qDebug() << "[ChatServer] New SSL socket created:" << socket;

    connect(socket, &QSslSocket::disconnected, this, &ChatServer::onClientDisconnected);
    connect(socket, &QSslSocket::readyRead, this, &ChatServer::onClientDataReceived);
    connect(socket, &QSslSocket::sslErrors, this, &ChatServer::onSslErrors);
    connect(socket, &QSslSocket::encrypted, this, [this, socket]() {
        qCInfo(chatServer) << "SSL handshake completed for socket:" << socket;
    });
    
    ClientConnection *client = new ClientConnection;
    client->socket = socket;
    client->userId = -1;
    client->lastActivity = QDateTime::currentDateTime();
    
    qDebug() << "[ChatServer] New ClientConnection created:" << client;

    QMutexLocker locker(&_clientsMutex);
    _clients[socket] = client;
    
    qCInfo(chatServer) << "New client connected:" << socket->peerAddress().toString();
    qCInfo(chatServer) << "SSL mode:" << socket->mode();
    emit clientConnected(socket->socketDescriptor());
}

// 处理客户端断开连接
void ChatServer::onClientDisconnected()
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());
    qDebug() << "[ChatServer] onClientDisconnected for socket:" << socket;
    if (socket) {
        removeClient(socket);
        qCInfo(chatServer) << "Client disconnected:" << socket->peerAddress().toString();
        emit clientDisconnected(socket->socketDescriptor());
    }
}

// 处理客户端数据接收
void ChatServer::onClientDataReceived()
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) {
        qCWarning(chatServer) << "onClientDataReceived: sender is not a QSslSocket";
        return;
    }
    
    ClientConnection *client = getClientBySocket(socket);
    if (!client) {
        qCWarning(chatServer) << "onClientDataReceived: no client found for socket";
        return;
    }
    
    client->lastActivity = QDateTime::currentDateTime();
    
    QByteArray data = socket->readAll();
    qCInfo(chatServer) << "Received data from client, size:" << data.size();
    qCInfo(chatServer) << "Client socket:" << socket;
    qCInfo(chatServer) << "Client address:" << socket->peerAddress().toString();
    
    client->readBuffer.append(data);
    qCInfo(chatServer) << "Total buffer size after append:" << client->readBuffer.size();
    
    // 处理完整的数据包
    while (client->readBuffer.size() >= 4) {
        qint32 packetSize = qFromBigEndian<qint32>(client->readBuffer.left(4));
        qCInfo(chatServer) << "Packet size from header:" << packetSize;
        
        if (packetSize <= 0 || packetSize > 1024 * 1024) {
            qCWarning(chatServer) << "Invalid packet size:" << packetSize;
            client->readBuffer.clear();
            return;
        }
        
        if (client->readBuffer.size() >= packetSize + 4) {
            QByteArray packet = client->readBuffer.mid(4, packetSize);
            client->readBuffer.remove(0, packetSize + 4);
            
            qCInfo(chatServer) << "Processing packet, size:" << packet.size();
            processClientMessage(client, packet);
        } else {
            qCInfo(chatServer) << "Incomplete packet, waiting for more data";
            break; // 等待更多数据
        }
    }
}

// 处理SSL错误
void ChatServer::onSslErrors(const QList<QSslError> &errors)
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());
    if (socket) {
        qCWarning(chatServer) << "SSL errors for" << socket->peerAddress().toString() << ":" << errors;
        // 在生产环境中，应该验证证书
        socket->ignoreSslErrors();
    }
}

// 清理连接
void ChatServer::cleanupConnections()
{
    qDebug() << "[ChatServer] cleanupConnections triggered.";
    QMutexLocker locker(&_clientsMutex);
    if (_clients.isEmpty()) {
        return;
    }

    QDateTime now = QDateTime::currentDateTime();
    QList<QSslSocket*> socketsToDisconnect;

    // 1. 收集超时的客户端，避免在迭代时修改容器
    for (auto it = _clients.begin(); it != _clients.end(); ++it) {
        ClientConnection *client = it.value();
        if (client && client->socket) {
            if (client->lastActivity.secsTo(now) > HEARTBEAT_TIMEOUT) {
                qCInfo(chatServer) << "Client timeout, scheduling for disconnection:" 
                                   << client->socket->peerAddress().toString();
                socketsToDisconnect.append(client->socket);
            }
        }
    }

    // 2. 对超时的客户端发起断开连接操作
    // disconnectFromHost() 会触发 onClientDisconnected, 由它统一处理清理工作
    for (QSslSocket* socket : socketsToDisconnect) {
        socket->disconnectFromHost();
    }
}

// 处理客户端消息
void ChatServer::processClientMessage(ClientConnection *client, const QByteArray &data)
{
    qDebug() << "[ChatServer] Processing message for client:" << client;
    if (!client) {
        qWarning() << "[ChatServer] processClientMessage called with null client.";
        return;
    }

    _threadPool->enqueue([this, client, data]() {
        qDebug() << "[ChatServer] Thread pool task started for client:" << client;
        qDebug() << "[ChatServer] Received raw data size:" << data.size();
        qDebug() << "[ChatServer] Raw data:" << data;
        
        // 再次检查客户端是否仍然有效，因为它可能在任务开始执行前断开连接
        if (!getClientBySocket(client->socket)) {
            qWarning() << "[ChatServer] Client disconnected before message processing started.";
            return;
        }

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        
        if (error.error != QJsonParseError::NoError) {
            qCWarning(chatServer) << "JSON parse error:" << error.errorString();
            qCWarning(chatServer) << "Raw data:" << data;
            return;
        }
        
        QVariantMap message = doc.toVariant().toMap();
        qDebug() << "[ChatServer] Parsed message:" << message;
        
        // 处理客户端发送的包装消息格式
        QVariantMap actualMessage;
        if (message.contains("data")) {
            // 客户端发送的是包装格式：{type: "auth", data: {...}, timestamp: ...}
            actualMessage = message["data"].toMap();
            qDebug() << "[ChatServer] Extracted data from wrapper:" << actualMessage;
        } else {
            // 直接格式：{type: "register", ...}
            actualMessage = message;
            qDebug() << "[ChatServer] Using direct message format:" << actualMessage;
        }
        
        QString type = actualMessage["type"].toString();
        
        qDebug() << "[ChatServer] Message type:" << type << "for client:" << client;

        if (type == "register") {
            handleRegisterRequest(client, actualMessage);
        } else if (type == "login") {
            handleLoginRequest(client, actualMessage);
        } else if (type == "verify_email_code") {
            handleEmailCodeVerificationRequest(client, actualMessage);
        } else if (type == "send_verification") {
            handleSendEmailVerificationRequest(client, actualMessage);
        } else if (type == "resend_verification") {
            handleResendVerificationRequest(client, actualMessage);
        } else if (type == "logout") {
            handleLogoutRequest(client);
        } else if (type == "message") {
            handleMessageRequest(client, actualMessage);
        } else if (type == "heartbeat") {
            handleHeartbeat(client);
        } else {
            qCWarning(chatServer) << "Unknown message type:" << type;
        }
        qDebug() << "[ChatServer] Thread pool task finished for client:" << client;
    });
}

// 处理登出请求
void ChatServer::handleLogoutRequest(ClientConnection *client)
{
    if (!client) {
        return;
    }
    
    if (client->userId > 0) {
        _sessionManager->removeUserSessions(client->userId);
        _database->updateUserLastOnline(client->userId);
        
        QMutexLocker locker(&_clientsMutex);
        _userConnections.remove(client->userId);
        
        emit userOffline(client->userId);
        qCInfo(chatServer) << "User logged out:" << client->userId;
    }
    
    if (client->socket) {
        client->socket->disconnectFromHost();
    }
}

// 处理消息请求
void ChatServer::handleMessageRequest(ClientConnection *client, const QVariantMap &data)
{
    if (!client || !_database) {
        return;
    }
    
    qint64 receiverId = data["receiverId"].toLongLong();
    QString messageContent = data["content"].toString();
    QString messageType = data["messageType"].toString();
    if (messageType.isEmpty()) {
        messageType = "text";
    }
    
    if (client->userId <= 0) {
        return; // 用户未登录
    }
    
    // 保存消息到数据库
    QString messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (_database->saveMessage(messageId, client->userId, receiverId, messageType, messageContent, "")) {
        // 发送给接收者
        QVariantMap messageData;
        messageData["type"] = "message";
        messageData["messageId"] = messageId;
        messageData["senderId"] = client->userId;
        messageData["content"] = messageContent;
        messageData["messageType"] = messageType;
        messageData["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        QJsonDocument doc = QJsonDocument::fromVariant(messageData);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        QByteArray lengthBytes(4, 0);
        qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
        QByteArray packet = lengthBytes + jsonData;
        
        sendMessageToUser(receiverId, packet);
        
        emit messageReceived(client->userId, receiverId, messageContent);
        qCInfo(chatServer) << "Message sent from" << client->userId << "to" << receiverId;
        
        QMutexLocker locker(&_statsMutex);
        _totalMessages++;
    }
}

// 处理心跳
void ChatServer::handleHeartbeat(ClientConnection *client)
{
    if (client) {
        client->lastActivity = QDateTime::currentDateTime();
        
        // 发送心跳响应
        QVariantMap response;
        response["type"] = "heartbeat";
        response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        
        QJsonDocument doc = QJsonDocument::fromVariant(response);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        QByteArray lengthBytes(4, 0);
        qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
        QByteArray packet = lengthBytes + jsonData;
        
        client->socket->write(packet);
    }
}

// 移除客户端
void ChatServer::removeClient(QSslSocket *socket)
{
    QMutexLocker locker(&_clientsMutex);
    qDebug() << "[ChatServer] Attempting to remove client with socket:" << socket;
    
    auto it = _clients.find(socket);
    if (it != _clients.end()) {
        ClientConnection *client = it.value();
        qDebug() << "[ChatServer] Found client connection:" << client << "with user ID:" << client->userId;
        if (client->userId > 0) {
            _userConnections.remove(client->userId);
            emit userOffline(client->userId);
            qDebug() << "[ChatServer] Removed user from online list:" << client->userId;
        }
        
        _clients.erase(it); // 使用 erase(it) 更安全
        delete client; // 确保在从 map 中移除后再删除
        qDebug() << "[ChatServer] Client connection and socket removed successfully.";
    } else {
        qWarning() << "[ChatServer] removeClient: Socket not found in clients map:" << socket;
    }
}

// 根据socket获取客户端
ChatServer::ClientConnection *ChatServer::getClientBySocket(QSslSocket *socket)
{
    QMutexLocker locker(&_clientsMutex);
    auto it = _clients.find(socket);
    return (it != _clients.end()) ? it.value() : nullptr;
}

// 根据用户ID获取客户端
ChatServer::ClientConnection *ChatServer::getClientByUserId(qint64 userId)
{
    QMutexLocker locker(&_clientsMutex);
    auto it = _userConnections.find(userId);
    return (it != _userConnections.end()) ? it.value() : nullptr;
}

void ChatServer::handleRegisterRequest(ClientConnection *client, const QVariantMap &data)
{
    if (!client || !_database) {
        return;
    }
    
    QString username = data["username"].toString();
    QString email = data["email"].toString();
    QString password = data["password"].toString();
    QString avatarUrl = data["avatar"].toString();
    
    QVariantMap response;
    response["type"] = "auth";
    
    // 检查用户名和邮箱是否可用
    if (!_database->isUsernameAvailable(username)) {
        QVariantMap responseData;
        responseData["type"] = "register";
        responseData["success"] = false;
        responseData["message"] = "用户名已被使用";
        response["data"] = responseData;
    } else if (!_database->isEmailAvailable(email)) {
        QVariantMap responseData;
        responseData["type"] = "register";
        responseData["success"] = false;
        responseData["message"] = "邮箱已被使用";
        response["data"] = responseData;
    } else {
        // 创建用户
        if (_database->createUser(username, email, password, avatarUrl)) {
            // 获取新创建用户的ID
            Database::UserInfo newUser = _database->getUserByUsername(username);
            if (newUser.id > 0) {
                // 生成邮箱验证令牌
                QString verificationToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
                
                // 创建邮箱验证记录
                if (_database->createEmailVerification(newUser.id, email, verificationToken)) {
                    // 发送验证邮件
                    QString verificationLink = QString("http://localhost:8080/verify-email?token=%1").arg(verificationToken);
                    EmailService::instance().sendEmailAsync(EmailMessage(
                        email,
                        "欢迎注册 QKChat - 请验证您的邮箱",
                        EmailTemplate::getRegisterVerificationEmail(username, verificationLink)
                    ));
                    
                    QVariantMap responseData;
                    responseData["type"] = "register";
                    responseData["success"] = true;
                    responseData["message"] = "注册成功，请检查您的邮箱完成验证";
                    responseData["requiresVerification"] = true;
                    response["data"] = responseData;
                    
                    qCInfo(chatServer) << "User registered and verification email sent:" << username;
                    
                    // 记录注册日志
                    _database->logEvent(Database::Info, "auth", "User registered with email verification", 
                                      newUser.id, client->socket->peerAddress().toString(), "", 
                                      QVariantMap{{"username", username}, {"email", email}});
                } else {
                    QVariantMap responseData;
                    responseData["type"] = "register";
                    responseData["success"] = false;
                    responseData["message"] = "注册成功，但发送验证邮件失败";
                    response["data"] = responseData;
                }
            }
        } else {
            QVariantMap responseData;
            responseData["type"] = "register";
            responseData["success"] = false;
            responseData["message"] = "注册失败";
            response["data"] = responseData;
        }
    }
    
    // 发送响应
    QJsonDocument doc = QJsonDocument::fromVariant(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
    QByteArray packet = lengthBytes + jsonData;
    
    client->socket->write(packet);
    client->socket->flush();
}

void ChatServer::handleEmailVerificationRequest(ClientConnection *client, const QVariantMap &data)
{
    if (!client || !_database) {
        return;
    }
    
    QString email = data["email"].toString();
    QString code = data["code"].toString();
    
    QVariantMap response;
    response["type"] = "auth";
    
    // 验证邮箱验证码
    if (_database->verifyEmailCode(email, code)) {
        // 获取用户信息
        Database::UserInfo userInfo = _database->getUserByEmail(email);
        if (userInfo.id > 0) {
            // 更新用户邮箱验证状态
            if (_database->updateUserEmailVerification(userInfo.id, true)) {
                QVariantMap responseData;
                responseData["type"] = "verify_email_code";
                responseData["success"] = true;
                responseData["message"] = "邮箱验证成功";
                response["data"] = responseData;
                
                qCInfo(chatServer) << "Email code verified for user ID:" << userInfo.id;
            } else {
                QVariantMap responseData;
                responseData["type"] = "verify_email_code";
                responseData["success"] = false;
                responseData["message"] = "邮箱验证失败";
                response["data"] = responseData;
            }
        } else {
            QVariantMap responseData;
            responseData["type"] = "verify_email_code";
            responseData["success"] = false;
            responseData["message"] = "用户不存在";
            response["data"] = responseData;
        }
    } else {
        QVariantMap responseData;
        responseData["type"] = "verify_email_code";
        responseData["success"] = false;
        responseData["message"] = "验证码错误或已过期";
        response["data"] = responseData;
    }
    
    // 发送响应
    QJsonDocument doc = QJsonDocument::fromVariant(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
    QByteArray packet = lengthBytes + jsonData;
    
    client->socket->write(packet);
    client->socket->flush();
}

void ChatServer::handleSendEmailVerificationRequest(ClientConnection *client, const QVariantMap &data)
{
    if (!client || !_database) {
        return;
    }
    
    QString email = data["email"].toString();
    
    QVariantMap response;
    response["type"] = "auth";
    
    // 检查邮箱是否已注册
    Database::UserInfo userInfo = _database->getUserByEmail(email);
    if (userInfo.id > 0) {
        // 生成验证码
        QString verificationCode = QString::number(QRandomGenerator::global()->bounded(100000, 999999));
        
        // 保存验证码到数据库
        if (_database->saveEmailVerificationCode(email, verificationCode)) {
            // 发送验证邮件
            EmailService::instance().sendEmailAsync(EmailMessage(
                email,
                "QKChat - 邮箱验证码",
                EmailTemplate::getEmailVerificationCodeEmail(userInfo.username, verificationCode)
            ));
            
            QVariantMap responseData;
            responseData["type"] = "send_verification";
            responseData["success"] = true;
            responseData["message"] = "验证码已发送";
            response["data"] = responseData;
            
            qCInfo(chatServer) << "Email verification code sent to:" << email;
        } else {
            QVariantMap responseData;
            responseData["type"] = "send_verification";
            responseData["success"] = false;
            responseData["message"] = "发送验证码失败";
            response["data"] = responseData;
        }
    } else {
        QVariantMap responseData;
        responseData["type"] = "send_verification";
        responseData["success"] = false;
        responseData["message"] = "邮箱未注册";
        response["data"] = responseData;
    }
    
    // 发送响应
    QJsonDocument doc = QJsonDocument::fromVariant(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
    QByteArray packet = lengthBytes + jsonData;
    
    client->socket->write(packet);
    client->socket->flush();
}

void ChatServer::handleEmailCodeVerificationRequest(ClientConnection *client, const QVariantMap &data)
{
    if (!client || !_database) {
        return;
    }
    
    QString email = data["email"].toString();
    QString code = data["code"].toString();
    
    QVariantMap response;
    response["type"] = "auth";
    
    // 验证邮箱验证码
    if (_database->verifyEmailCode(email, code)) {
        // 获取用户信息
        Database::UserInfo userInfo = _database->getUserByEmail(email);
        if (userInfo.id > 0) {
            // 更新用户邮箱验证状态
            if (_database->updateUserEmailVerification(userInfo.id, true)) {
                QVariantMap responseData;
                responseData["type"] = "verify_email_code";
                responseData["success"] = true;
                responseData["message"] = "邮箱验证成功";
                response["data"] = responseData;
                
                qCInfo(chatServer) << "Email code verified for user ID:" << userInfo.id;
            } else {
                QVariantMap responseData;
                responseData["type"] = "verify_email_code";
                responseData["success"] = false;
                responseData["message"] = "邮箱验证失败";
                response["data"] = responseData;
            }
        } else {
            QVariantMap responseData;
            responseData["type"] = "verify_email_code";
            responseData["success"] = false;
            responseData["message"] = "用户不存在";
            response["data"] = responseData;
        }
    } else {
        QVariantMap responseData;
        responseData["type"] = "verify_email_code";
        responseData["success"] = false;
        responseData["message"] = "验证码错误或已过期";
        response["data"] = responseData;
    }
    
    // 发送响应
    QJsonDocument doc = QJsonDocument::fromVariant(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
    QByteArray packet = lengthBytes + jsonData;
    
    client->socket->write(packet);
    client->socket->flush();
}

void ChatServer::handleResendVerificationRequest(ClientConnection *client, const QVariantMap &data)
{
    if (!client || !_database) {
        return;
    }
    
    QString email = data["email"].toString();
    
    QVariantMap response;
    response["type"] = "auth";
    
    // 获取用户信息
    Database::UserInfo userInfo = _database->getUserByEmail(email);
    if (userInfo.id > 0 && userInfo.status == "unverified") {
        // 生成新的验证码
        QString verificationCode = QString::number(QRandomGenerator::global()->bounded(100000, 999999));
        
        // 保存验证码到数据库
        if (_database->saveEmailVerificationCode(email, verificationCode)) {
            // 发送验证邮件
            EmailService::instance().sendEmailAsync(EmailMessage(
                email,
                "QKChat - 重新发送邮箱验证码",
                EmailTemplate::getEmailVerificationCodeEmail(userInfo.username, verificationCode)
            ));
            
            QVariantMap responseData;
            responseData["type"] = "resend_verification";
            responseData["success"] = true;
            responseData["message"] = "验证码已重新发送";
            response["data"] = responseData;
            
            qCInfo(chatServer) << "Email verification code resent to:" << email;
        } else {
            QVariantMap responseData;
            responseData["type"] = "resend_verification";
            responseData["success"] = false;
            responseData["message"] = "重新发送验证码失败";
            response["data"] = responseData;
        }
    } else {
        QVariantMap responseData;
        responseData["type"] = "resend_verification";
        responseData["success"] = false;
        responseData["message"] = "用户不存在或邮箱已验证";
        response["data"] = responseData;
    }
    
    // 发送响应
    QJsonDocument doc = QJsonDocument::fromVariant(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
    QByteArray packet = lengthBytes + jsonData;
    
    client->socket->write(packet);
    client->socket->flush();
}

void ChatServer::handleLoginRequest(ClientConnection *client, const QVariantMap &data)
{
    if (!client || !_database) {
        return;
    }
    
    QString usernameOrEmail = data["usernameOrEmail"].toString();
    QString password = data["password"].toString();
    QString captcha = data["captcha"].toString();
    
    QVariantMap response;
    response["type"] = "auth";
    
    // 获取用户信息
    Database::UserInfo userInfo;
    if (usernameOrEmail.contains("@")) {
        userInfo = _database->getUserByEmail(usernameOrEmail);
    } else {
        userInfo = _database->getUserByUsername(usernameOrEmail);
    }
    
    if (userInfo.id <= 0) {
        QVariantMap responseData;
        responseData["type"] = "login";
        responseData["success"] = false;
        responseData["message"] = "用户名或密码错误";
        response["data"] = responseData;
    } else if (userInfo.status == "unverified") {
        QVariantMap responseData;
        responseData["type"] = "login";
        responseData["success"] = false;
        responseData["message"] = "请先验证您的邮箱后再登录";
        responseData["requiresVerification"] = true;
        responseData["email"] = userInfo.email;
        response["data"] = responseData;
    } else if (userInfo.status == "banned") {
        QVariantMap responseData;
        responseData["type"] = "login";
        responseData["success"] = false;
        responseData["message"] = "您的账号已被封禁";
        response["data"] = responseData;
    } else {
        // 验证密码
        QString hashedPassword = QString(QCryptographicHash::hash((password + userInfo.salt).toUtf8(), QCryptographicHash::Sha256).toHex());
        if (hashedPassword != userInfo.passwordHash) {
            QVariantMap responseData;
            responseData["type"] = "login";
            responseData["success"] = false;
            responseData["message"] = "用户名或密码错误";
            response["data"] = responseData;
        } else {
            // 创建会话
            QString loginToken = _sessionManager->createSession(userInfo.id, client->socket->peerAddress().toString());
            
            QVariantMap responseData;
            responseData["type"] = "login";
            responseData["success"] = true;
            responseData["message"] = "登录成功";
            responseData["token"] = loginToken;
            responseData["userId"] = userInfo.id;
            responseData["username"] = userInfo.username;
            responseData["email"] = userInfo.email;
            responseData["avatar"] = userInfo.avatarUrl;
            responseData["displayName"] = userInfo.displayName;
            response["data"] = responseData;
            
            qCInfo(chatServer) << "User logged in:" << userInfo.username;
            
            // 更新会话最后活动时间
            _sessionManager->updateSessionLastActive(loginToken);
            
            // 记录登录日志
            _database->logEvent(Database::Info, "auth", "User logged in", 
                              userInfo.id, client->socket->peerAddress().toString(), loginToken);
        }
    }
    
    // 发送响应
    QJsonDocument doc = QJsonDocument::fromVariant(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
    QByteArray packet = lengthBytes + jsonData;
    
    client->socket->write(packet);
    client->socket->flush();
}