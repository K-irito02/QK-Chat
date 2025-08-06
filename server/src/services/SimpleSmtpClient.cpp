#include "SimpleSmtpClient.h"
#include <QLoggingCategory>
#include <QByteArray>
#include <QSslConfiguration>

Q_LOGGING_CATEGORY(smtpClient, "qkchat.server.smtp")

SimpleSmtpClient::SimpleSmtpClient(QObject *parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_port(587)
    , m_timeoutTimer(nullptr)
    , m_step(Connected)
    , m_authenticated(false)
{
    m_timeoutTimer = new QTimer(this);
    m_timeoutTimer->setSingleShot(true);
    connect(m_timeoutTimer, &QTimer::timeout, this, &SimpleSmtpClient::onTimeout);
}

SimpleSmtpClient::~SimpleSmtpClient()
{
    if (m_socket) {
        m_socket->disconnect();
        m_socket->deleteLater();
    }
}

void SimpleSmtpClient::setSmtpConfig(const QString &host, int port, const QString &username, const QString &password)
{
    m_host = host;
    m_port = port;
    m_username = username;
    m_password = password;
}

void SimpleSmtpClient::sendEmail(const QString &to, const QString &subject, const QString &content)
{
    m_toEmail = to;
    m_subject = subject;
    m_content = content;
    m_step = Connected;
    m_authenticated = false;
    
    if (m_socket) {
        m_socket->disconnect();
        m_socket->deleteLater();
    }
    
    m_socket = new QSslSocket(this);
    
    if (!m_socket) {
        qCWarning(smtpClient) << "Failed to create QSslSocket";
        emit emailSent(m_toEmail, false, "创建SSL连接失败");
        return;
    }
    
    // 配置SSL设置，忽略开发环境的证书验证
    QSslConfiguration sslConfig = m_socket->sslConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);
    sslConfig.setProtocol(QSsl::TlsV1_2);
    m_socket->setSslConfiguration(sslConfig);
    
    connect(m_socket, &QSslSocket::connected, this, &SimpleSmtpClient::onConnected);
    connect(m_socket, &QSslSocket::encrypted, this, &SimpleSmtpClient::onEncrypted);
    connect(m_socket, &QSslSocket::readyRead, this, &SimpleSmtpClient::onReadyRead);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::errorOccurred),
            this, &SimpleSmtpClient::onError);
    connect(m_socket, &QSslSocket::disconnected, this, &SimpleSmtpClient::onDisconnected);
    
    qCInfo(smtpClient) << "Connecting to SMTP server:" << m_host << ":" << m_port;
    m_socket->connectToHost(m_host, m_port);
    
    if (m_timeoutTimer) {
        m_timeoutTimer->start(30000); // 30秒超时
    }
}

void SimpleSmtpClient::onConnected()
{
    qCInfo(smtpClient) << "Connected to SMTP server";
    m_timeoutTimer->stop();
    m_timeoutTimer->start(10000); // 10秒超时
    
    // 发送EHLO命令
    sendCommand("EHLO localhost");
    m_step = EhloSent;
}

void SimpleSmtpClient::onEncrypted()
{
    qCInfo(smtpClient) << "SMTP connection encrypted";
}

void SimpleSmtpClient::onReadyRead()
{
    QString response = QString::fromUtf8(m_socket->readAll());
    qCDebug(smtpClient) << "SMTP Response:" << response.trimmed();
    
    processSmtpResponse(response);
}

void SimpleSmtpClient::onError(QAbstractSocket::SocketError error)
{
    qCWarning(smtpClient) << "SMTP connection error:" << error;
    emit connectionError(QString("连接错误: %1").arg(error));
    emit emailSent(m_toEmail, false, "连接失败");
}

void SimpleSmtpClient::onDisconnected()
{
    qCInfo(smtpClient) << "SMTP connection closed";
    m_timeoutTimer->stop();
}

void SimpleSmtpClient::onTimeout()
{
    qCWarning(smtpClient) << "SMTP operation timeout";
    emit emailSent(m_toEmail, false, "操作超时");
    if (m_socket) {
        m_socket->disconnectFromHost();
    }
}

void SimpleSmtpClient::sendCommand(const QString &command)
{
    qCDebug(smtpClient) << "Sending command:" << command.trimmed();
    
    if (!m_socket) {
        qCWarning(smtpClient) << "Socket is null, cannot send command";
        return;
    }
    
    m_socket->write(command.toUtf8() + "\r\n");
    
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
        m_timeoutTimer->start(10000); // 10秒超时
    }
}

void SimpleSmtpClient::processSmtpResponse(const QString &response)
{
    if (m_timeoutTimer) {
        m_timeoutTimer->stop();
    }
    
    if (response.contains("220")) {
        // 服务器就绪
        if (m_step == Connected) {
            sendCommand("EHLO localhost");
            m_step = EhloSent;
        }
    } else if (response.contains("250")) {
        // 命令成功
        if (m_step == EhloSent) {
            sendCommand("STARTTLS");
            m_step = StartTlsSent;
        } else if (m_step == StartTlsSent) {
            if (m_socket) {
                m_socket->startClientEncryption();
            }
            m_step = Authenticating;
        } else if (m_step == Authenticating) {
            startAuthentication();
        } else if (m_step == PasswordSent) {
            m_authenticated = true;
            sendCommand("MAIL FROM:<" + m_username + ">");
            m_step = MailFromSent;
        } else if (m_step == MailFromSent) {
            sendCommand("RCPT TO:<" + m_toEmail + ">");
            m_step = RcptToSent;
        } else if (m_step == RcptToSent) {
            sendCommand("DATA");
            m_step = DataSent;
        } else if (m_step == DataSent) {
            sendMailData();
        } else if (m_step == ContentSent) {
            sendCommand("QUIT");
            m_step = QuitSent;
        } else if (m_step == QuitSent) {
            emit emailSent(m_toEmail, true, "邮件发送成功");
            if (m_socket) {
                m_socket->disconnectFromHost();
            }
        }
    } else if (response.contains("334")) {
        // 需要认证信息
        if (m_step == Authenticating) {
            // 发送Base64编码的用户名
            QByteArray username = m_username.toUtf8().toBase64();
            sendCommand(username);
            m_step = UsernameSent;
        } else if (m_step == UsernameSent) {
            // 发送Base64编码的密码
            QByteArray password = m_password.toUtf8().toBase64();
            sendCommand(password);
            m_step = PasswordSent;
        }
    } else if (response.contains("354")) {
        // 可以发送邮件内容
        sendMailData();
    } else if (response.contains("535") || response.contains("550")) {
        // 认证失败
        qCWarning(smtpClient) << "SMTP authentication failed";
        emit emailSent(m_toEmail, false, "认证失败");
        if (m_socket) {
            m_socket->disconnectFromHost();
        }
    } else {
        // 其他错误
        qCWarning(smtpClient) << "SMTP error response:" << response;
        emit emailSent(m_toEmail, false, "发送失败");
        if (m_socket) {
            m_socket->disconnectFromHost();
        }
    }
    
    if (m_timeoutTimer) {
        m_timeoutTimer->start(10000); // 10秒超时
    }
}

void SimpleSmtpClient::startAuthentication()
{
    sendCommand("AUTH LOGIN");
    m_step = Authenticating;
}

void SimpleSmtpClient::sendMailData()
{
    QString emailContent = QString(
        "From: %1 <%2>\r\n"
        "To: %3\r\n"
        "Subject: %4\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "\r\n"
        "%5\r\n"
        ".\r\n"
    ).arg("QK Chat", m_username, m_toEmail, m_subject, m_content);
    
    qCDebug(smtpClient) << "Sending email content...";
    
    if (m_socket) {
        m_socket->write(emailContent.toUtf8());
        m_step = ContentSent;
    } else {
        qCWarning(smtpClient) << "Socket is null, cannot send mail data";
        emit emailSent(m_toEmail, false, "连接已断开");
    }
}