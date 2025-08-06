#include "NonBlockingConnectionManager.h"
#include "../utils/LogManager.h"
#include "../utils/StackTraceLogger.h"
#include <QDir>
#include <QDateTime>
#include <QApplication>
#include <QThread>

Q_LOGGING_CATEGORY(nonBlockingConnectionManager, "qkchat.server.nonblocking.connection")

NonBlockingConnectionManager::NonBlockingConnectionManager(QObject *parent)
    : QObject(parent)
    , m_monitorTimer(new QTimer(this))
    , m_connectionTimeout(5000)  // 5秒超时
    , m_maxRetries(3)
    , m_retryInterval(1000)      // 1秒重试间隔
    , m_monitoringActive(false)
{
    connect(m_monitorTimer, &QTimer::timeout, this, &NonBlockingConnectionManager::onConnectionCheck);
}

NonBlockingConnectionManager::~NonBlockingConnectionManager()
{
    stopMonitoring();
    disconnectAll();
}

NonBlockingConnectionManager& NonBlockingConnectionManager::instance()
{
    static NonBlockingConnectionManager instance;
    return instance;
}

void NonBlockingConnectionManager::addConnection(QSslSocket* socket, const QString& identifier, bool isCritical)
{
    QMutexLocker locker(&m_mutex);
    
    if (!socket) {
        qCWarning(nonBlockingConnectionManager) << "Attempted to add null socket";
        return;
    }
    
    ConnectionInfo info;
    info.socket = socket;
    info.state = Disconnected;
    info.lastActivity = QDateTime::currentDateTime();
    info.connectionStartTime = QDateTime::currentDateTime();
    info.retryCount = 0;
    info.lastError = "";
    info.isCritical = isCritical;
    
    m_connections[identifier] = info;
    
    setupSocketSignals(socket, identifier);
    
    qCInfo(nonBlockingConnectionManager) << "Added connection:" << identifier
                             << "Critical:" << isCritical;
}

void NonBlockingConnectionManager::removeConnection(const QString& identifier)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_connections.contains(identifier)) {
        ConnectionInfo info = m_connections[identifier];
        if (info.socket) {
            info.socket->disconnect(this);
            if (info.socket->state() != QAbstractSocket::UnconnectedState) {
                info.socket->disconnectFromHost();
            }
        }
        m_connections.remove(identifier);

        emit connectionRemoved(identifier);
        
        qCInfo(nonBlockingConnectionManager) << "Removed connection:" << identifier;
    }
}

void NonBlockingConnectionManager::setConnectionTimeout(int timeoutMs)
{
    QMutexLocker locker(&m_mutex);
    m_connectionTimeout = timeoutMs;
}

void NonBlockingConnectionManager::setMaxRetries(int maxRetries)
{
    QMutexLocker locker(&m_mutex);
    m_maxRetries = maxRetries;
}

void NonBlockingConnectionManager::setRetryInterval(int intervalMs)
{
    QMutexLocker locker(&m_mutex);
    m_retryInterval = intervalMs;
}

bool NonBlockingConnectionManager::isConnectionActive(const QString& identifier) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_connections.contains(identifier)) {
        return false;
    }
    
    const ConnectionInfo& info = m_connections[identifier];
    return info.socket && info.socket->state() == QAbstractSocket::ConnectedState;
}

NonBlockingConnectionManager::ConnectionState NonBlockingConnectionManager::getConnectionState(const QString& identifier) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_connections.contains(identifier)) {
        return Disconnected;
    }
    
    return m_connections[identifier].state;
}

QString NonBlockingConnectionManager::getConnectionError(const QString& identifier) const
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_connections.contains(identifier)) {
        return QString();
    }
    
    return m_connections[identifier].lastError;
}

void NonBlockingConnectionManager::startMonitoring()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_monitoringActive) {
        return;
    }
    
    m_monitorTimer->start(1000); // 每秒检查一次
    m_monitoringActive = true;
    
    qCInfo(nonBlockingConnectionManager) << "Connection monitoring started";
}

void NonBlockingConnectionManager::stopMonitoring()
{
    QMutexLocker locker(&m_mutex);
    
    m_monitorTimer->stop();
    m_monitoringActive = false;
    
    qCInfo(nonBlockingConnectionManager) << "Connection monitoring stopped";
}

void NonBlockingConnectionManager::forceDisconnect(const QString& identifier)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_connections.contains(identifier)) {
        ConnectionInfo info = m_connections[identifier];
        if (info.socket) {
            info.socket->disconnectFromHost();
            info.socket->close();
        }
        m_connections.remove(identifier);
        
        qCInfo(nonBlockingConnectionManager) << "Forcefully disconnected:" << identifier;
    }
}

void NonBlockingConnectionManager::disconnectAll()
{
    QMutexLocker locker(&m_mutex);
    
    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        ConnectionInfo& info = it.value();
        if (info.socket) {
            info.socket->disconnectFromHost();
            info.socket->close();
        }
    }
    
    m_connections.clear();
    
    qCInfo(nonBlockingConnectionManager) << "All connections disconnected";
}

void NonBlockingConnectionManager::reconnectAllCritical()
{
    QMutexLocker locker(&m_mutex);
    
    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        ConnectionInfo& info = it.value();
        if (info.isCritical && info.socket && info.socket->state() != QAbstractSocket::ConnectedState) {
            info.socket->connectToHost(info.socket->peerName(), info.socket->peerPort());
        }
    }
}

void NonBlockingConnectionManager::onConnectionCheck()
{
    QMutexLocker locker(&m_mutex);
    
    QDateTime now = QDateTime::currentDateTime();
    QStringList connectionsToRemove;
    
    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        const QString& identifier = it.key();
        ConnectionInfo& info = it.value();
        
        // 检查连接超时
        if (info.lastActivity.msecsTo(now) > m_connectionTimeout) {
            info.state = Timeout;
            info.lastError = "Connection timeout";
            
            qCWarning(nonBlockingConnectionManager) << "Connection timeout:" << identifier
                                 << "Last activity:" << info.lastActivity.toString();
            
            emit connectionTimeout(identifier);
            
            if (!info.isCritical) {
                connectionsToRemove.append(identifier);
            }
        }
        
        // 检查空闲连接（30秒无活动）
        if (info.lastActivity.msecsTo(now) > 30000) {
            qCInfo(nonBlockingConnectionManager) << "Connection idle for 30s:" << identifier;
        }
        
        // 移除失败的连接
        if (info.socket && info.socket->state() == QAbstractSocket::UnconnectedState && !info.isCritical) {
            connectionsToRemove.append(identifier);
        }
    }
    
    // 移除标记的连接
    for (const QString& identifier : connectionsToRemove) {
        m_connections.remove(identifier);
        qCInfo(nonBlockingConnectionManager) << "Removed failed connection:" << identifier;
    }
}

void NonBlockingConnectionManager::onSocketDisconnected()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    
    QString identifier;
    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        if (it.value().socket == socket) {
            identifier = it.key();
            break;
        }
    }
    
    if (!identifier.isEmpty()) {
        ConnectionInfo& info = m_connections[identifier];
        info.state = Disconnected;
        info.lastError = "Socket disconnected";
        
        qCWarning(nonBlockingConnectionManager) << "Connection lost:" << identifier
                             << "Error:" << socket->errorString();
        
        emit connectionLost(identifier, socket->errorString());
        
        // 记录堆栈追踪
        StackTraceLogger::instance().logStackTrace(
            QString("CONNECTION_LOST_%1").arg(identifier), 
            "NonBlockingConnectionManager::onSocketDisconnected"
        );
        
        // 如果是关键连接，尝试重连
        if (info.isCritical && info.retryCount < m_maxRetries) {
            handleConnectionRetry(identifier);
        }
    }
}

void NonBlockingConnectionManager::onSocketError(QAbstractSocket::SocketError error)
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    
    QString identifier;
    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        if (it.value().socket == socket) {
            identifier = it.key();
            break;
        }
    }
    
    if (!identifier.isEmpty()) {
        ConnectionInfo& info = m_connections[identifier];
        info.state = Disconnected;
        info.lastError = socket->errorString();
        
        qCWarning(nonBlockingConnectionManager) << "Socket error for" << identifier
                             << "Error:" << socket->errorString();
        
        emit connectionFailed(identifier, socket->errorString());
        
        // 记录堆栈追踪
        StackTraceLogger::instance().logStackTrace(
            QString("SOCKET_ERROR_%1").arg(identifier), 
            "NonBlockingConnectionManager::onSocketError"
        );
    }
}

void NonBlockingConnectionManager::onSocketConnected()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    
    QString identifier;
    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        if (it.value().socket == socket) {
            identifier = it.key();
            break;
        }
    }
    
    if (!identifier.isEmpty()) {
        ConnectionInfo& info = m_connections[identifier];
        info.state = Connected;
        info.lastActivity = QDateTime::currentDateTime();
        info.lastError = "";
        info.retryCount = 0;
        
        qCInfo(nonBlockingConnectionManager) << "Connection established:" << identifier;
        
        emit connectionRestored(identifier);
    }
}

void NonBlockingConnectionManager::onSocketStateChanged(QAbstractSocket::SocketState state)
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    
    QString identifier;
    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        if (it.value().socket == socket) {
            identifier = it.key();
            break;
        }
    }
    
    if (!identifier.isEmpty()) {
        ConnectionInfo& info = m_connections[identifier];
        ConnectionState oldState = info.state;
        
        switch (state) {
            case QAbstractSocket::ConnectedState:
                info.state = Connected;
                info.lastActivity = QDateTime::currentDateTime();
                break;
            case QAbstractSocket::ConnectingState:
                info.state = Connecting;
                break;
            case QAbstractSocket::UnconnectedState:
                info.state = Disconnected;
                break;
            default:
                break;
        }
        
        if (oldState != info.state) {
            logConnectionEvent(identifier, 
                QString("State changed from %1 to %2").arg(oldState).arg(info.state));
        }
    }
}

void NonBlockingConnectionManager::setupSocketSignals(QSslSocket* socket, const QString& identifier)
{
    connect(socket, &QSslSocket::disconnected, this, &NonBlockingConnectionManager::onSocketDisconnected);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::errorOccurred), 
            this, &NonBlockingConnectionManager::onSocketError);
    connect(socket, &QSslSocket::connected, this, &NonBlockingConnectionManager::onSocketConnected);
    connect(socket, &QSslSocket::stateChanged, this, &NonBlockingConnectionManager::onSocketStateChanged);
    
    // 设置非阻塞模式
    socket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
}

void NonBlockingConnectionManager::handleConnectionTimeout(const QString& identifier)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_connections.contains(identifier)) {
        return;
    }
    
    ConnectionInfo& info = m_connections[identifier];
    info.state = Timeout;
    info.lastError = "Connection timeout";
    
    qCWarning(nonBlockingConnectionManager) << "Connection timeout:" << identifier;
    
    emit connectionTimeout(identifier);
    
    // 记录堆栈追踪
    StackTraceLogger::instance().logStackTrace(
        QString("CONNECTION_TIMEOUT_%1").arg(identifier), 
        "NonBlockingConnectionManager::handleConnectionTimeout"
    );
    
    // 尝试重连（如果是关键连接）
    if (info.isCritical && info.retryCount < m_maxRetries) {
        handleConnectionRetry(identifier);
    }
}

void NonBlockingConnectionManager::handleConnectionRetry(const QString& identifier)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_connections.contains(identifier)) {
        return;
    }
    
    ConnectionInfo& info = m_connections[identifier];
    
    if (info.retryCount >= m_maxRetries) {
        qCWarning(nonBlockingConnectionManager) << "Max retries reached for:" << identifier;
        return;
    }
    
    info.retryCount++;
    info.state = Reconnecting;
    info.connectionStartTime = QDateTime::currentDateTime();
    info.lastError = "Reconnecting...";
    
    qCInfo(nonBlockingConnectionManager) << "Retrying connection:" << identifier
                             << "Attempt:" << info.retryCount << "/" << m_maxRetries;
    
    if (info.socket) {
        info.socket->connectToHost(info.socket->peerName(), info.socket->peerPort());
    }
    
    // 记录重试日志
    StackTraceLogger::instance().logStackTrace(
        QString("CONNECTION_RETRY_%1_%2").arg(identifier).arg(info.retryCount), 
        "NonBlockingConnectionManager::handleConnectionRetry"
    );
}

void NonBlockingConnectionManager::logConnectionEvent(const QString& identifier, const QString& event)
{
    QString logFile = QString("D:/QT_Learn/Projects/QKChatApp/logs/server/connection_%1.log")
                     .arg(QDate::currentDate().toString("yyyyMMdd"));
    
    QDir().mkpath(QFileInfo(logFile).absolutePath());
    
    QFile file(logFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
               << " [" << identifier << "] " << event << Qt::endl;
        file.close();
    }
}
