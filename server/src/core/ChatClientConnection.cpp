#include "ChatClientConnection.h"
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

ChatClientConnection::ChatClientConnection(QSslSocket* sock, QObject* parent)
    : QObject(parent)
    , socket(sock)
    , userId(0)
    , lastActivity(QDateTime::currentDateTime())
{
    if (socket) {
        socket->setParent(this);
        
        // 连接信号槽
        connect(socket, &QSslSocket::disconnected, this, &ChatClientConnection::disconnected);
        connect(socket, &QSslSocket::readyRead, this, [this]() {
            readBuffer.append(socket->readAll());
            emit dataReceived();
        });
        connect(socket, &QSslSocket::errorOccurred,
                this, [this](QAbstractSocket::SocketError socketError) {
            Q_UNUSED(socketError)
            QString errorMessage = QString("Socket error: %1").arg(socket->errorString());
            qWarning() << "ChatClientConnection:" << errorMessage;
            emit error(errorMessage);
        });
        
        connect(socket, &QSslSocket::sslErrors,
                this, [this](const QList<QSslError>& errors) {
            QString errorMessage;
            for (const QSslError& error : errors) {
                errorMessage += error.errorString() + "\n";
            }
            qWarning() << "ChatClientConnection SSL errors:" << errorMessage;
            emit error(errorMessage);
        });
    }
}

ChatClientConnection::~ChatClientConnection()
{
    if (socket) {
        socket->disconnect(); // 断开所有信号槽连接
        socket->deleteLater();
        socket = nullptr;
    }
}

void ChatClientConnection::clearReadBuffer()
{
    readBuffer.clear();
}

QByteArray ChatClientConnection::takeReadBuffer()
{
    QByteArray data = readBuffer;
    readBuffer.clear();
    return data;
}

bool ChatClientConnection::isConnected() const
{
    return socket && socket->state() == QAbstractSocket::ConnectedState;
}

bool ChatClientConnection::write(const QByteArray& data)
{
    if (!isConnected()) {
        qWarning() << "ChatClientConnection: Attempting to write to disconnected socket";
        return false;
    }
    
    qint64 bytesWritten = socket->write(data);
    if (bytesWritten < 0) {
        qWarning() << "ChatClientConnection: Failed to write data:" << socket->errorString();
        return false;
    }
    
    return true;
}

bool ChatClientConnection::write(const QJsonObject& json)
{
    QJsonDocument doc(json);
    return write(doc.toJson(QJsonDocument::Compact));
}