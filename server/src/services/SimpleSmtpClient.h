#ifndef SIMPLESMTPCLIENT_H
#define SIMPLESMTPCLIENT_H

#include <QObject>
#include <QSslSocket>
#include <QString>
#include <QTimer>

class SimpleSmtpClient : public QObject
{
    Q_OBJECT

public:
    explicit SimpleSmtpClient(QObject *parent = nullptr);
    ~SimpleSmtpClient();

    // 发送邮件
    void sendEmail(const QString &to, const QString &subject, const QString &content);
    
    // 设置SMTP配置
    void setSmtpConfig(const QString &host, int port, const QString &username, const QString &password);

signals:
    void emailSent(const QString &to, bool success, const QString &message);
    void connectionError(const QString &error);

private slots:
    void onConnected();
    void onEncrypted();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);
    void onDisconnected();
    void onTimeout();

private:
    void sendCommand(const QString &command);
    void processSmtpResponse(const QString &response);
    void startAuthentication();
    void sendMailData();

    QSslSocket *m_socket;
    QString m_host;
    int m_port;
    QString m_username;
    QString m_password;
    QString m_fromEmail;
    QString m_toEmail;
    QString m_subject;
    QString m_content;
    
    QTimer *m_timeoutTimer;
    int m_step;
    bool m_authenticated;
    
    enum SmtpStep {
        Connected,
        EhloSent,
        StartTlsSent,
        Authenticating,
        AuthLoginSent,
        UsernameSent,
        PasswordSent,
        MailFromSent,
        RcptToSent,
        DataSent,
        ContentSent,
        QuitSent
    };
};

#endif // SIMPLESMTPCLIENT_H