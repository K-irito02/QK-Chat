#ifndef NONBLOCKINGCONNECTIONMANAGER_H
#define NONBLOCKINGCONNECTIONMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QSslSocket>
#include <QTimer>
#include <QMutex>
#include <QHash>
#include <QList>
#include <functional>

/**
 * @brief 非阻塞连接管理器
 * 
 * 管理所有网络连接，提供超时检测、自动重连、连接监控等功能
 * 防止UI线程被网络操作阻塞
 */
class NonBlockingConnectionManager : public QObject
{
    Q_OBJECT

public:
    enum ConnectionState {
        Disconnected,
        Connecting,
        Connected,
        Reconnecting,
        Timeout
    };

    struct ConnectionInfo {
        QSslSocket* socket;
        ConnectionState state;
        QDateTime lastActivity;
        QDateTime connectionStartTime;
        int retryCount;
        QString lastError;
        bool isCritical;
    };

    explicit NonBlockingConnectionManager(QObject *parent = nullptr);
    ~NonBlockingConnectionManager();

    static NonBlockingConnectionManager& instance();

    // 连接管理
    void addConnection(QSslSocket* socket, const QString& identifier, bool isCritical = false);
    void removeConnection(const QString& identifier);
    void setConnectionTimeout(int timeoutMs);
    void setMaxRetries(int maxRetries);
    void setRetryInterval(int intervalMs);

    // 连接状态检查
    bool isConnectionActive(const QString& identifier) const;
    ConnectionState getConnectionState(const QString& identifier) const;
    QString getConnectionError(const QString& identifier) const;

    // 连接监控
    void startMonitoring();
    void stopMonitoring();
    void forceDisconnect(const QString& identifier);

    // 批量操作
    void disconnectAll();
    void reconnectAllCritical();

signals:
    void connectionTimeout(const QString& identifier);
    void connectionLost(const QString& identifier, const QString& error);
    void connectionRestored(const QString& identifier);
    void connectionFailed(const QString& identifier, const QString& error);

private slots:
    void onConnectionCheck();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onSocketConnected();
    void onSocketStateChanged(QAbstractSocket::SocketState state);

private:
    void setupSocketSignals(QSslSocket* socket, const QString& identifier);
    void handleConnectionTimeout(const QString& identifier);
    void handleConnectionRetry(const QString& identifier);
    void logConnectionEvent(const QString& identifier, const QString& event);

    mutable QMutex m_mutex;
    QHash<QString, ConnectionInfo> m_connections;
    QTimer* m_monitorTimer;
    
    int m_connectionTimeout;
    int m_maxRetries;
    int m_retryInterval;
    bool m_monitoringActive;
};

#endif // NONBLOCKINGCONNECTIONMANAGER_H