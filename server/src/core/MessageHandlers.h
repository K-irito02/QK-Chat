#ifndef MESSAGEHANDLERS_H
#define MESSAGEHANDLERS_H

#include "MessageEngine.h"
#include "ConnectionManager.h"
#include "SessionManager.h"
#include "../database/DatabasePool.h"
#include "../cache/CacheManagerV2.h"
#include "../services/EmailVerificationService.h"
#include <QJsonObject>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(messageHandlers)

/**
 * @brief 登录消息处理器
 */
class LoginMessageHandler : public MessageHandler
{
public:
    explicit LoginMessageHandler(ConnectionManager* connectionManager,
                                SessionManager* sessionManager,
                                DatabasePool* databasePool,
                                CacheManagerV2* cacheManager);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    ConnectionManager* m_connectionManager;
    SessionManager* m_sessionManager;
    DatabasePool* m_databasePool;
    CacheManagerV2* m_cacheManager;
    
    bool authenticateUser(const QString& username, const QString& password, qint64& userId);
    bool createUserSession(qint64 userId, QSslSocket* socket, QString& sessionToken);
    void sendLoginResponse(QSslSocket* socket, bool success, const QString& message, 
                          qint64 userId = 0, const QString& sessionToken = QString());
};

/**
 * @brief 聊天消息处理器
 */
class ChatMessageHandler : public MessageHandler
{
public:
    explicit ChatMessageHandler(ConnectionManager* connectionManager,
                               DatabasePool* databasePool,
                               CacheManagerV2* cacheManager);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    ConnectionManager* m_connectionManager;
    DatabasePool* m_databasePool;
    CacheManagerV2* m_cacheManager;
    
    bool validateChatMessage(const Message& message);
    bool saveMessageToDatabase(const Message& message);
    bool deliverMessage(const Message& message);
    void sendDeliveryConfirmation(QSslSocket* socket, const QString& messageId, bool delivered);
};

/**
 * @brief 心跳消息处理器
 */
class HeartbeatMessageHandler : public MessageHandler
{
public:
    explicit HeartbeatMessageHandler(ConnectionManager* connectionManager);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    ConnectionManager* m_connectionManager;
    
    void sendHeartbeatResponse(QSslSocket* socket);
};

/**
 * @brief 注册消息处理器
 */
class RegisterMessageHandler : public MessageHandler
{
public:
    explicit RegisterMessageHandler(ConnectionManager* connectionManager,
                                   DatabasePool* databasePool,
                                   CacheManagerV2* cacheManager,
                                   EmailVerificationService* emailService);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    ConnectionManager* m_connectionManager;
    DatabasePool* m_databasePool;
    CacheManagerV2* m_cacheManager;
    EmailVerificationService* m_emailService;
    
    bool validateRegistrationData(const QJsonObject& data);
    bool checkUserExists(const QString& username);
    bool checkEmailExists(const QString& email);
    bool createUser(const QJsonObject& data, qint64& userId);
    void sendRegistrationResponse(QSslSocket* socket, bool success, const QString& message, qint64 userId = 0);
};

/**
 * @brief 用户状态消息处理器
 */
class UserStatusMessageHandler : public MessageHandler
{
public:
    explicit UserStatusMessageHandler(ConnectionManager* connectionManager,
                                     DatabasePool* databasePool,
                                     CacheManagerV2* cacheManager);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    ConnectionManager* m_connectionManager;
    DatabasePool* m_databasePool;
    CacheManagerV2* m_cacheManager;
    
    bool updateUserStatus(qint64 userId, const QString& status);
    void broadcastStatusChange(qint64 userId, const QString& status);
};

/**
 * @brief 群聊消息处理器
 */
class GroupChatMessageHandler : public MessageHandler
{
public:
    explicit GroupChatMessageHandler(ConnectionManager* connectionManager,
                                    DatabasePool* databasePool,
                                    CacheManagerV2* cacheManager);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    ConnectionManager* m_connectionManager;
    DatabasePool* m_databasePool;
    CacheManagerV2* m_cacheManager;
    
    bool validateGroupMessage(const Message& message);
    QList<qint64> getGroupMembers(qint64 groupId);
    bool saveGroupMessage(const Message& message);
    bool deliverToGroupMembers(const Message& message, const QList<qint64>& members);
};



/**
 * @brief 系统通知消息处理器
 */
class SystemNotificationHandler : public MessageHandler
{
public:
    explicit SystemNotificationHandler(ConnectionManager* connectionManager,
                                      CacheManagerV2* cacheManager);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    ConnectionManager* m_connectionManager;
    CacheManagerV2* m_cacheManager;
    
    bool validateNotification(const Message& message);
    void broadcastSystemNotification(const QJsonObject& notification);
    void sendTargetedNotification(const QList<qint64>& userIds, const QJsonObject& notification);
};

/**
 * @brief 文件传输消息处理器
 */
class FileTransferMessageHandler : public MessageHandler
{
public:
    explicit FileTransferMessageHandler(ConnectionManager* connectionManager,
                                       DatabasePool* databasePool,
                                       CacheManagerV2* cacheManager);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    ConnectionManager* m_connectionManager;
    DatabasePool* m_databasePool;
    CacheManagerV2* m_cacheManager;
    
    bool validateFileTransfer(const Message& message);
    bool saveFileMetadata(const Message& message);
    void notifyFileTransfer(qint64 toUserId, const QJsonObject& fileInfo);
};

/**
 * @brief 邮箱验证消息处理器
 */
class EmailVerificationMessageHandler : public MessageHandler
{
public:
    explicit EmailVerificationMessageHandler(EmailVerificationService* emailService);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    EmailVerificationService* m_emailService;
    
    void sendVerificationResponse(QSslSocket* socket, bool success, const QString& message);
    void sendCodeSentResponse(QSslSocket* socket, bool success, const QString& message);
};

/**
 * @brief 验证消息处理器
 */
class ValidationMessageHandler : public MessageHandler
{
public:
    explicit ValidationMessageHandler(DatabasePool* databasePool, EmailVerificationService* emailService);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    DatabasePool* m_databasePool;
    EmailVerificationService* m_emailService;
    
    void handleUsernameValidation(const Message& message);
    void handleEmailValidation(const Message& message);
    void sendValidationResponse(QSslSocket* socket, const QString& type, bool available, const QString& message);
};

/**
 * @brief 登出消息处理器
 */
class LogoutMessageHandler : public MessageHandler
{
public:
    explicit LogoutMessageHandler(ConnectionManager* connectionManager,
                                 SessionManager* sessionManager,
                                 DatabasePool* databasePool);

    bool canHandle(MessageType type) const override;
    bool handleMessage(const Message& message) override;
    QString handlerName() const override;

private:
    ConnectionManager* m_connectionManager;
    SessionManager* m_sessionManager;
    DatabasePool* m_databasePool;
    
    bool invalidateSession(const QString& sessionToken);
    void updateUserLastOnline(qint64 userId);
    void sendLogoutResponse(QSslSocket* socket, bool success);
};

#endif // MESSAGEHANDLERS_H
