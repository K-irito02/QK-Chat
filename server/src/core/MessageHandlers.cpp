#include "MessageHandlers.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QUuid>
#include <QSslSocket>
#include <QtConcurrent/QtConcurrent>
#include "../core/ConnectionManager.h"
#include "../services/EmailVerificationService.h"
#include <memory>

// ============================================================================
// EmailVerificationMessageHandler Implementation
// ============================================================================

EmailVerificationMessageHandler::EmailVerificationMessageHandler(EmailVerificationService* emailService)
    : m_emailService(emailService)
{
}

bool EmailVerificationMessageHandler::canHandle(MessageType type) const
{
    return type == MessageType::EmailVerification;
}

bool EmailVerificationMessageHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling email verification message from" << message.fromUserId;

    QJsonObject data = message.data;
    QString email = data["email"].toString();
    QString action = data["action"].toString();

    if (email.isEmpty()) {
        sendVerificationResponse(message.sourceSocket, false, "Email is required");
        return false;
    }

    if (!m_emailService) {
        sendVerificationResponse(message.sourceSocket, false, "Email service is not available");
        return false;
    }

    QSslSocket* socket = message.sourceSocket;
    
    if (action == "sendCode" || action.isEmpty()) {
        // 异步发送验证码
        QtConcurrent::run([this, email, socket]() {
            bool success = m_emailService->sendVerificationCode(email);
            // 发送响应
            if (success) {
                sendCodeSentResponse(socket, true, "Verification code sent successfully");
            } else {
                sendVerificationResponse(socket, false, "Failed to send verification code");
            }
        });
    } else if (action == "verifyCode") {
        // 验证验证码
        QString code = data["code"].toString();
        if (code.isEmpty()) {
            sendVerificationResponse(socket, false, "Verification code is required");
            return false;
        }
        
        QtConcurrent::run([this, email, code, socket]() {
            bool success = m_emailService->verifyCode(email, code);
            // 发送响应
            if (success) {
                sendVerificationResponse(socket, true, "Email verified successfully");
            } else {
                sendVerificationResponse(socket, false, "Invalid verification code");
            }
        });
    } else {
        sendVerificationResponse(socket, false, "Invalid action. Use 'sendCode' or 'verifyCode'");
        return false;
    }

    return true;
}

QString EmailVerificationMessageHandler::handlerName() const
{
    return "EmailVerificationMessageHandler";
}

void EmailVerificationMessageHandler::sendVerificationResponse(QSslSocket* socket, bool success, const QString& message)
{
    if (!socket || !socket->isOpen()) {
        return;
    }

    QJsonObject response;
    response["type"] = "emailVerification";
    response["success"] = success;
    response["message"] = message;

    QJsonDocument doc(response);
    socket->write(doc.toJson(QJsonDocument::Compact));
}

void EmailVerificationMessageHandler::sendCodeSentResponse(QSslSocket* socket, bool success, const QString& message)
{
    if (!socket || !socket->isOpen()) {
        return;
    }

    QJsonObject response;
    response["type"] = "emailCodeSent";
    response["success"] = success;
    response["message"] = message;

    QJsonDocument doc(response);
    socket->write(doc.toJson(QJsonDocument::Compact));
}


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
                                             CacheManagerV2* cacheManager,
                                             EmailVerificationService* emailService)
    : m_connectionManager(connectionManager)
    , m_databasePool(databasePool)
    , m_cacheManager(cacheManager)
    , m_emailService(emailService)
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
    QString verificationCode = data["verificationCode"].toString();
    
    if (checkUserExists(username)) {
        sendRegistrationResponse(message.sourceSocket, false, "Username already exists");
        return false;
    }
    
    if (checkEmailExists(email)) {
        sendRegistrationResponse(message.sourceSocket, false, "Email already exists");
        return false;
    }
    
    // 验证邮箱验证码
    if (m_emailService && !m_emailService->verifyCode(email, verificationCode)) {
        sendRegistrationResponse(message.sourceSocket, false, "Invalid verification code");
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
    QString verificationCode = data["verificationCode"].toString();
    
    if (username.isEmpty() || username.length() < 3 || username.length() > 20) {
        return false;
    }
    
    if (email.isEmpty() || !email.contains("@")) {
        return false;
    }
    
    if (password.isEmpty() || password.length() < 6) {
        return false;
    }
    
    if (verificationCode.isEmpty() || verificationCode.length() != 6) {
        return false;
    }
    
    return true;
}

bool RegisterMessageHandler::checkUserExists(const QString& username)
{
    if (!m_databasePool) {
        return true; // 保守处理，假设用户存在
    }
    
    QString sql = "SELECT COUNT(*) as count FROM users WHERE username = ?";
    QVariantList params = {username};
    
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

bool RegisterMessageHandler::checkEmailExists(const QString& email)
{
    if (!m_databasePool) {
        return true; // 保守处理，假设邮箱存在
    }
    
    QString sql = "SELECT COUNT(*) as count FROM users WHERE email = ?";
    QVariantList params = {email};
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Read);
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to check email existence:" << result.error;
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
    
    QString sql = "INSERT INTO users (username, email, password_hash, display_name, status, created_at, updated_at) "
                  "VALUES (?, ?, ?, ?, 'active', NOW(), NOW())";
    
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



// ============================================================================
// ValidationMessageHandler Implementation
// ============================================================================

ValidationMessageHandler::ValidationMessageHandler(DatabasePool* databasePool, EmailVerificationService* emailService)
    : m_databasePool(databasePool)
    , m_emailService(emailService)
{
}

bool ValidationMessageHandler::canHandle(MessageType type) const
{
    return type == MessageType::UsernameValidation || type == MessageType::EmailAvailability;
}

bool ValidationMessageHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling validation message";
    
    if (message.type == MessageType::UsernameValidation) {
        handleUsernameValidation(message);
    } else if (message.type == MessageType::EmailAvailability) {
        handleEmailValidation(message);
    }
    
    return true;
}

QString ValidationMessageHandler::handlerName() const
{
    return "ValidationMessageHandler";
}

void ValidationMessageHandler::handleUsernameValidation(const Message& message)
{
    QJsonObject data = message.data;
    QString username = data["username"].toString();
    
    if (username.isEmpty()) {
        sendValidationResponse(message.sourceSocket, "username", false, "Username is required");
        return;
    }
    
    // 检查用户名是否可用
    QString sql = "SELECT COUNT(*) as count FROM users WHERE username = ?";
    QVariantList params = {username};
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Read);
    bool available = true;
    QString errorMessage = "";
    
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to check username availability:" << result.error;
        available = false;
        errorMessage = "Database error";
    } else if (result.data.next()) {
        int count = result.data.value("count").toInt();
        available = count == 0;
        errorMessage = available ? "" : "Username already exists";
    }
    
    sendValidationResponse(message.sourceSocket, "username", available, errorMessage);
}

void ValidationMessageHandler::handleEmailValidation(const Message& message)
{
    QJsonObject data = message.data;
    QString email = data["email"].toString();
    
    if (email.isEmpty()) {
        sendValidationResponse(message.sourceSocket, "email", false, "Email is required");
        return;
    }
    
    if (!email.contains("@")) {
        sendValidationResponse(message.sourceSocket, "email", false, "Invalid email format");
        return;
    }
    
    // 检查邮箱是否可用
    QString sql = "SELECT COUNT(*) as count FROM users WHERE email = ?";
    QVariantList params = {email};
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Read);
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to check email availability:" << result.error;
        sendValidationResponse(message.sourceSocket, "email", false, "Database error");
        return;
    }
    
    bool available = true;
    QString errorMessage = "";
    
    if (result.data.next()) {
        int count = result.data.value("count").toInt();
        available = count == 0;
        errorMessage = available ? "" : "Email already exists";
    }
    
    sendValidationResponse(message.sourceSocket, "email", available, errorMessage);
}

void ValidationMessageHandler::sendValidationResponse(QSslSocket* socket, const QString& type, bool available, const QString& message)
{
    QJsonObject response;
    response["type"] = "validation";
    response["validationType"] = type;
    response["available"] = available;
    response["message"] = message;
    
    QJsonDocument doc(response);
    socket->write(doc.toJson());
}

// ============================================================================
// UserStatusMessageHandler Implementation
// ============================================================================

UserStatusMessageHandler::UserStatusMessageHandler(ConnectionManager* connectionManager,
                                                 DatabasePool* databasePool,
                                                 CacheManagerV2* cacheManager)
    : m_connectionManager(connectionManager)
    , m_databasePool(databasePool)
    , m_cacheManager(cacheManager)
{
}

bool UserStatusMessageHandler::canHandle(MessageType type) const
{
    return type == MessageType::UserStatus;
}

bool UserStatusMessageHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling user status message from" << message.fromUserId;
    
    QJsonObject data = message.data;
    QString status = data["status"].toString();
    
    if (status.isEmpty()) {
        qCWarning(messageHandlers) << "Empty status in user status message";
        return false;
    }
    
    if (!updateUserStatus(message.fromUserId, status)) {
        qCWarning(messageHandlers) << "Failed to update user status";
        return false;
    }
    
    broadcastStatusChange(message.fromUserId, status);
    
    qCDebug(messageHandlers) << "User status updated successfully for user" << message.fromUserId;
    return true;
}

QString UserStatusMessageHandler::handlerName() const
{
    return "UserStatusMessageHandler";
}

bool UserStatusMessageHandler::updateUserStatus(qint64 userId, const QString& status)
{
    if (!m_databasePool) {
        return false;
    }
    
    QString sql = "UPDATE users SET status = ?, last_online = NOW() WHERE user_id = ?";
    QVariantList params = {status, userId};
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Write);
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to update user status:" << result.error;
        return false;
    }
    
    // 更新缓存
    if (m_cacheManager) {
        QString cacheKey = QString("user_status:%1").arg(userId);
        m_cacheManager->set(cacheKey, status, 300); // 缓存5分钟
    }
    
    return true;
}

void UserStatusMessageHandler::broadcastStatusChange(qint64 userId, const QString& status)
{
    if (!m_connectionManager) {
        return;
    }
    
    QJsonObject statusMessage;
    statusMessage["type"] = "user_status_change";
    statusMessage["user_id"] = userId;
    statusMessage["status"] = status;
    statusMessage["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(statusMessage);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    // 广播给所有在线用户
    auto connections = m_connectionManager->getAllConnections();
    for (const auto& connection : connections) {
        if (connection && connection->getUserId() != userId) {
            QSslSocket* socket = connection->getSocket();
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(data);
                socket->flush();
            }
        }
    }
}

// ============================================================================
// GroupChatMessageHandler Implementation
// ============================================================================

GroupChatMessageHandler::GroupChatMessageHandler(ConnectionManager* connectionManager,
                                               DatabasePool* databasePool,
                                               CacheManagerV2* cacheManager)
    : m_connectionManager(connectionManager)
    , m_databasePool(databasePool)
    , m_cacheManager(cacheManager)
{
}

bool GroupChatMessageHandler::canHandle(MessageType type) const
{
    return type == MessageType::GroupChat;
}

bool GroupChatMessageHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling group chat message from" << message.fromUserId;
    
    if (!validateGroupMessage(message)) {
        return false;
    }
    
    if (!saveGroupMessage(message)) {
        qCWarning(messageHandlers) << "Failed to save group message";
        return false;
    }
    
    qint64 groupId = message.data["group_id"].toVariant().toLongLong();
    QList<qint64> members = getGroupMembers(groupId);
    
    if (!deliverToGroupMembers(message, members)) {
        qCWarning(messageHandlers) << "Failed to deliver group message";
        return false;
    }
    
    qCDebug(messageHandlers) << "Group message processed successfully";
    return true;
}

QString GroupChatMessageHandler::handlerName() const
{
    return "GroupChatMessageHandler";
}

bool GroupChatMessageHandler::validateGroupMessage(const Message& message)
{
    if (message.fromUserId <= 0) {
        qCWarning(messageHandlers) << "Invalid sender ID in group message";
        return false;
    }
    
    qint64 groupId = message.data["group_id"].toVariant().toLongLong();
    if (groupId <= 0) {
        qCWarning(messageHandlers) << "Invalid group ID in group message";
        return false;
    }
    
    if (message.data["content"].toString().isEmpty()) {
        qCWarning(messageHandlers) << "Empty content in group message";
        return false;
    }
    
    return true;
}

QList<qint64> GroupChatMessageHandler::getGroupMembers(qint64 groupId)
{
    QList<qint64> members;
    
    if (!m_databasePool) {
        return members;
    }
    
    QString sql = "SELECT user_id FROM group_members WHERE group_id = ? AND status = 'active'";
    QVariantList params = {groupId};
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Read);
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to get group members:" << result.error;
        return members;
    }
    
    while (result.data.next()) {
        qint64 userId = result.data.value("user_id").toLongLong();
        members.append(userId);
    }
    
    return members;
}

bool GroupChatMessageHandler::saveGroupMessage(const Message& message)
{
    if (!m_databasePool) {
        return false;
    }
    
    qint64 groupId = message.data["group_id"].toVariant().toLongLong();
    
    QString sql = "INSERT INTO group_messages (message_id, group_id, sender_id, content, message_type, created_at) "
                  "VALUES (?, ?, ?, ?, ?, NOW())";
    
    QVariantList params = {
        message.id,
        groupId,
        message.fromUserId,
        message.data["content"].toString(),
        static_cast<int>(message.type)
    };
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Write);
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to save group message:" << result.error;
        return false;
    }
    
    return true;
}

bool GroupChatMessageHandler::deliverToGroupMembers(const Message& message, const QList<qint64>& members)
{
    if (!m_connectionManager) {
        return false;
    }
    
    QJsonObject deliveryMessage;
    deliveryMessage["type"] = "group_message";
    deliveryMessage["id"] = message.id;
    deliveryMessage["group_id"] = message.data["group_id"];
    deliveryMessage["from_user_id"] = message.fromUserId;
    deliveryMessage["content"] = message.data["content"];
    deliveryMessage["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(deliveryMessage);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    int deliveredCount = 0;
    
    for (qint64 memberId : members) {
        if (memberId == message.fromUserId) {
            continue; // 跳过发送者
        }
        
        auto connection = m_connectionManager->getConnectionByUserId(memberId);
        if (connection) {
            QSslSocket* socket = connection->getSocket();
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(data);
                socket->flush();
                deliveredCount++;
            }
        }
    }
    
    qCDebug(messageHandlers) << "Group message delivered to" << deliveredCount << "members";
    return deliveredCount > 0;
}

// ============================================================================
// SystemNotificationHandler Implementation
// ============================================================================

SystemNotificationHandler::SystemNotificationHandler(ConnectionManager* connectionManager,
                                                   CacheManagerV2* cacheManager)
    : m_connectionManager(connectionManager)
    , m_cacheManager(cacheManager)
{
}

bool SystemNotificationHandler::canHandle(MessageType type) const
{
    return type == MessageType::SystemNotification;
}

bool SystemNotificationHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling system notification message";
    
    if (!validateNotification(message)) {
        return false;
    }
    
    QJsonObject notification = message.data;
    QString notificationType = notification["notification_type"].toString();
    
    if (notificationType == "broadcast") {
        broadcastSystemNotification(notification);
    } else if (notificationType == "targeted") {
        QList<qint64> userIds;
        QJsonArray usersArray = notification["user_ids"].toArray();
        for (const auto& userValue : usersArray) {
            userIds.append(userValue.toVariant().toLongLong());
        }
        sendTargetedNotification(userIds, notification);
    }
    
    return true;
}

QString SystemNotificationHandler::handlerName() const
{
    return "SystemNotificationHandler";
}

bool SystemNotificationHandler::validateNotification(const Message& message)
{
    QJsonObject notification = message.data;
    
    if (notification["notification_type"].toString().isEmpty()) {
        qCWarning(messageHandlers) << "Missing notification type";
        return false;
    }
    
    if (notification["title"].toString().isEmpty()) {
        qCWarning(messageHandlers) << "Missing notification title";
        return false;
    }
    
    if (notification["content"].toString().isEmpty()) {
        qCWarning(messageHandlers) << "Missing notification content";
        return false;
    }
    
    return true;
}

void SystemNotificationHandler::broadcastSystemNotification(const QJsonObject& notification)
{
    if (!m_connectionManager) {
        return;
    }
    
    QJsonObject systemMessage;
    systemMessage["type"] = "system_notification";
    systemMessage["title"] = notification["title"];
    systemMessage["content"] = notification["content"];
    systemMessage["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(systemMessage);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    auto connections = m_connectionManager->getAllConnections();
    for (const auto& connection : connections) {
        if (connection) {
            QSslSocket* socket = connection->getSocket();
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(data);
                socket->flush();
            }
        }
    }
    
    qCInfo(messageHandlers) << "System notification broadcasted to" << connections.size() << "users";
}

void SystemNotificationHandler::sendTargetedNotification(const QList<qint64>& userIds, const QJsonObject& notification)
{
    if (!m_connectionManager) {
        return;
    }
    
    QJsonObject systemMessage;
    systemMessage["type"] = "system_notification";
    systemMessage["title"] = notification["title"];
    systemMessage["content"] = notification["content"];
    systemMessage["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(systemMessage);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    int deliveredCount = 0;
    
    for (qint64 userId : userIds) {
        auto connection = m_connectionManager->getConnectionByUserId(userId);
        if (connection) {
            QSslSocket* socket = connection->getSocket();
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(data);
                socket->flush();
                deliveredCount++;
            }
        }
    }
    
    qCInfo(messageHandlers) << "Targeted notification sent to" << deliveredCount << "users";
}

// ============================================================================
// FileTransferMessageHandler Implementation
// ============================================================================

FileTransferMessageHandler::FileTransferMessageHandler(ConnectionManager* connectionManager,
                                                     DatabasePool* databasePool,
                                                     CacheManagerV2* cacheManager)
    : m_connectionManager(connectionManager)
    , m_databasePool(databasePool)
    , m_cacheManager(cacheManager)
{
}

bool FileTransferMessageHandler::canHandle(MessageType type) const
{
    return type == MessageType::FileTransfer;
}

bool FileTransferMessageHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling file transfer message from" << message.fromUserId;
    
    if (!validateFileTransfer(message)) {
        return false;
    }
    
    if (!saveFileMetadata(message)) {
        qCWarning(messageHandlers) << "Failed to save file metadata";
        return false;
    }
    
    qint64 toUserId = message.toUserId;
    QJsonObject fileInfo;
    fileInfo["file_id"] = message.data["file_id"];
    fileInfo["file_name"] = message.data["file_name"];
    fileInfo["file_size"] = message.data["file_size"];
    fileInfo["file_type"] = message.data["file_type"];
    fileInfo["from_user_id"] = message.fromUserId;
    
    notifyFileTransfer(toUserId, fileInfo);
    
    qCDebug(messageHandlers) << "File transfer message processed successfully";
    return true;
}

QString FileTransferMessageHandler::handlerName() const
{
    return "FileTransferMessageHandler";
}

bool FileTransferMessageHandler::validateFileTransfer(const Message& message)
{
    if (message.fromUserId <= 0 || message.toUserId <= 0) {
        qCWarning(messageHandlers) << "Invalid user IDs in file transfer message";
        return false;
    }
    
    QJsonObject data = message.data;
    if (data["file_id"].toString().isEmpty()) {
        qCWarning(messageHandlers) << "Missing file ID in file transfer message";
        return false;
    }
    
    if (data["file_name"].toString().isEmpty()) {
        qCWarning(messageHandlers) << "Missing file name in file transfer message";
        return false;
    }
    
    if (data["file_size"].toVariant().toLongLong() <= 0) {
        qCWarning(messageHandlers) << "Invalid file size in file transfer message";
        return false;
    }
    
    return true;
}

bool FileTransferMessageHandler::saveFileMetadata(const Message& message)
{
    if (!m_databasePool) {
        return false;
    }
    
    QString sql = "INSERT INTO file_transfers (file_id, sender_id, receiver_id, file_name, file_size, file_type, status, created_at) "
                  "VALUES (?, ?, ?, ?, ?, ?, 'pending', NOW())";
    
    QVariantList params = {
        message.data["file_id"].toString(),
        message.fromUserId,
        message.toUserId,
        message.data["file_name"].toString(),
        message.data["file_size"].toVariant().toLongLong(),
        message.data["file_type"].toString()
    };
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Write);
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to save file metadata:" << result.error;
        return false;
    }
    
    return true;
}

void FileTransferMessageHandler::notifyFileTransfer(qint64 toUserId, const QJsonObject& fileInfo)
{
    if (!m_connectionManager) {
        return;
    }
    
    auto connection = m_connectionManager->getConnectionByUserId(toUserId);
    if (!connection) {
        qCDebug(messageHandlers) << "Target user" << toUserId << "is not online for file transfer";
        return;
    }
    
    QSslSocket* socket = connection->getSocket();
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    
    QJsonObject notification;
    notification["type"] = "file_transfer_notification";
    notification["file_info"] = fileInfo;
    notification["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(notification);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    socket->write(data);
    socket->flush();
}

// ============================================================================
// LogoutMessageHandler Implementation
// ============================================================================

LogoutMessageHandler::LogoutMessageHandler(ConnectionManager* connectionManager,
                                         SessionManager* sessionManager,
                                         DatabasePool* databasePool)
    : m_connectionManager(connectionManager)
    , m_sessionManager(sessionManager)
    , m_databasePool(databasePool)
{
}

bool LogoutMessageHandler::canHandle(MessageType type) const
{
    return type == MessageType::Logout;
}

bool LogoutMessageHandler::handleMessage(const Message& message)
{
    qCDebug(messageHandlers) << "Handling logout message from" << message.fromUserId;
    
    QJsonObject data = message.data;
    QString sessionToken = data["session_token"].toString();
    
    if (sessionToken.isEmpty()) {
        qCWarning(messageHandlers) << "Missing session token in logout message";
        sendLogoutResponse(message.sourceSocket, false);
        return false;
    }
    
    if (!invalidateSession(sessionToken)) {
        qCWarning(messageHandlers) << "Failed to invalidate session";
        sendLogoutResponse(message.sourceSocket, false);
        return false;
    }
    
    updateUserLastOnline(message.fromUserId);
    
    sendLogoutResponse(message.sourceSocket, true);
    
    qCInfo(messageHandlers) << "User" << message.fromUserId << "logged out successfully";
    return true;
}

QString LogoutMessageHandler::handlerName() const
{
    return "LogoutMessageHandler";
}

bool LogoutMessageHandler::invalidateSession(const QString& sessionToken)
{
    if (!m_sessionManager) {
        return false;
    }
    
    // 从会话管理器移除会话
    if (!m_sessionManager->removeSession(sessionToken)) {
        qCWarning(messageHandlers) << "Failed to remove session from session manager";
        return false;
    }
    
    // 从连接管理器移除连接
    if (m_connectionManager) {
        auto connection = m_connectionManager->getConnectionBySessionToken(sessionToken);
        if (connection) {
            m_connectionManager->removeConnection(connection->getSocket());
        }
    }
    
    return true;
}

void LogoutMessageHandler::updateUserLastOnline(qint64 userId)
{
    if (!m_databasePool) {
        return;
    }
    
    QString sql = "UPDATE users SET last_online = NOW(), status = 'offline' WHERE user_id = ?";
    QVariantList params = {userId};
    
    auto result = m_databasePool->executeQuery(sql, params, DatabaseOperationType::Write);
    if (!result.success) {
        qCWarning(messageHandlers) << "Failed to update user last online:" << result.error;
    }
}

void LogoutMessageHandler::sendLogoutResponse(QSslSocket* socket, bool success)
{
    QJsonObject response;
    response["type"] = "logout_response";
    response["success"] = success;
    response["message"] = success ? "Logout successful" : "Logout failed";
    response["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    
    if (socket && socket->state() == QAbstractSocket::ConnectedState) {
        socket->write(data);
        socket->flush();
    }
}