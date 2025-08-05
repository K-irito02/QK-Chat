#ifndef EMAILSERVICE_H
#define EMAILSERVICE_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QThread>
#include <functional>
#include <memory>

struct EmailMessage {
    QString to;
    QString subject;
    QString body;
    QStringList attachments;
    QString contentType;
    
    EmailMessage() : contentType("text/html") {}
    
    EmailMessage(const QString& to, const QString& subject, const QString& body)
        : to(to), subject(subject), body(body), contentType("text/html") {}
};

class EmailService : public QObject {
    Q_OBJECT

public:
    static EmailService& instance();
    
    bool initialize(const QString& smtpServer, int smtpPort, 
                   const QString& username, const QString& password,
                   bool useSSL = true, bool useTLS = true);
    
    bool sendEmail(const EmailMessage& message);
    bool sendEmailAsync(const EmailMessage& message);
    
    void setSenderInfo(const QString& senderEmail, const QString& senderName);
    
    bool isReady() const;
    QString getLastError() const;
    
    // 获取服务状态
    QJsonObject getServiceStatus() const;

signals:
    void emailSent(const QString& recipient);
    void emailFailed(const QString& recipient, const QString& error);

private:
    explicit EmailService(QObject* parent = nullptr);
    ~EmailService();
    
    void processQueue();
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketSslErrors(const QList<QSslError>& errors);
    
    bool sendSmtpCommand(const QString& command);
    bool waitForResponse(int expectedCode, int timeoutMs = 30000);
    QString generateMessageId() const;
    QString encodeMimeHeader(const QString& text) const;
    void sendEmailInternal(const EmailMessage& message);
    
    QString m_smtpServer;
    int m_smtpPort;
    QString m_username;
    QString m_password;
    QString m_senderEmail;
    QString m_senderName;
    bool m_useSSL;
    bool m_useTLS;
    bool m_isReady;
    
    QSslSocket* m_socket;
    QQueue<EmailMessage> m_emailQueue;
    QMutex m_mutex;
    QWaitCondition m_condition;
    QThread m_workerThread;
    
    QString m_lastError;
    QString m_responseBuffer;
    bool m_connected;
    bool m_smtpReady;
    
    Q_DISABLE_COPY(EmailService)
};

#endif // EMAILSERVICE_H