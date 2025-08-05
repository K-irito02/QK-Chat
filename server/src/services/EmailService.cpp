#include "EmailService.h"
#include "../config/ServerConfig.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QUuid>

#include <QRegularExpression>
#include <QLoggingCategory>
#include <QElapsedTimer>
#include <QSslError>
#include <QJsonObject>

Q_LOGGING_CATEGORY(emailService, "email.service")

EmailService& EmailService::instance() {
    static EmailService instance;
    return instance;
}

EmailService::EmailService(QObject* parent) : QObject(parent),
    m_smtpPort(587), m_useSSL(true), m_useTLS(true),
    m_isReady(false), m_socket(nullptr), m_connected(false), m_smtpReady(false) {

    qCInfo(emailService) << "EmailService constructor called";

    // 启用工作线程，但使用更安全的方式
    moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::started, this, &EmailService::processQueue);
    m_workerThread.start();

    qCInfo(emailService) << "EmailService constructor completed";
}

EmailService::~EmailService() {
    qCInfo(emailService) << "EmailService destructor called";

    // 安全停止工作线程
    if (m_workerThread.isRunning()) {
        m_workerThread.quit();
        if (!m_workerThread.wait(5000)) { // 等待最多5秒
            qCWarning(emailService) << "Worker thread did not stop gracefully";
        }
    }

    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
    }
}

bool EmailService::initialize(const QString& smtpServer, int smtpPort, 
                             const QString& username, const QString& password,
                             bool useSSL, bool useTLS) {
    
    m_smtpServer = smtpServer;
    m_smtpPort = smtpPort;
    m_username = username;
    m_password = password;
    m_useSSL = useSSL;
    m_useTLS = useTLS;
    
    m_isReady = !smtpServer.isEmpty() && !username.isEmpty() && !password.isEmpty();
    
    if (m_isReady) {
        qCInfo(emailService) << "Email service initialized successfully";
        qCInfo(emailService) << "SMTP Server:" << m_smtpServer << ":" << m_smtpPort;
    } else {
        qCWarning(emailService) << "Email service initialization failed - missing configuration";
    }
    
    return m_isReady;
}

bool EmailService::sendEmail(const EmailMessage& message) {
    qCInfo(emailService) << "Attempting to send email to:" << message.to;

    if (!m_isReady) {
        m_lastError = "Email service not initialized";
        qCWarning(emailService) << "Email service not ready:" << m_lastError;
        return false;
    }

    if (!m_socket) {
        qCInfo(emailService) << "Creating new SSL socket for SMTP connection";
        m_socket = new QSslSocket();

        connect(m_socket, &QSslSocket::connected, this, &EmailService::onSocketConnected);
        connect(m_socket, &QSslSocket::disconnected, this, &EmailService::onSocketDisconnected);
        connect(m_socket, &QSslSocket::readyRead, this, &EmailService::onSocketReadyRead);
        connect(m_socket, &QSslSocket::errorOccurred, this, &EmailService::onSocketError);
        connect(m_socket, &QSslSocket::sslErrors, this, &EmailService::onSocketSslErrors);
    }

    if (!m_connected) {
        qCInfo(emailService) << "Connecting to SMTP server:" << m_smtpServer << ":" << m_smtpPort;
        m_socket->connectToHost(m_smtpServer, m_smtpPort);

        if (!m_socket->waitForConnected(10000)) {
            m_lastError = "Connection timeout: " + m_socket->errorString();
            qCWarning(emailService) << "Failed to connect to SMTP server:" << m_lastError;
            return false;
        }
        qCInfo(emailService) << "Successfully connected to SMTP server";
    }
    
    // 发送邮件内容
    QStringList commands;
    commands << "EHLO " + QCoreApplication::instance()->applicationName();

    qCInfo(emailService) << "Sending EHLO command";
    if (!sendSmtpCommand("EHLO " + QCoreApplication::instance()->applicationName())) {
        qCWarning(emailService) << "EHLO command failed:" << m_lastError;
        return false;
    }

    qCInfo(emailService) << "Sending STARTTLS command";
    if (!sendSmtpCommand("STARTTLS")) {
        qCWarning(emailService) << "STARTTLS command failed:" << m_lastError;
        return false;
    }

    // 启动TLS加密
    qCInfo(emailService) << "Starting TLS encryption";
    m_socket->startClientEncryption();
    if (!m_socket->waitForEncrypted(10000)) {
        m_lastError = "TLS encryption failed: " + m_socket->errorString();
        qCWarning(emailService) << "TLS encryption failed:" << m_lastError;
        return false;
    }
    qCInfo(emailService) << "TLS encryption established successfully";
    
    // TLS连接后重新发送EHLO
    qCInfo(emailService) << "Sending EHLO after TLS";
    if (!sendSmtpCommand("EHLO " + QCoreApplication::instance()->applicationName())) {
        qCWarning(emailService) << "EHLO after TLS failed:" << m_lastError;
        return false;
    }

    // 开始认证
    qCInfo(emailService) << "Starting SMTP authentication";
    if (!sendSmtpCommand("AUTH LOGIN")) {
        qCWarning(emailService) << "AUTH LOGIN failed:" << m_lastError;
        return false;
    }

    // 发送用户名
    if (!sendSmtpCommand(QByteArray(m_username.toUtf8()).toBase64())) {
        qCWarning(emailService) << "Username authentication failed:" << m_lastError;
        return false;
    }

    // 发送密码
    if (!sendSmtpCommand(QByteArray(m_password.toUtf8()).toBase64())) {
        qCWarning(emailService) << "Password authentication failed:" << m_lastError;
        return false;
    }
    qCInfo(emailService) << "SMTP authentication successful";

    // 设置发件人和收件人
    if (!sendSmtpCommand("MAIL FROM:<" + m_senderEmail + ">")) {
        qCWarning(emailService) << "MAIL FROM failed:" << m_lastError;
        return false;
    }

    if (!sendSmtpCommand("RCPT TO:<" + message.to + ">")) {
        qCWarning(emailService) << "RCPT TO failed:" << m_lastError;
        return false;
    }

    if (!sendSmtpCommand("DATA")) {
        qCWarning(emailService) << "DATA command failed:" << m_lastError;
        return false;
    }
    
    QString headers;
    headers += "From: " + encodeMimeHeader(m_senderName) + " <" + m_senderEmail + ">\r\n";
    headers += "To: " + message.to + "\r\n";
    headers += "Subject: " + encodeMimeHeader(message.subject) + "\r\n";
    headers += "Date: " + QDateTime::currentDateTime().toString("ddd, dd MMM yyyy hh:mm:ss +0000") + "\r\n";
    headers += "Message-ID: " + generateMessageId() + "\r\n";
    headers += "Content-Type: " + message.contentType + "; charset=UTF-8\r\n";
    headers += "MIME-Version: 1.0\r\n";
    headers += "\r\n";
    
    QString emailData = headers + message.body + "\r\n.";

    // 发送邮件内容
    qCInfo(emailService) << "Sending email content";
    m_socket->write(emailData.toUtf8());
    m_socket->flush();

    if (!waitForResponse(250, 30000)) {
        qCWarning(emailService) << "Failed to send email content:" << m_lastError;
        return false;
    }

    // 发送QUIT命令
    if (!sendSmtpCommand("QUIT")) {
        qCWarning(emailService) << "QUIT command failed:" << m_lastError;
        return false;
    }

    qCInfo(emailService) << "Email sent successfully to:" << message.to;
    return true;
}

bool EmailService::sendEmailAsync(const EmailMessage& message) {
    if (!m_isReady) {
        m_lastError = "Email service not initialized";
        return false;
    }
    
    {
        QMutexLocker locker(&m_mutex);
        m_emailQueue.enqueue(message);
        m_condition.wakeOne();
    }
    
    return true;
}

void EmailService::setSenderInfo(const QString& senderEmail, const QString& senderName) {
    m_senderEmail = senderEmail;
    m_senderName = senderName;
}

bool EmailService::isReady() const {
    return m_isReady;
}

QString EmailService::getLastError() const {
    return m_lastError;
}



// 获取邮件服务状态信息
QJsonObject EmailService::getServiceStatus() const
{
    QJsonObject status;
    status["ready"] = m_isReady;
    status["connected"] = m_connected;
    status["smtp_ready"] = m_smtpReady;
    status["smtp_server"] = m_smtpServer;
    status["smtp_port"] = m_smtpPort;
    status["use_ssl"] = m_useSSL;
    status["use_tls"] = m_useTLS;
    status["sender_email"] = m_senderEmail;
    status["sender_name"] = m_senderName;
    status["queue_size"] = m_emailQueue.size();
    status["last_error"] = m_lastError;
    
    return status;
}

void EmailService::processQueue() {
    while (true) {
        EmailMessage message;
        
        {
            QMutexLocker locker(&m_mutex);
            while (m_emailQueue.isEmpty()) {
                m_condition.wait(&m_mutex);
            }
            if (!m_emailQueue.isEmpty()) {
                message = m_emailQueue.dequeue();
            }
        }
        
        if (!message.to.isEmpty()) {
            sendEmailInternal(message);
        }
    }
}

void EmailService::sendEmailInternal(const EmailMessage& message) {
    m_lastError.clear();
    
    if (!m_socket) {
        m_socket = new QSslSocket();
        
        connect(m_socket, &QSslSocket::connected, this, &EmailService::onSocketConnected);
        connect(m_socket, &QSslSocket::disconnected, this, &EmailService::onSocketDisconnected);
        connect(m_socket, &QSslSocket::readyRead, this, &EmailService::onSocketReadyRead);
        connect(m_socket, &QSslSocket::errorOccurred, this, &EmailService::onSocketError);
        connect(m_socket, &QSslSocket::sslErrors, this, &EmailService::onSocketSslErrors);
    }
    
    if (!m_connected) {
        m_socket->connectToHost(m_smtpServer, m_smtpPort);
        
        if (!m_socket->waitForConnected(10000)) {
            m_lastError = "Connection timeout: " + m_socket->errorString();
            emit emailFailed(message.to, m_lastError);
            return;
        }
        
        if (m_useSSL) {
            m_socket->startClientEncryption();
            if (!m_socket->waitForEncrypted(10000)) {
                m_lastError = "SSL handshake failed: " + m_socket->errorString();
                emit emailFailed(message.to, m_lastError);
                return;
            }
        }
    }
    
    // 发送邮件内容
    QStringList commands;
    commands << "EHLO " + QCoreApplication::instance()->applicationName();
    
    // 如果使用TLS但还没有加密，先发送STARTTLS
    if (m_useTLS && !m_socket->isEncrypted()) {
        commands << "STARTTLS";
        if (!sendSmtpCommand("STARTTLS")) {
            emit emailFailed(message.to, m_lastError);
            return;
        }
        
        // 启动TLS加密
        m_socket->startClientEncryption();
        if (!m_socket->waitForEncrypted(10000)) {
            m_lastError = "TLS handshake failed: " + m_socket->errorString();
            emit emailFailed(message.to, m_lastError);
            return;
        }
        
        // TLS连接后重新发送EHLO
        commands << "EHLO " + QCoreApplication::instance()->applicationName();
    }
    
    commands << "AUTH LOGIN";
    commands << QByteArray(m_username.toUtf8()).toBase64();
    commands << QByteArray(m_password.toUtf8()).toBase64();
    commands << "MAIL FROM:<" + m_senderEmail + ">";
    commands << "RCPT TO:<" + message.to + ">";
    commands << "DATA";
    
    QString headers;
    headers += "From: " + encodeMimeHeader(m_senderName) + " <" + m_senderEmail + ">\r\n";
    headers += "To: " + message.to + "\r\n";
    headers += "Subject: " + encodeMimeHeader(message.subject) + "\r\n";
    headers += "Date: " + QDateTime::currentDateTime().toString("ddd, dd MMM yyyy hh:mm:ss +0000") + "\r\n";
    headers += "Message-ID: " + generateMessageId() + "\r\n";
    headers += "Content-Type: " + message.contentType + "; charset=UTF-8\r\n";
    headers += "MIME-Version: 1.0\r\n";
    headers += "\r\n";
    
    QString emailData = headers + message.body + "\r\n.\r\n";
    commands << emailData;
    commands << "QUIT";
    
    foreach (const QString& command, commands) {
        if (!sendSmtpCommand(command)) {
            emit emailFailed(message.to, m_lastError);
            return;
        }
    }
    
    emit emailSent(message.to);
}

bool EmailService::sendSmtpCommand(const QString& command) {
    if (!m_socket || !m_socket->isOpen()) {
        m_lastError = "Socket not connected";
        qCWarning(emailService) << "SMTP command failed - socket not connected:" << command;
        return false;
    }

    // 不记录包含密码的命令
    QString logCommand = command;
    if (command.contains("AUTH") && command.length() > 20) {
        logCommand = "AUTH [HIDDEN]";
    }
    qCDebug(emailService) << "Sending SMTP command:" << logCommand;

    m_socket->write((command + "\r\n").toUtf8());
    m_socket->flush();

    bool success = waitForResponse(250, 30000);
    if (!success) {
        qCWarning(emailService) << "SMTP command failed:" << logCommand << "Error:" << m_lastError;
    }

    return success;
}

bool EmailService::waitForResponse(int expectedCode, int timeoutMs) {
    m_responseBuffer.clear();
    
    QElapsedTimer timer;
    timer.start();
    
    while (timer.elapsed() < timeoutMs) {
        if (m_socket->bytesAvailable() > 0) {
            m_responseBuffer += m_socket->readAll();
            
            QStringList lines = QString(m_responseBuffer).split("\r\n");
            for (const QString& line : lines) {
                if (line.startsWith(QString::number(expectedCode))) {
                    return true;
                }
                if (line.startsWith("5") || line.startsWith("4")) {
                    m_lastError = "SMTP Error: " + line;
                    return false;
                }
            }
        }
        
        m_socket->waitForReadyRead(100);
    }
    
    m_lastError = "Response timeout";
    return false;
}

QString EmailService::generateMessageId() const {
    return QUuid::createUuid().toString().remove('{').remove('}') + "@" + 
           QCoreApplication::instance()->applicationName();
}

QString EmailService::encodeMimeHeader(const QString& text) const {
    if (text.isEmpty()) return text;
    
    // 如果已经是ASCII，直接返回
    if (text.toLatin1() == text) {
        return text;
    }
    
    // 编码为非ASCII字符
    return "=?UTF-8?B?" + text.toUtf8().toBase64() + "?=";
}

void EmailService::onSocketConnected() {
    m_connected = true;
    qCInfo(emailService) << "Connected to SMTP server";
}

void EmailService::onSocketDisconnected() {
    m_connected = false;
    qCInfo(emailService) << "Disconnected from SMTP server";
}

void EmailService::onSocketReadyRead() {
    m_responseBuffer += m_socket->readAll();
}

void EmailService::onSocketError(QAbstractSocket::SocketError error) {
    Q_UNUSED(error)
    m_lastError = m_socket->errorString();
    qCWarning(emailService) << "Socket error:" << m_lastError;
}

void EmailService::onSocketSslErrors(const QList<QSslError>& errors) {
    for (const QSslError& error : errors) {
        qCWarning(emailService) << "SSL error:" << error.errorString();
    }
    
    // 在开发环境中忽略SSL错误
    m_socket->ignoreSslErrors();
}