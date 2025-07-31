#include "ChatServer.h"
#include "SessionManager.h"
#include "../database/Database.h"
#include "../network/ProtocolParser.h"
#include "../config/ServerConfig.h"
#include <QSslCertificate>
#include <QSslKey>
#include <QSslConfiguration>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QUuid>
#include <QFile>
#include <QIODevice>
#include <QSsl>
#include <functional>
#include <QtConcurrent>
#include <QRandomGenerator>
#include <QDateTime>
#include <QHostAddress>
#include <QChar>
#include <QtAlgorithms>

Q_LOGGING_CATEGORY(chatServer, "qkchat.server.chatserver")

const int ChatServer::HEARTBEAT_TIMEOUT;
const int ChatServer::CLEANUP_INTERVAL;

ChatServer::ChatServer(QObject *parent)
    : QObject(parent)
    , _sslServer(nullptr)
    , _isRunning(false)
    , _totalMessages(0)
{
    // 初始化组件
    _database = new Database(this);
    _sessionManager = new SessionManager(this);
    _protocolParser = new ProtocolParser(this);
    _threadPool = QThreadPool::globalInstance();
    
    // 从配置获取服务器参数
    ServerConfig *config = ServerConfig::instance();
    _host = config->getServerHost();
    _port = config->getServerPort();
    
    // 设置线程池大小
    _threadPool->setMaxThreadCount(config->getThreadPoolSize());
    
    setupSslServer();
    setupCleanupTimer();
    
    qCInfo(chatServer) << "ChatServer initialized";
}

ChatServer::~ChatServer()
{
    stopServer();
}

bool ChatServer::startServer()
{
    if (_isRunning) {
        qCWarning(chatServer) << "Server is already running";
        return true;
    }
    
    // 初始化数据库连接
    if (!_database->initialize()) {
        emit serverError("无法连接到数据库");
        return false;
    }
    
    // 启动SSL服务器
    if (!_sslServer->listen(QHostAddress(_host), _port)) {
        QString error = QString("无法启动服务器: %1").arg(_sslServer->errorString());
        qCCritical(chatServer) << error;
        emit serverError(error);
        return false;
    }
    
    _isRunning = true;
    _startTime = QDateTime::currentDateTime();
    _cleanupTimer->start();
    
    qCInfo(chatServer) << "Server started on" << _host << ":" << _port;
    emit serverStarted();
    
    return true;
}

void ChatServer::stopServer()
{
    if (!_isRunning) {
        return;
    }
    
    _isRunning = false;
    _cleanupTimer->stop();
    
    // 断开所有客户端连接
    QMutexLocker locker(&_clientsMutex);
    for (auto it = _clients.begin(); it != _clients.end(); ++it) {
        ClientConnection *client = it.value();
        client->socket->disconnectFromHost();
        delete client;
    }
    _clients.clear();
    _userConnections.clear();
    locker.unlock();
    
    // 停止SSL服务器
    if (_sslServer && _sslServer->isListening()) {
        _sslServer->close();
    }
    
    qCInfo(chatServer) << "Server stopped";
    emit serverStopped();
}

void ChatServer::restartServer()
{
    qCInfo(chatServer) << "Restarting server...";
    stopServer();
    
    // 短暂延迟后重启
    QTimer::singleShot(1000, this, [this]() {
        startServer();
    });
}

bool ChatServer::isRunning() const
{
    return _isRunning;
}

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
        // 这里可以通过数据库查询用户名，为简化直接返回用户ID
        users << QString::number(userId);
    }
    
    return users;
}

bool ChatServer::sendMessageToUser(qint64 userId, const QByteArray &message)
{
    QMutexLocker locker(&_clientsMutex);
    
    ClientConnection *client = getClientByUserId(userId);
    if (!client) {
        qCWarning(chatServer) << "User not found or offline:" << userId;
        return false;
    }
    
    qint64 bytesWritten = client->socket->write(message);
    if (bytesWritten == -1) {
        qCWarning(chatServer) << "Failed to send message to user:" << userId;
        return false;
    }
    
    client->socket->flush();
    return true;
}

void ChatServer::broadcastMessage(const QByteArray &message)
{
    QMutexLocker locker(&_clientsMutex);
    
    for (auto it = _clients.begin(); it != _clients.end(); ++it) {
        ClientConnection *client = it.value();
        client->socket->write(message);
        client->socket->flush();
    }
    
    qCInfo(chatServer) << "Broadcast message to" << _clients.size() << "clients";
}

void ChatServer::setupSslServer()
{
    _sslServer = new QSslServer(this);
    
    // 配置SSL证书
    ServerConfig *config = ServerConfig::instance();
    if (config->isSslEnabled()) {
        QString certFile = config->getCertificateFile();
        QString keyFile = config->getPrivateKeyFile();
        
        QList<QSslCertificate> certs = QSslCertificate::fromPath(certFile);
        QSslCertificate certificate = certs.isEmpty() ? QSslCertificate() : certs.first();
        
        QSslKey privateKey;
        QFile keyFileObj(keyFile);
        if (keyFileObj.open(QIODevice::ReadOnly)) {
            privateKey = QSslKey(keyFileObj.readAll(), QSsl::Rsa);
            keyFileObj.close();
        }
        
        if (certificate.isNull() || privateKey.isNull()) {
            qCWarning(chatServer) << "Failed to load SSL certificate or key";
        } else {
            QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
            sslConfig.setLocalCertificate(certificate);
            sslConfig.setPrivateKey(privateKey);
            _sslServer->setSslConfiguration(sslConfig);
            qCInfo(chatServer) << "SSL configuration loaded";
        }
    }
    
    // 连接信号
    connect(_sslServer, &QSslServer::newConnection, this, &ChatServer::onNewConnection);
}

void ChatServer::setupCleanupTimer()
{
    _cleanupTimer = new QTimer(this);
    _cleanupTimer->setInterval(CLEANUP_INTERVAL);
    connect(_cleanupTimer, &QTimer::timeout, this, &ChatServer::cleanupConnections);
}

void ChatServer::onNewConnection()
{
    while (_sslServer->hasPendingConnections()) {
        QSslSocket *socket = qobject_cast<QSslSocket*>(_sslServer->nextPendingConnection());
        if (!socket) {
            continue;
        }
        
        // 创建客户端连接对象
        ClientConnection *client = new ClientConnection;
        client->socket = socket;
        client->userId = -1; // 未认证
        client->lastActivity = QDateTime::currentDateTime();
        
        // 设置SSL配置
        socket->setPeerVerifyMode(QSslSocket::VerifyNone);
        socket->startServerEncryption();
        
        // 连接信号
        connect(socket, &QSslSocket::disconnected, this, &ChatServer::onClientDisconnected);
        connect(socket, &QSslSocket::readyRead, this, &ChatServer::onClientDataReceived);
        connect(socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors),
                this, &ChatServer::onSslErrors);
        
        // 添加到连接列表
        QMutexLocker locker(&_clientsMutex);
        _clients[socket] = client;
        locker.unlock();
        
        qCInfo(chatServer) << "New client connected from" << socket->peerAddress().toString();
        emit clientConnected(reinterpret_cast<qint64>(socket));
    }
}

void ChatServer::onClientDisconnected()
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) {
        return;
    }
    
    qCInfo(chatServer) << "Client disconnected from" << socket->peerAddress().toString();
    
    removeClient(socket);
    emit clientDisconnected(reinterpret_cast<qint64>(socket));
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
        socket->disconnectFromHost();
        return;
    }
    
    // 读取数据
    QByteArray data = socket->readAll();
    client->readBuffer.append(data);
    client->lastActivity = QDateTime::currentDateTime();
    
    locker.unlock();
    
    // 处理消息（在工作线程中）
    Q_UNUSED(QtConcurrent::run(_threadPool, [this, client]() {
        processClientMessage(client, client->readBuffer);
        client->readBuffer.clear();
    }));
}

void ChatServer::onSslErrors(const QList<QSslError> &errors)
{
    QSslSocket *socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) {
        return;
    }
    
    // 在开发环境中忽略SSL错误
    qCWarning(chatServer) << "SSL errors for" << socket->peerAddress().toString();
    for (const QSslError &error : errors) {
        qCWarning(chatServer) << error.errorString();
    }
    
    socket->ignoreSslErrors();
}

void ChatServer::cleanupConnections()
{
    QMutexLocker locker(&_clientsMutex);
    QDateTime cutoffTime = QDateTime::currentDateTime().addMSecs(-HEARTBEAT_TIMEOUT);
    
    auto it = _clients.begin();
    while (it != _clients.end()) {
        ClientConnection *client = it.value();
        
        if (client->lastActivity < cutoffTime) {
            qCInfo(chatServer) << "Removing inactive client:" << client->socket->peerAddress().toString();
            
            // 从用户连接映射中移除
            if (client->userId > 0) {
                _userConnections.remove(client->userId);
                emit userOffline(client->userId);
            }
            
            // 断开连接并删除对象
            client->socket->disconnectFromHost();
            delete client;
            
            it = _clients.erase(it);
        } else {
            ++it;
        }
    }
}

void ChatServer::processClientMessage(ClientConnection *client, const QByteArray &data)
{
    try {
        QVariantMap message = _protocolParser->parseMessage(data);
        QString type = message["type"].toString();
        
        if (type == "login") {
            handleLoginRequest(client, message);
        } else if (type == "logout") {
            handleLogoutRequest(client);
        } else if (type == "send_message") {
            handleMessageRequest(client, message);
        } else if (type == "heartbeat") {
            handleHeartbeat(client);
        } else {
            qCWarning(chatServer) << "Unknown message type:" << type;
        }
        
    } catch (const std::exception &e) {
        qCWarning(chatServer) << "Error processing message:" << e.what();
    }
}

void ChatServer::handleLoginRequest(ClientConnection *client, const QVariantMap &data)
{
    QString usernameOrEmail = data["username_or_email"].toString();
    QString password = data["password"].toString();
    
    // 验证用户身份
    Database::UserInfo userInfo = _database->authenticateUser(usernameOrEmail, password);
    
    QVariantMap response;
    response["type"] = "login_response";
    
    if (userInfo.id > 0) {
        // 创建会话
        QString sessionToken = _sessionManager->createSession(userInfo.id, 
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
            
            response["success"] = true;
            response["message"] = "登录成功";
            response["token"] = sessionToken;
            response["user_info"] = QVariantMap{
                {"id", userInfo.id},
                {"username", userInfo.username},
                {"email", userInfo.email},
                {"display_name", userInfo.displayName},
                {"avatar_url", userInfo.avatarUrl}
            };
            
            emit userOnline(userInfo.id);
            qCInfo(chatServer) << "User logged in:" << userInfo.username;
        } else {
            response["success"] = false;
            response["message"] = "会话创建失败";
        }
    } else {
        response["success"] = false;
        response["message"] = "用户名或密码错误";
    }
    
    // 发送响应
    QByteArray responseData = _protocolParser->createMessage(response);
    client->socket->write(responseData);
    client->socket->flush();
}

void ChatServer::handleLogoutRequest(ClientConnection *client)
{
    if (client->userId > 0) {
        // 删除会话
        _sessionManager->removeSession(client->sessionToken);
        
        // 从用户连接映射中移除
        QMutexLocker locker(&_clientsMutex);
        _userConnections.remove(client->userId);
        locker.unlock();
        
        emit userOffline(client->userId);
        qCInfo(chatServer) << "User logged out:" << client->userId;
        
        client->userId = -1;
        client->sessionToken.clear();
    }
    
    // 发送响应
    QVariantMap response;
    response["type"] = "logout_response";
    response["success"] = true;
    
    QByteArray responseData = _protocolParser->createMessage(response);
    client->socket->write(responseData);
    client->socket->flush();
}

void ChatServer::handleMessageRequest(ClientConnection *client, const QVariantMap &data)
{
    if (client->userId <= 0) {
        // 未认证用户不能发送消息
        return;
    }
    
    QString messageId = data["message_id"].toString();
    QString receiver = data["receiver"].toString();
    QString content = data["content"].toString();
    QString messageType = data["message_type"].toString();
    qint64 timestamp = data["timestamp"].toLongLong();
    
    // 查找接收者
    Database::UserInfo receiverInfo = _database->getUserByUsername(receiver);
    if (receiverInfo.id <= 0) {
        // 接收者不存在
        QVariantMap response;
        response["type"] = "message_error";
        response["message_id"] = messageId;
        response["error"] = "接收者不存在";
        
        QByteArray responseData = _protocolParser->createMessage(response);
        client->socket->write(responseData);
        return;
    }
    
    // 保存消息到数据库
    bool saved = _database->saveMessage(messageId, client->userId, receiverInfo.id, 
                                       messageType, content);
    
    if (saved) {
        // 尝试转发给在线用户
        QMutexLocker locker(&_clientsMutex);
        ClientConnection *receiverClient = getClientByUserId(receiverInfo.id);
        
        if (receiverClient) {
            // 用户在线，直接转发
            QVariantMap forwardMessage;
            forwardMessage["type"] = "message_received";
            forwardMessage["message_id"] = messageId;
            forwardMessage["sender"] = data["sender"];
            forwardMessage["content"] = content;
            forwardMessage["message_type"] = messageType;
            forwardMessage["timestamp"] = timestamp;
            
            QByteArray forwardData = _protocolParser->createMessage(forwardMessage);
            receiverClient->socket->write(forwardData);
            receiverClient->socket->flush();
            
            // 更新消息状态为已投递
            _database->updateMessageStatus(messageId, "delivered");
        }
        locker.unlock();
        
        // 发送确认响应
        QVariantMap response;
        response["type"] = "message_sent";
        response["message_id"] = messageId;
        
        QByteArray responseData = _protocolParser->createMessage(response);
        client->socket->write(responseData);
        client->socket->flush();
        
        {
            QMutexLocker locker(&_statsMutex);
            _totalMessages++;
        }
        emit messageReceived(client->userId, receiverInfo.id, content);
    }
}

void ChatServer::handleHeartbeat(ClientConnection *client)
{
    client->lastActivity = QDateTime::currentDateTime();
    
    // 发送心跳响应
    QVariantMap response;
    response["type"] = "heartbeat_response";
    response["timestamp"] = QDateTime::currentSecsSinceEpoch();
    
    QByteArray responseData = _protocolParser->createMessage(response);
    client->socket->write(responseData);
    client->socket->flush();
}

void ChatServer::removeClient(QSslSocket *socket)
{
    QMutexLocker locker(&_clientsMutex);
    
    ClientConnection *client = _clients.value(socket);
    if (client) {
        // 从用户连接映射中移除
        if (client->userId > 0) {
            _userConnections.remove(client->userId);
            emit userOffline(client->userId);
        }
        
        // 删除会话
        if (!client->sessionToken.isEmpty()) {
            _sessionManager->removeSession(client->sessionToken);
        }
        
        delete client;
        _clients.remove(socket);
    }
    
    socket->deleteLater();
}

ChatServer::ClientConnection *ChatServer::getClientBySocket(QSslSocket *socket)
{
    return _clients.value(socket);
}

QString ChatServer::getUptime() const
{
    if (_startTime.isNull()) {
        return "00:00:00";
    }
    
    qint64 seconds = _startTime.secsTo(QDateTime::currentDateTime());
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    return QString("%1:%2:%3")
           .arg(hours, 2, 10, QChar('0'))
           .arg(minutes, 2, 10, QChar('0'))
           .arg(secs, 2, 10, QChar('0'));
}

int ChatServer::getCpuUsage() const
{
    // 简单的CPU使用率模拟，实际项目中应该使用系统API
    return QRandomGenerator::global()->bounded(20) + 10; // 10-30%
}

int ChatServer::getMemoryUsage() const
{
    // 简单的内存使用率模拟，实际项目中应该使用系统API
    return QRandomGenerator::global()->bounded(30) + 20; // 20-50%
}

ChatServer::ClientConnection *ChatServer::getClientByUserId(qint64 userId)
{
    return _userConnections.value(userId);
}

int ChatServer::getTotalUserCount() const
{
    if (_database) {
        return _database->getTotalUserCount();
    }
    return 0;
}

