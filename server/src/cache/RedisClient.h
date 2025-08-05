#ifndef REDISCLIENT_H
#define REDISCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QMutex>
#include <QVariant>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(redisClient)

/**
 * @brief 简单的Redis客户端实现
 * 
 * 提供基本的Redis操作功能，用于缓存系统
 */
class RedisClient : public QObject
{
    Q_OBJECT

public:
    explicit RedisClient(QObject* parent = nullptr);
    ~RedisClient();

    // 连接管理
    bool connectToServer(const QString& host, int port, const QString& password = QString(), int database = 0);
    void disconnectFromServer();
    bool isConnected() const;

    // 基本操作
    bool set(const QString& key, const QVariant& value, int ttlSeconds = -1);
    QVariant get(const QString& key, const QVariant& defaultValue = QVariant());
    bool remove(const QString& key);
    bool exists(const QString& key);
    
    // 批量操作
    bool setMultiple(const QVariantMap& keyValues, int ttlSeconds = -1);
    QVariantMap getMultiple(const QStringList& keys);
    
    // 过期时间管理
    bool expire(const QString& key, int seconds);
    int ttl(const QString& key);
    
    // 数据库操作
    bool selectDatabase(int database);
    bool flushDatabase();
    
    // 连接信息
    QString getLastError() const;

signals:
    void connected();
    void disconnected();
    void error(const QString& errorString);

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketReadyRead();

private:
    // 内部方法
    bool sendCommand(const QStringList& command);
    QString readResponse();
    bool waitForResponse(int timeoutMs = 5000);
    QString escapeValue(const QVariant& value);
    QVariant parseValue(const QString& value);
    
    // 成员变量
    QTcpSocket* m_socket;
    QString m_host;
    int m_port;
    QString m_password;
    int m_database;
    bool m_connected;
    QString m_lastError;
    QByteArray m_responseBuffer;
    mutable QMutex m_mutex;
};

#endif // REDISCLIENT_H
