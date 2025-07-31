#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QSslSocket>
#include <QTimer>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

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
    
signals:
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    
    void loginResponse(bool success, const QString &message, const QString &token);
    void registerResponse(bool success, const QString &message);
    void logoutResponse(bool success);
    
    void captchaReceived(const QString &captchaImage);
    
    void usernameAvailability(bool available);
    void emailAvailability(bool available);
    
    void avatarUploaded(bool success, const QUrl &avatarUrl);
    
    void messageReceived(const QString &sender, const QString &content, const QString &messageType, qint64 timestamp);
    void messageSent(const QString &messageId);
    void messageDelivered(const QString &messageId);
    
    void networkError(const QString &error);
    
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
    void processIncomingData();
    void sendData(const QByteArray &data);
    QByteArray createPacket(const QString &type, const QVariantMap &data);
    void parsePacket(const QByteArray &packet);
    
    QSslSocket *_sslSocket;
    QTimer *_heartbeatTimer;
    QNetworkAccessManager *_networkManager;
    
    QString _serverHost;
    int _serverPort;
    bool _isConnected;
    
    QByteArray _readBuffer;
    
    static const int HEARTBEAT_INTERVAL = 30000; // 30 seconds
    static const int CONNECTION_TIMEOUT = 10000; // 10 seconds
};

#endif // NETWORKCLIENT_H 