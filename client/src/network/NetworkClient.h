#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QSslSocket>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QDateTime>
#include <QThread>
#include <QElapsedTimer>
#include <QMutex>
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
    
    // 连接状态
    bool isConnected() const;
    bool isSslEncrypted() const;
    bool connectToServer(const QString &host, int port);
    void disconnect();
    
    // 用户认证
    void login(const QString &usernameOrEmail, const QString &password, const QString &captcha = "");
    void registerUser(const QString &username, const QString &email, const QString &verificationCode, const QString &password, const QUrl &avatar);
    void logout();
    

    
    // 验证码
    void requestCaptcha();
    
    // 用户名/邮箱可用性检查
    void checkUsernameAvailability(const QString &username);
    void checkEmailAvailability(const QString &email);
    
    // 邮箱验证
    void sendEmailVerificationCode(const QString &email);
    void verifyEmailCode(const QString &email, const QString &code);

    
    // 头像上传
    void uploadAvatar(const QUrl &filePath);
    
    // 消息发送
    void sendMessage(const QString &receiver, const QString &content, const QString &messageType = "text");
    
    // 心跳检测
    void startHeartbeat();
    void stopHeartbeat();
    void sendHeartbeat();
    

    
signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    
    void loginResponse(bool success, const QString &message);
    void registerResponse(bool success, const QString &message);

    void logoutResponse(bool success);
    
    void captchaReceived(const QString &captchaImage);
    
    void usernameAvailability(bool available);
    void emailAvailability(bool available);
    
    void emailVerificationCodeSent(bool success, const QString &message);
    void emailVerificationCodeVerified(bool success, const QString &message);

    
    void avatarUploaded(bool success, const QUrl &avatarUrl);
    
    void messageReceived(const QString &sender, const QString &content, const QString &messageType, qint64 timestamp);
    void messageSent(const QString &messageId);
    void messageDelivered(const QString &messageId);
    
    void networkError(const QString &error);

    
private slots:
    void onConnected();
    void onDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);
    void onSslErrors(const QList<QSslError> &errors);
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
    void handleAuthResponse(const QJsonObject& response);
    void handleMessageResponse(const QVariantMap &data);
    void handleValidationResponse(const QVariantMap &data);
    void handleHeartbeatResponse(const QVariantMap &data);
    void handleErrorResponse(const QVariantMap &data);
    void handleEmailVerificationResponse(const QVariantMap &data);
    void handleEmailCodeSentResponse(const QVariantMap &data);
    
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
    QMutex *_sendMutex; // 线程安全保护
    
    static const int HEARTBEAT_INTERVAL = 30000; // 30 seconds
    static const int CONNECTION_TIMEOUT = 10000; // 10 seconds
};

#endif // NETWORKCLIENT_H 