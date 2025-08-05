#include "RedisClient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QThread>

Q_LOGGING_CATEGORY(redisClient, "qkchat.server.redisclient")

RedisClient::RedisClient(QObject* parent)
    : QObject(parent)
    , m_socket(nullptr)
    , m_port(6379)
    , m_database(0)
    , m_connected(false)
{
    m_socket = new QTcpSocket(this);
    
    connect(m_socket, &QTcpSocket::connected, this, &RedisClient::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &RedisClient::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &RedisClient::onSocketError);
    connect(m_socket, &QTcpSocket::readyRead, this, &RedisClient::onSocketReadyRead);
}

RedisClient::~RedisClient()
{
    disconnectFromServer();
}

bool RedisClient::connectToServer(const QString& host, int port, const QString& password, int database)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_connected) {
        return true;
    }
    
    m_host = host;
    m_port = port;
    m_password = password;
    m_database = database;
    
    qCInfo(redisClient) << "Connecting to Redis server:" << host << ":" << port;
    
    m_socket->connectToHost(host, port);
    
    if (!m_socket->waitForConnected(5000)) {
        m_lastError = "Connection timeout: " + m_socket->errorString();
        qCWarning(redisClient) << "Failed to connect to Redis:" << m_lastError;
        return false;
    }
    
    // 认证（如果需要）
    if (!password.isEmpty()) {
        if (!sendCommand({"AUTH", password})) {
            qCWarning(redisClient) << "Redis authentication failed";
            return false;
        }
    }
    
    // 选择数据库
    if (database != 0) {
        if (!sendCommand({"SELECT", QString::number(database)})) {
            qCWarning(redisClient) << "Failed to select Redis database:" << database;
            return false;
        }
    }
    
    m_connected = true;
    qCInfo(redisClient) << "Successfully connected to Redis server";
    emit connected();
    
    return true;
}

void RedisClient::disconnectFromServer()
{
    QMutexLocker locker(&m_mutex);
    
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
        if (m_socket->state() != QAbstractSocket::UnconnectedState) {
            m_socket->waitForDisconnected(3000);
        }
    }
    
    m_connected = false;
}

bool RedisClient::isConnected() const
{
    QMutexLocker locker(&m_mutex);
    return m_connected && m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

bool RedisClient::set(const QString& key, const QVariant& value, int ttlSeconds)
{
    QMutexLocker locker(&m_mutex);
    
    if (!isConnected()) {
        m_lastError = "Not connected to Redis server";
        return false;
    }
    
    QString valueStr = escapeValue(value);
    
    QStringList command;
    if (ttlSeconds > 0) {
        command = {"SETEX", key, QString::number(ttlSeconds), valueStr};
    } else {
        command = {"SET", key, valueStr};
    }
    
    bool success = sendCommand(command);
    if (success) {
        qCDebug(redisClient) << "Successfully set key:" << key;
    } else {
        qCWarning(redisClient) << "Failed to set key:" << key << "Error:" << m_lastError;
    }
    
    return success;
}

QVariant RedisClient::get(const QString& key, const QVariant& defaultValue)
{
    QMutexLocker locker(&m_mutex);
    
    if (!isConnected()) {
        m_lastError = "Not connected to Redis server";
        return defaultValue;
    }
    
    if (!sendCommand({"GET", key})) {
        return defaultValue;
    }
    
    QString response = readResponse();
    if (response.isEmpty() || response == "$-1") {
        return defaultValue;
    }
    
    // 解析Redis响应格式
    if (response.startsWith("$")) {
        // 批量字符串响应
        int length = response.mid(1).split("\r\n")[0].toInt();
        if (length == -1) {
            return defaultValue;
        }
        
        QString value = response.split("\r\n")[1];
        return parseValue(value);
    }
    
    return parseValue(response);
}

bool RedisClient::remove(const QString& key)
{
    QMutexLocker locker(&m_mutex);
    
    if (!isConnected()) {
        m_lastError = "Not connected to Redis server";
        return false;
    }
    
    return sendCommand({"DEL", key});
}

bool RedisClient::exists(const QString& key)
{
    QMutexLocker locker(&m_mutex);
    
    if (!isConnected()) {
        m_lastError = "Not connected to Redis server";
        return false;
    }
    
    if (!sendCommand({"EXISTS", key})) {
        return false;
    }
    
    QString response = readResponse();
    return response.trimmed() == ":1";
}

QString RedisClient::getLastError() const
{
    QMutexLocker locker(&m_mutex);
    return m_lastError;
}

void RedisClient::onSocketConnected()
{
    qCDebug(redisClient) << "Socket connected to Redis server";
}

void RedisClient::onSocketDisconnected()
{
    qCDebug(redisClient) << "Socket disconnected from Redis server";
    m_connected = false;
    emit disconnected();
}

void RedisClient::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    m_lastError = m_socket->errorString();
    qCWarning(redisClient) << "Socket error:" << m_lastError;
    m_connected = false;
    emit this->error(m_lastError);
}

void RedisClient::onSocketReadyRead()
{
    m_responseBuffer += m_socket->readAll();
}

bool RedisClient::sendCommand(const QStringList& command)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        m_lastError = "Socket not connected";
        return false;
    }
    
    // 构建Redis协议命令
    QString redisCommand = QString("*%1\r\n").arg(command.size());
    for (const QString& arg : command) {
        redisCommand += QString("$%1\r\n%2\r\n").arg(arg.toUtf8().size()).arg(arg);
    }
    
    qCDebug(redisClient) << "Sending Redis command:" << command.join(" ");
    
    m_responseBuffer.clear();
    m_socket->write(redisCommand.toUtf8());
    m_socket->flush();
    
    return waitForResponse();
}

QString RedisClient::readResponse()
{
    return QString::fromUtf8(m_responseBuffer);
}

bool RedisClient::waitForResponse(int timeoutMs)
{
    if (!m_socket->waitForReadyRead(timeoutMs)) {
        m_lastError = "Response timeout";
        return false;
    }
    
    // 简单的响应检查
    QString response = QString::fromUtf8(m_responseBuffer);
    if (response.startsWith("-ERR")) {
        m_lastError = "Redis error: " + response.mid(5).trimmed();
        return false;
    }
    
    return true;
}

QString RedisClient::escapeValue(const QVariant& value)
{
    if (value.type() == QVariant::Map || value.type() == QVariant::List) {
        QJsonDocument doc = QJsonDocument::fromVariant(value);
        return doc.toJson(QJsonDocument::Compact);
    }
    
    return value.toString();
}

QVariant RedisClient::parseValue(const QString& value)
{
    // 尝试解析JSON
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8(), &error);
    if (error.error == QJsonParseError::NoError) {
        return doc.toVariant();
    }
    
    // 返回原始字符串
    return value;
}

bool RedisClient::flushDatabase()
{
    QMutexLocker locker(&m_mutex);
    
    if (!isConnected()) {
        m_lastError = "Not connected to Redis server";
        return false;
    }
    
    qCInfo(redisClient) << "Flushing Redis database";
    
    if (!sendCommand({"FLUSHDB"})) {
        qCWarning(redisClient) << "Failed to flush Redis database:" << m_lastError;
        return false;
    }
    
    QString response = readResponse();
    bool success = response.trimmed() == "+OK";
    
    if (success) {
        qCInfo(redisClient) << "Successfully flushed Redis database";
    } else {
        qCWarning(redisClient) << "Failed to flush Redis database, response:" << response;
    }
    
    return success;
}

bool RedisClient::setMultiple(const QVariantMap& keyValues, int ttlSeconds)
{
    QMutexLocker locker(&m_mutex);
    
    if (!isConnected()) {
        m_lastError = "Not connected to Redis server";
        return false;
    }
    
    qCDebug(redisClient) << "Setting multiple keys:" << keyValues.keys();
    
    // 使用Redis的MSET命令
    QStringList command = {"MSET"};
    for (auto it = keyValues.begin(); it != keyValues.end(); ++it) {
        command << it.key() << escapeValue(it.value());
    }
    
    if (!sendCommand(command)) {
        qCWarning(redisClient) << "Failed to set multiple keys:" << m_lastError;
        return false;
    }
    
    // 如果设置了TTL，为所有键设置过期时间
    if (ttlSeconds > 0) {
        for (auto it = keyValues.begin(); it != keyValues.end(); ++it) {
            expire(it.key(), ttlSeconds);
        }
    }
    
    return true;
}

QVariantMap RedisClient::getMultiple(const QStringList& keys)
{
    QMutexLocker locker(&m_mutex);
    
    if (!isConnected()) {
        m_lastError = "Not connected to Redis server";
        return QVariantMap();
    }
    
    if (keys.isEmpty()) {
        return QVariantMap();
    }
    
    qCDebug(redisClient) << "Getting multiple keys:" << keys;
    
    // 使用Redis的MGET命令
    QStringList command = {"MGET"};
    command << keys;
    
    if (!sendCommand(command)) {
        qCWarning(redisClient) << "Failed to get multiple keys:" << m_lastError;
        return QVariantMap();
    }
    
    QString response = readResponse();
    // 解析响应并构建结果映射
    QVariantMap result;
    // 这里需要根据实际的Redis响应格式进行解析
    // 简化实现，实际使用时需要更复杂的解析逻辑
    
    return result;
}

bool RedisClient::expire(const QString& key, int seconds)
{
    QMutexLocker locker(&m_mutex);
    
    if (!isConnected()) {
        m_lastError = "Not connected to Redis server";
        return false;
    }
    
    if (!sendCommand({"EXPIRE", key, QString::number(seconds)})) {
        qCWarning(redisClient) << "Failed to set expire for key:" << key << m_lastError;
        return false;
    }
    
    QString response = readResponse();
    return response.trimmed() == ":1";
}

int RedisClient::ttl(const QString& key)
{
    QMutexLocker locker(&m_mutex);
    
    if (!isConnected()) {
        m_lastError = "Not connected to Redis server";
        return -2; // 键不存在
    }
    
    if (!sendCommand({"TTL", key})) {
        qCWarning(redisClient) << "Failed to get TTL for key:" << key << m_lastError;
        return -1; // 错误
    }
    
    QString response = readResponse();
    bool ok;
    int ttl = response.trimmed().toInt(&ok);
    return ok ? ttl : -1;
}

bool RedisClient::selectDatabase(int database)
{
    QMutexLocker locker(&m_mutex);
    
    if (!isConnected()) {
        m_lastError = "Not connected to Redis server";
        return false;
    }
    
    if (!sendCommand({"SELECT", QString::number(database)})) {
        qCWarning(redisClient) << "Failed to select database:" << database << m_lastError;
        return false;
    }
    
    QString response = readResponse();
    bool success = response.trimmed() == "+OK";
    
    if (success) {
        m_database = database;
        qCInfo(redisClient) << "Successfully selected database:" << database;
    }
    
    return success;
}
