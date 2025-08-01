#include "ChatServer.h"
#include "../database/Database.h"
#include "../core/SessionManager.h"
#include "../network/ProtocolParser.h"
#include "../config/ServerConfig.h"
#include <QSslCertificate>
#include <QSslKey>
#include <QSslConfiguration>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>
#include <QVariantMap>
#include <QThreadPool>
#include <QLoggingCategory>
#include <QDebug>
#include <QProcess>
#include <QTcpServer>
#include <QSslSocket>
#include <QDateTime>
#include <QtEndian>
#include <QCryptographicHash>

Q_LOGGING_CATEGORY(chatServer, "qkchat.server.chatserver")

ChatServer::ChatServer(QObject *parent)
    : QObject(parent)
    , _sslServer(new CustomSslServer(this))
    , _database(new Database(this))
    , _sessionManager(new SessionManager(this))
    , _protocolParser(new ProtocolParser(this))
    , _threadPool(QThreadPool::globalInstance())
    , _cleanupTimer(new QTimer(this))
    , _host("0.0.0.0")
    , _port(8888)
    , _isRunning(false)
    , _startTime(QDateTime::currentDateTime())
    , _totalMessages(0)
{
    setupSslServer();
    setupCleanupTimer();
    
    qCInfo(chatServer) << "ChatServer created";
}

ChatServer::~ChatServer()
{
    stopServer();
    qCInfo(chatServer) << "ChatServer destroyed";
}

// 数据库初始化
bool ChatServer::initializeDatabase()
{
    qCInfo(chatServer) << "Initializing database...";
    
    // 初始化数据库连接
    if (!_database->initialize()) {
        qCCritical(chatServer) << "Failed to initialize database connection";
        return false;
    }
    
    // 设置数据库（创建表和默认管理员账号）
    _database->setupDatabase();
    
    qCInfo(chatServer) << "Database initialization completed successfully";
    return true;
}

// 服务器控制
bool ChatServer::startServer()
{
    if (_isRunning) {
        qCWarning(chatServer) << "Server is already running";
        return true;
    }
    
    // 从配置文件获取服务器设置
    ServerConfig *config = ServerConfig::instance();
    _host = config->getServerHost();
    _port = config->getServerPort();
    
    // 启动SSL服务器
    if (!_sslServer->listen(QHostAddress(_host), _port)) {
        QString error = QString("Failed to start server on %1:%2 - %3")
                       .arg(_host).arg(_port).arg(_sslServer->errorString());
        qCCritical(chatServer) << error;
        emit serverError(error);
        return false;
    }
    
    _isRunning = true;
    _startTime = QDateTime::currentDateTime();
    
    // 启动清理定时器
    _cleanupTimer->start();
    
    qCInfo(chatServer) << "Chat server started on" << _host << ":" << _port;
    emit serverStarted();
    
    return true;
}

void ChatServer::stopServer()
{
    if (!_isRunning) {
        return;
    }
    
    qCInfo(chatServer) << "Stopping chat server...";
    
    // 停止接受新连接
    _sslServer->close();
    
    // 停止清理定时器
    _cleanupTimer->stop();
    
    // 断开所有客户端连接
    QMutexLocker locker(&_clientsMutex);
    QList<QSslSocket*> sockets = _clients.keys();
    locker.unlock();
    
    for (QSslSocket *socket : sockets) {
        if (socket->state() == QAbstractSocket::ConnectedState) {
            socket->disconnectFromHost();
        }
    }
    
    // 等待所有连接关闭
    QTimer::singleShot(3000, this, [this]() {
        QMutexLocker locker(&_clientsMutex);
        for (auto it = _clients.begin(); it != _clients.end(); ++it) {
            delete it.value();
            it.key()->deleteLater();
        }
        _clients.clear();
        _userConnections.clear();
    });
    
    _isRunning = false;
    
    qCInfo(chatServer) << "Chat server stopped";
    emit serverStopped();
}

void ChatServer::restartServer()
{
    qCInfo(chatServer) << "Restarting chat server...";
    stopServer();
    QTimer::singleShot(1000, this, &ChatServer::startServer);
}

bool ChatServer::isRunning() const
{
    return _isRunning;
}

// 状态查询
int ChatServer::getOnlineUserCount() const
{
    QMutexLocker locker(&_clientsMutex);
    return _userConnections.size();
}

int ChatServer::getConnectionCount() const
{
    QMutexLocker locker(&_clientsMutex);
    return _clients.size();
}

QStringList ChatServer::getConnectedUsers() const
{
    QMutexLocker locker(&_clientsMutex);
    QStringList users;
    
    for (auto it = _userConnections.begin(); it != _userConnections.end(); ++it) {
        qint64 userId = it.key();
        if (_database) {
            Database::UserInfo userInfo = _database->getUserById(userId);
            if (!userInfo.username.isEmpty()) {
                users.append(userInfo.username);
            }
        }
    }
    
    return users;
}

// Dashboard统计方法
int ChatServer::getTotalUserCount() const
{
    if (_database) {
        return _database->getTotalUserCount();
    }
    return 0;
}

QString ChatServer::getUptime() const
{
    qint64 seconds = _startTime.secsTo(QDateTime::currentDateTime());
    int days = seconds / 86400;
    int hours = (seconds % 86400) / 3600;
    int minutes = (seconds % 3600) / 60;
    
    if (days > 0) {
        return QString("%1天 %2小时 %3分钟").arg(days).arg(hours).arg(minutes);
    } else if (hours > 0) {
        return QString("%1小时 %2分钟").arg(hours).arg(minutes);
    } else {
        return QString("%1分钟").arg(minutes);
    }
}

int ChatServer::getCpuUsage() const
{
    // TODO: 实现CPU使用率获取
    // 这里可以使用系统命令或者第三方库获取CPU使用率
    return 0;
}

int ChatServer::getMemoryUsage() const
{
    // TODO: 实现内存使用率获取
    // 这里可以使用QProcess调用系统命令获取内存使用情况
    return 0;
}

// 消息发送
bool ChatServer::sendMessageToUser(qint64 userId, const QByteArray &message)
{
    QMutexLocker locker(&_clientsMutex);
    
    ClientConnection *client = getClientByUserId(userId);
    if (!client || !client->socket) {
        qCWarning(chatServer) << "User not connected:" << userId;
        return false;
    }
    
    qint64 bytesWritten = client->socket->write(message);
    if (bytesWritten == -1) {
        qCWarning(chatServer) << "Failed to send message to user:" << userId;
        return false;
    }
    
    client->socket->flush();
    client->lastActivity = QDateTime::currentDateTime();
    
    qCDebug(chatServer) << "Message sent to user:" << userId << "bytes:" << bytesWritten;
    return true;
}

void ChatServer::broadcastMessage(const QByteArray &message)
{
    QMutexLocker locker(&_clientsMutex);
    
    int sentCount = 0;
    for (auto it = _clients.begin(); it != _clients.end(); ++it) {
        ClientConnection *client = it.value();
        if (client && client->socket && client->userId > 0) {
            if (client->socket->write(message) != -1) {
                client->socket->flush();
                client->lastActivity = QDateTime::currentDateTime();
                sentCount++;
            }
        }
    }
    
    qCInfo(chatServer) << "Broadcast message sent to" << sentCount << "clients";
}

// 私有槽函数
void ChatServer::onNewConnection()
{
    while (_sslServer->hasPendingConnections()) {
        QSslSocket *socket = qobject_cast<QSslSocket*>(_sslServer->nextPendingConnection());
        if (!socket) {
            continue;
        }
        
        // 创建客户端连接对象
        ClientConnection *client = new ClientConnection();
        client->socket = socket;
        client->userId = 0; // 未认证用户
        client->sessionToken = "";
        client->lastActivity = QDateTime::currentDateTime();
        
        // 连接信号
        connect(socket, &QSslSocket::disconnected, this, &ChatServer::onClientDisconnected);
        connect(socket, &QSslSocket::readyRead, this, &ChatServer::onClientDataReceived);
        connect(socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors),
                this, &ChatServer::onSslErrors);
        
        // 添加到客户端列表
        QMutexLocker locker(&_clientsMutex);
        _clients[socket] = client;
        locker.unlock();
        
        QString clientAddress = QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());
        qCInfo(chatServer) << "New client connected from:" << clientAddress;
        emit clientConnected(reinterpret_cast<qint64>(socket));
        
        // 记录连接日志
        if (_database) {
            _database->logEvent(Database::Info, "connection", "Client connected", 
                              -1, socket->peerAddress().toString(), "", 
                              QVariantMap{{"socket_id", reinterpret_cast<qint64>(socket)}});
        }
    }
}

void ChatServer::onClientDisconnected()
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) {
        return;
    }
    
    removeClient(socket);
}

void ChatServer::onClientDataReceived()
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) {
        return;
    }
    
    QMutexLocker locker(&_clientsMutex);
    ClientConnection *client = getClientBySocket(socket);
    if (!client) {
        locker.unlock();
        qCWarning(chatServer) << "Data received from unknown client";
        return;
    }
    locker.unlock();
    
    // 读取数据
    QByteArray data = socket->readAll();
    client->readBuffer.append(data);
    client->lastActivity = QDateTime::currentDateTime();
    
    // 处理数据包
    processClientMessage(client, client->readBuffer);
}

void ChatServer::onSslErrors(const QList<QSslError> &errors)
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) {
        return;
    }
    
    qCWarning(chatServer) << "SSL errors from client" << socket->peerAddress().toString() << ":";
    for (const QSslError &error : errors) {
        qCWarning(chatServer) << "  -" << error.errorString();
    }
    
    // 在开发环境中忽略SSL错误
    socket->ignoreSslErrors();
}

void ChatServer::cleanupConnections()
{
    QDateTime now = QDateTime::currentDateTime();
    QList<QSslSocket*> timeoutSockets;
    
    // 查找超时的连接
    QMutexLocker locker(&_clientsMutex);
    for (auto it = _clients.begin(); it != _clients.end(); ++it) {
        ClientConnection *client = it.value();
        if (client && client->lastActivity.secsTo(now) > HEARTBEAT_TIMEOUT / 1000) {
            timeoutSockets.append(it.key());
        }
    }
    locker.unlock();
    
    // 移除超时的连接
    for (QSslSocket *socket : timeoutSockets) {
        qCInfo(chatServer) << "Removing timeout connection:" << socket->peerAddress().toString();
        removeClient(socket);
    }
    
    // 清理过期会话
    if (_sessionManager) {
        _sessionManager->cleanExpiredSessions();
    }
    
    if (_database) {
        _database->cleanExpiredSessions();
    }
}

// 私有方法
void ChatServer::setupSslServer()
{
    // 连接新连接信号
    connect(_sslServer, &CustomSslServer::newConnection, this, &ChatServer::onNewConnection);
    
    // 加载SSL证书和私钥
    ServerConfig *config = ServerConfig::instance();
    QString certPath = config->getSslCertificateFile();
    QString keyPath = config->getSslPrivateKeyFile();
    
    if (!certPath.isEmpty() && !keyPath.isEmpty()) {
        QFile certFile(certPath);
        QFile keyFile(keyPath);
        
        if (certFile.open(QIODevice::ReadOnly) && keyFile.open(QIODevice::ReadOnly)) {
            QSslCertificate certificate(&certFile);
            QSslKey privateKey(&keyFile, QSsl::Rsa);
            
            if (!certificate.isNull() && !privateKey.isNull()) {
                _sslServer->setSslConfiguration(QSslConfiguration::defaultConfiguration());
                
                QSslConfiguration sslConfig = _sslServer->sslConfiguration();
                sslConfig.setLocalCertificate(certificate);
                sslConfig.setPrivateKey(privateKey);
                sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
                _sslServer->setSslConfiguration(sslConfig);
                
                qCInfo(chatServer) << "SSL certificate loaded from:" << certPath;
            } else {
                qCWarning(chatServer) << "Failed to load SSL certificate or private key";
            }
        } else {
            qCWarning(chatServer) << "Failed to open SSL certificate or key files";
        }
    } else {
        qCWarning(chatServer) << "SSL certificate paths not configured, using default configuration";
    }
}

void ChatServer::setupCleanupTimer()
{
    _cleanupTimer->setInterval(CLEANUP_INTERVAL);
    _cleanupTimer->setSingleShot(false);
    connect(_cleanupTimer, &QTimer::timeout, this, &ChatServer::cleanupConnections);
}

void ChatServer::processClientMessage(ClientConnection *client, const QByteArray &data)
{
    Q_UNUSED(data) // 数据通过 client->readBuffer 处理
    
    if (!client || !_protocolParser) {
        return;
    }
    
    // 解析数据包
    while (client->readBuffer.size() >= 4) {
        // 协议格式：4字节长度 + JSON数据
        QByteArray lengthBytes = client->readBuffer.left(4);
        qint32 packetLength = qFromBigEndian<qint32>(lengthBytes.data());
        
        if (packetLength <= 0 || packetLength > 1024 * 1024) { // 最大1MB
            qCWarning(chatServer) << "Invalid packet length from client:" << packetLength;
            client->readBuffer.clear();
            return;
        }
        
        if (client->readBuffer.size() < 4 + packetLength) {
            // 数据还不完整，等待更多数据
            break;
        }
        
        // 提取完整的数据包
        QByteArray packetData = client->readBuffer.mid(4, packetLength);
        client->readBuffer.remove(0, 4 + packetLength);
        
        // 解析JSON数据包
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(packetData, &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            qCWarning(chatServer) << "JSON parse error from client:" << parseError.errorString();
            continue;
        }
        
        if (!doc.isObject()) {
            qCWarning(chatServer) << "Invalid packet format from client";
            continue;
        }
        
        QVariantMap packetMap = doc.toVariant().toMap();
        QString type = packetMap["type"].toString();
        QVariantMap data = packetMap["data"].toMap();
        
        qCDebug(chatServer) << "Packet received from client, type:" << type;
        
        // 处理不同类型的请求
        if (type == "auth") {
            handleLoginRequest(client, data);
        } else if (type == "message") {
            handleMessageRequest(client, data);
        } else if (type == "heartbeat") {
            handleHeartbeat(client);
        } else {
            qCWarning(chatServer) << "Unknown packet type from client:" << type;
        }
    }
}

void ChatServer::handleLoginRequest(ClientConnection *client, const QVariantMap &data)
{
    if (!client || !_database) {
        return;
    }
    
    QString authType = data["type"].toString();
    QVariantMap response;
    response["type"] = "auth";
    
    if (authType == "login") {
        QString usernameOrEmail = data["usernameOrEmail"].toString();
        QString password = data["password"].toString();
        
        // 验证用户凭据
        Database::UserInfo userInfo = _database->authenticateUser(usernameOrEmail, password);
        
        if (userInfo.id > 0) {
            // 登录成功
            QString sessionToken = _database->createUserSession(userInfo.id, 
                                                               client->socket->peerAddress().toString(),
                                                               client->socket->peerAddress().toString());
            
            if (!sessionToken.isEmpty()) {
                client->userId = userInfo.id;
                client->sessionToken = sessionToken;
                
                // 添加到用户连接映射
                QMutexLocker locker(&_clientsMutex);
                _userConnections[userInfo.id] = client;
                locker.unlock();
                
                // 更新用户最后在线时间
                _database->updateUserLastOnline(userInfo.id);
                
                QVariantMap responseData;
                responseData["type"] = "login";
                responseData["success"] = true;
                responseData["message"] = "登录成功";
                responseData["token"] = sessionToken;
                responseData["userInfo"] = QVariantMap{
                    {"id", userInfo.id},
                    {"username", userInfo.username},
                    {"email", userInfo.email},
                    {"displayName", userInfo.displayName},
                    {"avatarUrl", userInfo.avatarUrl}
                };
                response["data"] = responseData;
                
                emit userOnline(userInfo.id);
                qCInfo(chatServer) << "User logged in:" << userInfo.username;
                
                // 记录登录日志
                _database->logEvent(Database::Info, "auth", "User logged in", 
                                  userInfo.id, client->socket->peerAddress().toString());
            } else {
                QVariantMap responseData;
                responseData["type"] = "login";
                responseData["success"] = false;
                responseData["message"] = "会话创建失败";
                response["data"] = responseData;
            }
        } else {
            QVariantMap responseData;
            responseData["type"] = "login";
            responseData["success"] = false;
            responseData["message"] = "用户名或密码错误";
            response["data"] = responseData;
            
            qCWarning(chatServer) << "Login failed for:" << usernameOrEmail;
        }
    } else if (authType == "register") {
        QString username = data["username"].toString();
        QString email = data["email"].toString();
        QString password = data["password"].toString();
        QString avatarUrl = data["avatar"].toString();
        
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
                QVariantMap responseData;
                responseData["type"] = "register";
                responseData["success"] = true;
                responseData["message"] = "注册成功";
                response["data"] = responseData;
                
                qCInfo(chatServer) << "User registered:" << username;
                
                // 记录注册日志
                _database->logEvent(Database::Info, "auth", "User registered", 
                                  -1, client->socket->peerAddress().toString(), "", 
                                  QVariantMap{{"username", username}, {"email", email}});
            } else {
                QVariantMap responseData;
                responseData["type"] = "register";
                responseData["success"] = false;
                responseData["message"] = "注册失败";
                response["data"] = responseData;
            }
        }
    } else if (authType == "logout") {
        handleLogoutRequest(client);
        return;
    }
    
    // 发送响应
    QJsonDocument doc = QJsonDocument::fromVariant(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 添加长度前缀
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
    QByteArray packet = lengthBytes + jsonData;
    
    client->socket->write(packet);
    client->socket->flush();
}

void ChatServer::handleLogoutRequest(ClientConnection *client)
{
    if (!client) {
        return;
    }
    
    if (client->userId > 0) {
        // 删除会话
        if (!client->sessionToken.isEmpty() && _database) {
            _database->deleteSession(client->sessionToken);
        }
        
        // 更新用户最后在线时间
        if (_database) {
            _database->updateUserLastOnline(client->userId);
        }
        
        // 从用户连接映射中移除
        QMutexLocker locker(&_clientsMutex);
        _userConnections.remove(client->userId);
        locker.unlock();
        
        emit userOffline(client->userId);
        qCInfo(chatServer) << "User logged out:" << client->userId;
        
        client->userId = 0;
        client->sessionToken.clear();
    }
    
    // 发送登出响应
    QVariantMap response;
    response["type"] = "auth";
    QVariantMap responseData;
    responseData["type"] = "logout";
    responseData["success"] = true;
    response["data"] = responseData;
    
    QJsonDocument doc = QJsonDocument::fromVariant(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
    QByteArray packet = lengthBytes + jsonData;
    
    client->socket->write(packet);
    client->socket->flush();
}

void ChatServer::handleMessageRequest(ClientConnection *client, const QVariantMap &data)
{
    if (!client || client->userId <= 0 || !_database) {
        return;
    }
    
    QString msgType = data["type"].toString();
    
    if (msgType == "send_message") {
        QString receiverStr = data["receiver"].toString();
        QString content = data["content"].toString();
        QString messageType = data["messageType"].toString();
        
        qint64 receiverId = receiverStr.toLongLong();
        if (receiverId <= 0) {
            qCWarning(chatServer) << "Invalid receiver ID:" << receiverStr;
            return;
        }
        
        // 生成消息ID
        QString messageId = QUuid::createUuid().toString(QUuid::WithoutBraces);
        
        // 保存消息到数据库
        if (_database->saveMessage(messageId, client->userId, receiverId, messageType, content)) {
            // 尝试发送给接收者
            QMutexLocker locker(&_clientsMutex);
            ClientConnection *receiverClient = getClientByUserId(receiverId);
            locker.unlock();
            
            if (receiverClient) {
                // 构造消息数据包
                QVariantMap messagePacket;
                messagePacket["type"] = "message";
                QVariantMap messageData;
                messageData["type"] = "message_received";
                messageData["sender"] = QString::number(client->userId);
                messageData["content"] = content;
                messageData["messageType"] = messageType;
                messageData["timestamp"] = QDateTime::currentMSecsSinceEpoch();
                messagePacket["data"] = messageData;
                
                QJsonDocument doc = QJsonDocument::fromVariant(messagePacket);
                QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
                
                QByteArray lengthBytes(4, 0);
                qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
                QByteArray packet = lengthBytes + jsonData;
                
                receiverClient->socket->write(packet);
                receiverClient->socket->flush();
                
                // 更新消息状态为已发送
                _database->updateMessageStatus(messageId, "delivered");
                
                qCInfo(chatServer) << "Message delivered from" << client->userId << "to" << receiverId;
            } else {
                qCInfo(chatServer) << "Receiver not online, message saved:" << receiverId;
            }
            
            // 发送确认给发送者
            QVariantMap confirmPacket;
            confirmPacket["type"] = "message";
            QVariantMap confirmData;
            confirmData["type"] = "message_sent";
            confirmData["messageId"] = messageId;
            confirmPacket["data"] = confirmData;
            
            QJsonDocument doc = QJsonDocument::fromVariant(confirmPacket);
            QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
            
            QByteArray lengthBytes(4, 0);
            qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
            QByteArray packet = lengthBytes + jsonData;
            
            client->socket->write(packet);
            client->socket->flush();
            
            // 增加消息计数
            QMutexLocker statsLocker(&_statsMutex);
            _totalMessages++;
            statsLocker.unlock();
            
            emit messageReceived(client->userId, receiverId, content);
        } else {
            qCWarning(chatServer) << "Failed to save message to database";
        }
    }
}

void ChatServer::handleHeartbeat(ClientConnection *client)
{
    if (!client) {
        return;
    }
    
    // 更新活动时间
    client->lastActivity = QDateTime::currentDateTime();
    
    // 发送心跳响应
    QVariantMap response;
    response["type"] = "heartbeat";
    QVariantMap responseData;
    responseData["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    response["data"] = responseData;
    
    QJsonDocument doc = QJsonDocument::fromVariant(response);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), lengthBytes.data());
    QByteArray packet = lengthBytes + jsonData;
    
    client->socket->write(packet);
    client->socket->flush();
    
    qCDebug(chatServer) << "Heartbeat response sent to user:" << client->userId;
}

void ChatServer::removeClient(QSslSocket *socket)
{
    if (!socket) {
        return;
    }
    
    QMutexLocker locker(&_clientsMutex);
    ClientConnection *client = _clients.value(socket, nullptr);
    if (!client) {
        locker.unlock();
        return;
    }
    
    // 从用户连接映射中移除
    if (client->userId > 0) {
        _userConnections.remove(client->userId);
        emit userOffline(client->userId);
        
        // 更新用户最后在线时间
        if (_database) {
            _database->updateUserLastOnline(client->userId);
        }
    }
    
    // 从客户端列表中移除
    _clients.remove(socket);
    delete client;
    locker.unlock();
    
    // 断开并删除socket
    socket->disconnectFromHost();
    socket->deleteLater();
    
    QString clientAddress = QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());
    qCInfo(chatServer) << "Client disconnected:" << clientAddress;
    emit clientDisconnected(reinterpret_cast<qint64>(socket));
    
    // 记录断开连接日志
    if (_database) {
        _database->logEvent(Database::Info, "connection", "Client disconnected", 
                          client ? client->userId : -1, socket->peerAddress().toString(), "", 
                          QVariantMap{{"socket_id", reinterpret_cast<qint64>(socket)}});
    }
}

ChatServer::ClientConnection* ChatServer::getClientBySocket(QSslSocket *socket)
{
    return _clients.value(socket, nullptr);
}

ChatServer::ClientConnection* ChatServer::getClientByUserId(qint64 userId)
{
    return _userConnections.value(userId, nullptr);
}

