#ifndef CHATCLIENTCONNECTION_H
#define CHATCLIENTCONNECTION_H

#include <QSslSocket>
#include <QDateTime>
#include <QObject>
#include <QJsonObject>

class ChatClientConnection : public QObject {
    Q_OBJECT

public:
    explicit ChatClientConnection(QSslSocket* sock = nullptr, QObject* parent = nullptr);
    ~ChatClientConnection();

    void updateActivity() { lastActivity = QDateTime::currentDateTime(); }
    QDateTime getLastActivity() const { return lastActivity; }
    QSslSocket* getSocket() const { return socket; }
    qint64 getUserId() const { return userId; }
    void setUserId(qint64 id) { userId = id; }
    QString getSessionToken() const { return sessionToken; }
    void setSessionToken(const QString& token) { sessionToken = token; }
    QByteArray& getReadBuffer() { return readBuffer; }
    void clearReadBuffer();
    QByteArray takeReadBuffer();
    bool isConnected() const;
    bool write(const QByteArray& data);
    bool write(const QJsonObject& json);

private:
    QSslSocket* socket;
    qint64 userId;
    QString sessionToken;
    QDateTime lastActivity;
    QByteArray readBuffer;

signals:
    void disconnected();
    void dataReceived();
    void error(const QString& errorMessage);

private:
    Q_DISABLE_COPY(ChatClientConnection)
};

#endif // CHATCLIENTCONNECTION_H