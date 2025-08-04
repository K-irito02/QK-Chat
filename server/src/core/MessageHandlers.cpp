#include "MessageHandlers.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QUuid>
#include <QSslSocket>
#include <QtConcurrent/QtConcurrent>
#include "../core/ConnectionManager.h"
#include <memory>

Q_LOGGING_CATEGORY(messageHandlers, "qkchat.server.messagehandlers")

// ============================================================================
// LoginMessageHandler Implementation
// ============================================================================

LoginMessageHandler::LoginMessageHandler(ConnectionManager* connectionManager,
                                        SessionManager* sessionManager,
                                        DatabasePool* databasePool,
                                        CacheManagerV2* cacheManager)
    : m_connectionManager(connectionManager)
    , m_sessionManager(sessionManager)
    , m_databasePool(databasePool)
    , m_cacheManager(cacheManager)
{
}

bool LoginMessageHandler::canHandle(MessageType type) const
{
    return type == MessageType::Login;
}

bool LoginMessageHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling login message from" << message.fromUserId;

    QJsonObject data = message.data;
    QString username = data["username"].toString();
    QString password = data["password"].toString();

    if (username.isEmpty() || password.isEmpty()) {
        sendLoginResponse(message.sourceSocket, false, "Username and password required");
        return false;
    }

    // 获取连接的共享指针，确保在异步操作中socket不会被意外释放
    std::shared_ptr<ClientConnection> connection = m_connectionManager->getConnection(message.sourceSocket);
    if (!connection) {
        qCWarning(messageHandlers) << "Could not find connection for socket";
        return false;
    }

    // 使用异步方式处理登录
    QtConcurrent::run([this, connection, username, password]() {
        qint64 userId = 0;
        bool authSuccess = authenticateUser(username, password, userId);

        // 使用 QPointer 弱指针来安全地访问 socket
        QPointer<QSslSocket> socketPtr = connection->getSocket();

        // 回到Socket所在线程执行后续操作
        QMetaObject::invokeMethod(socketPtr->thread(), [this, socketPtr, userId, authSuccess, connection]() {
            if (!socketPtr) { // 检查 socket 是否仍然有效
                qCWarning(messageHandlers) << "Socket is no longer valid, aborting login completion.";
                return;
            }

            if (!authSuccess) {
                sendLoginResponse(socketPtr.data(), false, "Invalid credentials");
                return;
            }

            QString sessionToken;
            if (!createUserSession(userId, socketPtr.data(), sessionToken)) {
                sendLoginResponse(socketPtr.data(), false, "Failed to create session");
                return;
            }

            sendLoginResponse(socketPtr.data(), true, "Login successful", userId, sessionToken);
            qCInfo(messageHandlers) << "User" << userId << "logged in successfully";
        });
    });

    return true;
}

QString LoginMessageHandler::handlerName() const
{
    return "LoginMessageHandler";
}

bool LoginMessageHandler::authenticateUser(const QString& username, const QString& password, qint64& userId)
{
    if (!m_databasePool) {
        return false;
    }
    
    // 首先检查缓存
    if (m_cacheManager) {
        QString cacheKey = QString("user_auth:%1").arg(username);
        QVariant cachedData = m_cacheManager->get(cacheKey);
        if (cachedData.isValid()) {
            QVariantMap userData = cachedData.toMap();
            QString hashedPassword = userData["password"].toString();
            
            // 验证密码
            QByteArray passwordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
            if (hashedPassword == passwordHash.toHex()) {
                userId = userData["user_id"].toLongLong();
                return true;
            }
        }
    }
    
    // 从数据库查询
    QString sql = "SELECT user_id, password_hash FROM users WHERE username = ? AND active = 1";
    QVariantList params = {username};
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Read);
    if (!result.success) {
        qCWarning(messageHandlers) << "Database query failed:" << result.error;
        return false;
    }
    
    if (!result.data.next()) {
        return false; // 用户不存在
    }
    
    userId = result.data.value("user_id").toLongLong();
    QString storedHash = result.data.value("password_hash").toString();
    
    // 验证密码
    QByteArray passwordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    bool authenticated = (storedHash == passwordHash.toHex());
    
    // 缓存认证信息
    if (authenticated && m_cacheManager) {
        QString cacheKey = QString("user_auth:%1").arg(username);
        QVariantMap userData;
        userData["user_id"] = userId;
        userData["password"] = storedHash;
        m_cacheManager->set(cacheKey, userData, 300); // 缓存5分钟
    }
    
    return authenticated;
}

bool LoginMessageHandler::createUserSession(qint64 userId, QSslSocket* socket, QString& sessionToken)
{
    if (!m_sessionManager || !m_connectionManager) {
        return false;
    }
    
    sessionToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // 创建会话
    sessionToken = m_sessionManager->createSession(userId, socket->peerAddress().toString());
    if (sessionToken.isEmpty()) {
        return false;
    }
    
    // 注册连接
    if (!m_connectionManager->addConnection(socket)) {
        m_sessionManager->removeSession(sessionToken);
        return false;
    }
    
    // 认证连接
    if (!m_connectionManager->authenticateConnection(socket, userId, sessionToken)) {
        m_connectionManager->removeConnection(socket);
        m_sessionManager->removeSession(sessionToken);
        return false;
    }
    
    // 缓存会话信息
    if (m_cacheManager) {
        m_cacheManager->cacheUserSession(sessionToken, userId, 7200); // 2小时
    }
    
    return true;
}

void LoginMessageHandler::sendLoginResponse(QSslSocket* socket, bool success, const QString& message, 
                                          qint64 userId, const QString& sessionToken)
{
    QJsonObject response;
    response["type"] = "login_response";
    response["success"] = success;
    response["message"] = message;
    
    if (success) {
        response["user_id"] = userId;
        response["session_token"] = sessionToken;
    }
    
    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(data);
        socket->flush();
    }
}

// ============================================================================
// ChatMessageHandler Implementation
// ============================================================================

ChatMessageHandler::ChatMessageHandler(ConnectionManager* connectionManager,
                                      DatabasePool* databasePool,
                                      CacheManagerV2* cacheManager)
    : m_connectionManager(connectionManager)
    , m_databasePool(databasePool)
    , m_cacheManager(cacheManager)
{
}

bool ChatMessageHandler::canHandle(MessageType type) const
{
    return type == MessageType::Chat;
}

bool ChatMessageHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling chat message from" << message.fromUserId 
                            << "to" << message.toUserId;
    
    if (!validateChatMessage(message)) {
        return false;
    }
    
    if (!saveMessageToDatabase(message)) {
        qCWarning(messageHandlers) << "Failed to save message to database";
        return false;
    }
    
    if (!deliverMessage(message)) {
        qCWarning(messageHandlers) << "Failed to deliver message";
        return false;
    }
    
            sendDeliveryConfirmation(message.sourceSocket, message.id, true);
    
    qCDebug(messageHandlers) << "Chat message" << message.id << "processed successfully";
    return true;
}

QString ChatMessageHandler::handlerName() const
{
    return "ChatMessageHandler";
}

bool ChatMessageHandler::validateChatMessage(const Message& message)
{
    if (message.fromUserId <= 0 || message.toUserId <= 0) {
        qCWarning(messageHandlers) << "Invalid user IDs in message";
        return false;
    }
    
    if (message.data["content"].toString().isEmpty()) {
        qCWarning(messageHandlers) << "Empty message content";
        return false;
    }
    
    if (message.data["content"].toString().length() > 4096) {
        qCWarning(messageHandlers) << "Message content too long";
        return false;
    }
    
    return true;
}

bool ChatMessageHandler::saveMessageToDatabase(const Message& message)
{
    if (!m_databasePool) {
        return false;
    }
    
    QString sql = "INSERT INTO messages (message_id, from_user_id, to_user_id, content, message_type, created_at) "
                  "VALUES (?, ?, ?, ?, ?, NOW())";
    
    QVariantList params = {
        message.id,
        message.fromUserId,
        message.toUserId,
        message.data["content"].toString(),
        static_cast<int>(message.type)
    };
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Write);
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to save message:" << result.error;
        return false;
    }
    
    // 更新缓存中的最近消息
    if (m_cacheManager) {
        qint64 chatId = qMin(message.fromUserId, message.toUserId) * 1000000 + 
                       qMax(message.fromUserId, message.toUserId);
        
        QVariantList recentMessages = m_cacheManager->getRecentMessages(chatId);
        
        QVariantMap messageData;
        messageData["id"] = message.id;
        messageData["from_user_id"] = message.fromUserId;
        messageData["to_user_id"] = message.toUserId;
        messageData["content"] = message.data["content"];
        messageData["timestamp"] = QDateTime::currentDateTime();
        
        recentMessages.append(messageData);
        
        // 只保留最近50条消息
        if (recentMessages.size() > 50) {
            recentMessages = recentMessages.mid(recentMessages.size() - 50);
        }
        
        m_cacheManager->cacheRecentMessages(chatId, recentMessages, 300);
    }
    
    return true;
}

bool ChatMessageHandler::deliverMessage(const Message& message)
{
    if (!m_connectionManager) {
        return false;
    }
    
    auto targetConnection = m_connectionManager->getConnectionByUserId(message.toUserId);
    if (!targetConnection) {
        qCDebug(messageHandlers) << "Target user" << message.toUserId << "is not online";
        return false; // 用户不在线，消息已保存到数据库
    }
    
    QSslSocket* targetSocket = targetConnection->getSocket();
    
    QJsonObject deliveryMessage;
    deliveryMessage["type"] = "message";
    deliveryMessage["id"] = message.id;
    deliveryMessage["from_user_id"] = message.fromUserId;
    deliveryMessage["content"] = message.data["content"];
    deliveryMessage["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(deliveryMessage);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    if (targetSocket->state() == QAbstractSocket::ConnectedState) {
        targetSocket->write(data);
        targetSocket->flush();
        return true;
    }
    
    return false;
}

void ChatMessageHandler::sendDeliveryConfirmation(QSslSocket* socket, const QString& messageId, bool delivered)
{
    QJsonObject confirmation;
    confirmation["type"] = "delivery_confirmation";
    confirmation["message_id"] = messageId;
    confirmation["delivered"] = delivered;
    confirmation["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(confirmation);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(data);
        socket->flush();
    }
}

// ============================================================================
// HeartbeatMessageHandler Implementation
// ============================================================================

HeartbeatMessageHandler::HeartbeatMessageHandler(ConnectionManager* connectionManager)
    : m_connectionManager(connectionManager)
{
}

bool HeartbeatMessageHandler::canHandle(MessageType type) const
{
    return type == MessageType::Heartbeat;
}

bool HeartbeatMessageHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling heartbeat from user" << message.fromUserId;
    
    // 更新连接的最后活跃时间
    if (m_connectionManager && message.sourceSocket) {
        m_connectionManager->updateConnectionActivity(message.sourceSocket);
    }
    
    sendHeartbeatResponse(message.sourceSocket);
    return true;
}

QString HeartbeatMessageHandler::handlerName() const
{
    return "HeartbeatMessageHandler";
}

void HeartbeatMessageHandler::sendHeartbeatResponse(QSslSocket* socket)
{
    QJsonObject response;
    response["type"] = "heartbeat_response";
    response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    response["server_time"] = QDateTime::currentMSecsSinceEpoch();
    
    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(data);
        socket->flush();
    }
}

// ============================================================================
// RegisterMessageHandler Implementation
// ============================================================================

RegisterMessageHandler::RegisterMessageHandler(ConnectionManager* connectionManager,
                                              DatabasePool* databasePool,
                                              CacheManagerV2* cacheManager)
    : m_connectionManager(connectionManager)
    , m_databasePool(databasePool)
    , m_cacheManager(cacheManager)
{
}

bool RegisterMessageHandler::canHandle(MessageType type) const
{
    return type == MessageType::Register;
}

bool RegisterMessageHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling registration message";
    
    QJsonObject data = message.data;
    
    if (!validateRegistrationData(data)) {
        sendRegistrationResponse(message.sourceSocket, false, "Invalid registration data");
        return false;
    }
    
    QString username = data["username"].toString();
    QString email = data["email"].toString();
    
    if (checkUserExists(username, email)) {
        sendRegistrationResponse(message.sourceSocket, false, "Username or email already exists");
        return false;
    }
    
    qint64 userId = 0;
    if (!createUser(data, userId)) {
        sendRegistrationResponse(message.sourceSocket, false, "Failed to create user account");
        return false;
    }
    
    sendRegistrationResponse(message.sourceSocket, true, "Registration successful", userId);
    
    qCInfo(messageHandlers) << "User registered successfully with ID" << userId;
    return true;
}

QString RegisterMessageHandler::handlerName() const
{
    return "RegisterMessageHandler";
}

bool RegisterMessageHandler::validateRegistrationData(const QJsonObject& data)
{
    QString username = data["username"].toString();
    QString email = data["email"].toString();
    QString password = data["password"].toString();
    
    if (username.isEmpty() || username.length() < 3 || username.length() > 20) {
        return false;
    }
    
    if (email.isEmpty() || !email.contains("@")) {
        return false;
    }
    
    if (password.isEmpty() || password.length() < 6) {
        return false;
    }
    
    return true;
}

bool RegisterMessageHandler::checkUserExists(const QString& username, const QString& email)
{
    if (!m_databasePool) {
        return true; // 保守处理，假设用户存在
    }
    
    QString sql = "SELECT COUNT(*) as count FROM users WHERE username = ? OR email = ?";
    QVariantList params = {username, email};
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Read);
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to check user existence:" << result.error;
        return true; // 保守处理
    }
    
    if (result.data.next()) {
        int count = result.data.value("count").toInt();
        return count > 0;
    }
    
    return false;
}

bool RegisterMessageHandler::createUser(const QJsonObject& data, qint64& userId)
{
    if (!m_databasePool) {
        return false;
    }
    
    QString username = data["username"].toString();
    QString email = data["email"].toString();
    QString password = data["password"].toString();
    QString nickname = data["nickname"].toString();
    
    if (nickname.isEmpty()) {
        nickname = username;
    }
    
    // 哈希密码
    QByteArray passwordHash = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    
    QString sql = "INSERT INTO users (username, email, password_hash, nickname, created_at, active) "
                  "VALUES (?, ?, ?, ?, NOW(), 1)";
    
    QVariantList params = {username, email, passwordHash.toHex(), nickname};
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Write);
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to create user:" << result.error;
        return false;
    }
    
    // 获取新创建的用户ID
    sql = "SELECT LAST_INSERT_ID() as user_id";
    result = m_databasePool->executeQuery(sql, QVariantList(), DatabaseOperationType::Read);
    if (result.success && result.data.next()) {
        userId = result.data.value("user_id").toLongLong();
        return true;
    }
    
    return false;
}

void RegisterMessageHandler::sendRegistrationResponse(QSslSocket* socket, bool success, 
                                                     const QString& message, qint64 userId)
{
    QJsonObject response;
    response["type"] = "registration_response";
    response["success"] = success;
    response["message"] = message;
    
    if (success && userId > 0) {
        response["user_id"] = userId;
    }
    
    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(data);
        socket->flush();
    }
}
