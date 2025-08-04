#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QSslSocket>
#include <QTimer>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QVariantMap>
#include "SSLConfigManager.h"
#include "ConnectionStateManager.h"
#include "ErrorHandler.h"
#include "ReconnectManager.h"
#include "HeartbeatManager.h"

/**
 * @brief 网络客户端类
 * 
 * 负责与服务器的SSL通信，包括：
 * - 用户登录/注册
 * - 消息发送/接收
 * - 文件传输
 * - 心跳检测
 */
class NetworkClient : public QObject
{
    Q_OBJECT
    
public:
    explicit NetworkClient(QObject *parent = nullptr);
    ~NetworkClient();
    
    // 连接管理
    bool connectToServer(const QString &host, int port);
    void disconnect();
    bool isConnected() const;
    
    // 用户认证
    void login(const QString &usernameOrEmail, const QString &password, const QString &captcha = "");
    void registerUser(const QString &username, const QString &email, const QString &password, const QUrl &avatar);
    void logout();
    
    // 邮箱验证
    void verifyEmail(const QString &token);
    void sendEmailVerification(const QString &email);
    void resendVerification(const QString &email);
    
    // 验证码
    void requestCaptcha();
    
    // 用户名/邮箱可用性检查
    void checkUsernameAvailability(const QString &username);
    void checkEmailAvailability(const QString &email);
    
    // 头像上传
    void uploadAvatar(const QUrl &filePath);
    
    // 消息发送
    void sendMessage(const QString &receiver, const QString &content, const QString &messageType = "text");
    
    // 心跳检测
    void startHeartbeat();
    void stopHeartbeat();
    void sendHeartbeat();
    
    void verifyEmailCode(const QString &email, const QString &code);
    
signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    
    void loginResponse(bool success, const QString &message, const QString &token);
    void registerResponse(bool success, const QString &message, const QString &username, const QString &email, qint64 userId);
    void verifyEmailResponse(bool success, const QString &message);
    void resendVerificationResponse(bool success, const QString &message);
    void logoutResponse(bool success);
    
    void captchaReceived(const QString &captchaImage);
    
    void usernameAvailability(bool available);
    void emailAvailability(bool available);
    
    void avatarUploaded(bool success, const QUrl &avatarUrl);
    
    void messageReceived(const QString &sender, const QString &content, const QString &messageType, qint64 timestamp);
    void messageSent(const QString &messageId);
    void messageDelivered(const QString &messageId);
    
    void networkError(const QString &error);
    void emailCodeVerified(bool success, const QString &message);
    
private slots:
    void onConnected();
    void onDisconnected();
    void onSslErrors(const QList<QSslError> &errors);
    void onReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onHeartbeatTimeout();
    void onNetworkReplyFinished();
    
private:
    void setupSslSocket();
    void setupStateManager();
    void setupErrorHandler();
    void setupReconnectManager();
    void setupHeartbeatManager();
    void updateFromDevelopmentConfig();
    void processIncomingData();
    void sendData(const QByteArray &data);
    QByteArray createPacket(const QString &type, const QVariantMap &data);
    void parsePacket(const QByteArray &packet);
    
    // 响应处理函数
    void handleAuthResponse(const QVariantMap &data);
    void handleMessageResponse(const QVariantMap &data);
    void handleValidationResponse(const QVariantMap &data);
    void handleHeartbeatResponse(const QVariantMap &data);
    void handleErrorResponse(const QVariantMap &data);
    
    QSslSocket *_sslSocket;
    QTimer *_heartbeatTimer;
    QNetworkAccessManager *_networkManager;
    ConnectionStateManager *_stateManager;
    ErrorHandler *_errorHandler;
    ReconnectManager *_reconnectManager;
    HeartbeatManager *_heartbeatManager;

    QString _serverHost;
    int _serverPort;
    bool _isConnected;
    QByteArray _readBuffer;
    QString _authToken;
    
    static const int HEARTBEAT_INTERVAL = 30000; // 30 seconds
    static const int CONNECTION_TIMEOUT = 10000; // 10 seconds
};

#endif // NETWORKCLIENT_H 