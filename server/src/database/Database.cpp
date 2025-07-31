#include "Database.h"
#include "../config/ServerConfig.h"
#include <QSqlDriver>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QVariantMap>
#include <QUuid>
#include <QMutexLocker>
#include <QLoggingCategory>
#include <QThread>

Q_LOGGING_CATEGORY(database, "qkchat.server.database")

Database::Database(QObject *parent)
    : QObject(parent)
    , _isConnected(false)
    , _port(3306)
    , _connectTimeout(10)
    , _readTimeout(30)
{
    _connectionName = getConnectionName();
}

Database::~Database()
{
    close();
}

bool Database::initialize()
{
    QMutexLocker locker(&_mutex);
    
    if (_isConnected) {
        return true;
    }
    
    // 从配置中获取数据库连接参数
    ServerConfig *config = ServerConfig::instance();
    _host = config->getDatabaseHost();
    _port = config->getDatabasePort();
    _databaseName = config->getDatabaseName();
    _username = config->getDatabaseUsername();
    _password = config->getDatabasePassword();
    
    // 创建数据库连接
    _database = QSqlDatabase::addDatabase("QMYSQL", _connectionName);
    _database.setHostName(_host);
    _database.setPort(_port);
    _database.setDatabaseName(_databaseName);
    _database.setUserName(_username);
    _database.setPassword(_password);
    
    // 设置连接选项
    _database.setConnectOptions(QString("MYSQL_OPT_CONNECT_TIMEOUT=%1;MYSQL_OPT_READ_TIMEOUT=%2")
                               .arg(_connectTimeout).arg(_readTimeout));
    
    if (!_database.open()) {
        QString error = QString("Failed to connect to database: %1").arg(_database.lastError().text());
        qCCritical(database) << error;
        emit databaseError(error);
        return false;
    }
    
    _isConnected = true;
    
    // 设置字符集
    QSqlQuery query(_database);
    query.exec("SET NAMES utf8mb4");
    query.exec("SET CHARACTER SET utf8mb4");
    
    qCInfo(database) << "Database connected successfully to" << _host << ":" << _port;
    emit connectionRestored();
    
    return true;
}

bool Database::isConnected() const
{
    QMutexLocker locker(&_mutex);
    return _isConnected && _database.isOpen();
}

void Database::close()
{
    QMutexLocker locker(&_mutex);
    
    if (_database.isOpen()) {
        _database.close();
    }
    
    QSqlDatabase::removeDatabase(_connectionName);
    _isConnected = false;
    
    qCInfo(database) << "Database connection closed";
}

bool Database::reconnect()
{
    close();
    return initialize();
}

bool Database::createUser(const QString &username, const QString &email, const QString &passwordHash, const QString &avatarUrl)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("INSERT INTO users (username, email, password_hash, avatar_url, display_name, created_at) "
                  "VALUES (?, ?, ?, ?, ?, NOW())");
    query.addBindValue(username);
    query.addBindValue(email);
    query.addBindValue(passwordHash);
    query.addBindValue(avatarUrl.isEmpty() ? QVariant() : avatarUrl);
    query.addBindValue(username); // 默认显示名为用户名
    
    return executeQuery(query);
}

Database::UserInfo Database::getUserById(qint64 userId)
{
    QMutexLocker locker(&_mutex);
    UserInfo userInfo = {};
    
    if (!isConnected() && !initialize()) {
        return userInfo;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT id, username, email, password_hash, avatar_url, display_name, "
                  "status, last_online, created_at, updated_at FROM users WHERE id = ?");
    query.addBindValue(userId);
    
    if (executeQuery(query) && query.next()) {
        userInfo.id = query.value(0).toLongLong();
        userInfo.username = query.value(1).toString();
        userInfo.email = query.value(2).toString();
        userInfo.passwordHash = query.value(3).toString();
        userInfo.avatarUrl = query.value(4).toString();
        userInfo.displayName = query.value(5).toString();
        userInfo.status = query.value(6).toString();
        userInfo.lastOnline = query.value(7).toDateTime();
        userInfo.createdAt = query.value(8).toDateTime();
        userInfo.updatedAt = query.value(9).toDateTime();
    }
    
    return userInfo;
}

Database::UserInfo Database::getUserByUsername(const QString &username)
{
    QMutexLocker locker(&_mutex);
    UserInfo userInfo = {};
    
    if (!isConnected() && !initialize()) {
        return userInfo;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT id, username, email, password_hash, avatar_url, display_name, "
                  "status, last_online, created_at, updated_at FROM users WHERE username = ?");
    query.addBindValue(username);
    
    if (executeQuery(query) && query.next()) {
        userInfo.id = query.value(0).toLongLong();
        userInfo.username = query.value(1).toString();
        userInfo.email = query.value(2).toString();
        userInfo.passwordHash = query.value(3).toString();
        userInfo.avatarUrl = query.value(4).toString();
        userInfo.displayName = query.value(5).toString();
        userInfo.status = query.value(6).toString();
        userInfo.lastOnline = query.value(7).toDateTime();
        userInfo.createdAt = query.value(8).toDateTime();
        userInfo.updatedAt = query.value(9).toDateTime();
    }
    
    return userInfo;
}

Database::UserInfo Database::getUserByEmail(const QString &email)
{
    QMutexLocker locker(&_mutex);
    UserInfo userInfo = {};
    
    if (!isConnected() && !initialize()) {
        return userInfo;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT id, username, email, password_hash, avatar_url, display_name, "
                  "status, last_online, created_at, updated_at FROM users WHERE email = ?");
    query.addBindValue(email);
    
    if (executeQuery(query) && query.next()) {
        userInfo.id = query.value(0).toLongLong();
        userInfo.username = query.value(1).toString();
        userInfo.email = query.value(2).toString();
        userInfo.passwordHash = query.value(3).toString();
        userInfo.avatarUrl = query.value(4).toString();
        userInfo.displayName = query.value(5).toString();
        userInfo.status = query.value(6).toString();
        userInfo.lastOnline = query.value(7).toDateTime();
        userInfo.createdAt = query.value(8).toDateTime();
        userInfo.updatedAt = query.value(9).toDateTime();
    }
    
    return userInfo;
}

bool Database::updateUser(qint64 userId, const QVariantMap &data)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    if (data.isEmpty()) {
        return false;
    }
    
    QStringList setParts;
    QVariantList values;
    
    for (auto it = data.begin(); it != data.end(); ++it) {
        setParts << QString("%1 = ?").arg(it.key());
        values << it.value();
    }
    
    values << userId; // WHERE条件的值
    
    QString sql = QString("UPDATE users SET %1, updated_at = NOW() WHERE id = ?")
                  .arg(setParts.join(", "));
    
    QSqlQuery query(_database);
    query.prepare(sql);
    
    for (const QVariant &value : values) {
        query.addBindValue(value);
    }
    
    return executeQuery(query);
}

bool Database::deleteUser(qint64 userId)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("DELETE FROM users WHERE id = ?");
    query.addBindValue(userId);
    
    return executeQuery(query);
}

bool Database::isUsernameAvailable(const QString &username)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT COUNT(*) FROM users WHERE username = ?");
    query.addBindValue(username);
    
    if (executeQuery(query) && query.next()) {
        return query.value(0).toInt() == 0;
    }
    
    return false;
}

bool Database::isEmailAvailable(const QString &email)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT COUNT(*) FROM users WHERE email = ?");
    query.addBindValue(email);
    
    if (executeQuery(query) && query.next()) {
        return query.value(0).toInt() == 0;
    }
    
    return false;
}

QList<Database::UserInfo> Database::getActiveUsers(int limit)
{
    QMutexLocker locker(&_mutex);
    QList<UserInfo> users;
    
    if (!isConnected() && !initialize()) {
        return users;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT id, username, email, password_hash, avatar_url, display_name, "
                  "status, last_online, created_at, updated_at FROM users "
                  "WHERE status = 'active' ORDER BY last_online DESC LIMIT ?");
    query.addBindValue(limit);
    
    if (executeQuery(query)) {
        while (query.next()) {
            UserInfo userInfo;
            userInfo.id = query.value(0).toLongLong();
            userInfo.username = query.value(1).toString();
            userInfo.email = query.value(2).toString();
            userInfo.passwordHash = query.value(3).toString();
            userInfo.avatarUrl = query.value(4).toString();
            userInfo.displayName = query.value(5).toString();
            userInfo.status = query.value(6).toString();
            userInfo.lastOnline = query.value(7).toDateTime();
            userInfo.createdAt = query.value(8).toDateTime();
            userInfo.updatedAt = query.value(9).toDateTime();
            users.append(userInfo);
        }
    }
    
    return users;
}

bool Database::updateUserLastOnline(qint64 userId, const QDateTime &lastOnline)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("UPDATE users SET last_online = ? WHERE id = ?");
    query.addBindValue(lastOnline);
    query.addBindValue(userId);
    
    return executeQuery(query);
}

Database::UserInfo Database::authenticateUser(const QString &usernameOrEmail, const QString &passwordHash)
{
    QMutexLocker locker(&_mutex);
    UserInfo userInfo = {};
    
    if (!isConnected() && !initialize()) {
        return userInfo;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT id, username, email, password_hash, avatar_url, display_name, "
                  "status, last_online, created_at, updated_at FROM users "
                  "WHERE (username = ? OR email = ?) AND password_hash = ? AND status = 'active'");
    query.addBindValue(usernameOrEmail);
    query.addBindValue(usernameOrEmail);
    query.addBindValue(passwordHash);
    
    if (executeQuery(query) && query.next()) {
        userInfo.id = query.value(0).toLongLong();
        userInfo.username = query.value(1).toString();
        userInfo.email = query.value(2).toString();
        userInfo.passwordHash = query.value(3).toString();
        userInfo.avatarUrl = query.value(4).toString();
        userInfo.displayName = query.value(5).toString();
        userInfo.status = query.value(6).toString();
        userInfo.lastOnline = query.value(7).toDateTime();
        userInfo.createdAt = query.value(8).toDateTime();
        userInfo.updatedAt = query.value(9).toDateTime();
    }
    
    return userInfo;
}

QString Database::createUserSession(qint64 userId, const QString &deviceInfo, const QString &ipAddress, int expirationHours)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return QString();
    }
    
    QString sessionToken = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QDateTime expiresAt = QDateTime::currentDateTime().addSecs(expirationHours * 3600);
    
    QSqlQuery query(_database);
    query.prepare("INSERT INTO user_sessions (user_id, session_token, device_info, ip_address, expires_at, created_at) "
                  "VALUES (?, ?, ?, ?, ?, NOW())");
    query.addBindValue(userId);
    query.addBindValue(sessionToken);
    query.addBindValue(deviceInfo);
    query.addBindValue(ipAddress);
    query.addBindValue(expiresAt);
    
    if (executeQuery(query)) {
        return sessionToken;
    }
    
    return QString();
}

Database::SessionInfo Database::getSessionByToken(const QString &sessionToken)
{
    QMutexLocker locker(&_mutex);
    SessionInfo sessionInfo = {};
    
    if (!isConnected() && !initialize()) {
        return sessionInfo;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT id, user_id, session_token, device_info, ip_address, "
                  "expires_at, created_at, last_activity FROM user_sessions "
                  "WHERE session_token = ? AND expires_at > NOW()");
    query.addBindValue(sessionToken);
    
    if (executeQuery(query) && query.next()) {
        sessionInfo.id = query.value(0).toLongLong();
        sessionInfo.userId = query.value(1).toLongLong();
        sessionInfo.sessionToken = query.value(2).toString();
        sessionInfo.deviceInfo = query.value(3).toString();
        sessionInfo.ipAddress = query.value(4).toString();
        sessionInfo.expiresAt = query.value(5).toDateTime();
        sessionInfo.createdAt = query.value(6).toDateTime();
        sessionInfo.lastActivity = query.value(7).toDateTime();
    }
    
    return sessionInfo;
}

bool Database::updateSessionActivity(const QString &sessionToken)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("UPDATE user_sessions SET last_activity = NOW() WHERE session_token = ?");
    query.addBindValue(sessionToken);
    
    return executeQuery(query);
}

bool Database::deleteSession(const QString &sessionToken)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("DELETE FROM user_sessions WHERE session_token = ?");
    query.addBindValue(sessionToken);
    
    return executeQuery(query);
}

bool Database::deleteUserSessions(qint64 userId)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("DELETE FROM user_sessions WHERE user_id = ?");
    query.addBindValue(userId);
    
    return executeQuery(query);
}

void Database::cleanExpiredSessions()
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return;
    }
    
    QSqlQuery query(_database);
    query.prepare("DELETE FROM user_sessions WHERE expires_at < NOW()");
    
    if (executeQuery(query)) {
        int deletedCount = query.numRowsAffected();
        if (deletedCount > 0) {
            qCInfo(database) << "Cleaned" << deletedCount << "expired sessions";
        }
    }
}

bool Database::saveMessage(const QString &messageId, qint64 senderId, qint64 receiverId, 
                          const QString &messageType, const QString &content, 
                          const QString &fileUrl, qint64 fileSize)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("INSERT INTO messages (message_id, sender_id, receiver_id, message_type, "
                  "content, file_url, file_size, created_at) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, NOW())");
    query.addBindValue(messageId);
    query.addBindValue(senderId);
    query.addBindValue(receiverId);
    query.addBindValue(messageType);
    query.addBindValue(content);
    query.addBindValue(fileUrl.isEmpty() ? QVariant() : fileUrl);
    query.addBindValue(fileSize > 0 ? fileSize : QVariant());
    
    return executeQuery(query);
}

Database::MessageInfo Database::getMessageById(const QString &messageId)
{
    QMutexLocker locker(&_mutex);
    MessageInfo messageInfo = {};
    
    if (!isConnected() && !initialize()) {
        return messageInfo;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT id, message_id, sender_id, receiver_id, message_type, content, "
                  "file_url, file_size, delivery_status, created_at, delivered_at, read_at "
                  "FROM messages WHERE message_id = ?");
    query.addBindValue(messageId);
    
    if (executeQuery(query) && query.next()) {
        messageInfo.id = query.value(0).toLongLong();
        messageInfo.messageId = query.value(1).toString();
        messageInfo.senderId = query.value(2).toLongLong();
        messageInfo.receiverId = query.value(3).toLongLong();
        messageInfo.messageType = query.value(4).toString();
        messageInfo.content = query.value(5).toString();
        messageInfo.fileUrl = query.value(6).toString();
        messageInfo.fileSize = query.value(7).toLongLong();
        messageInfo.deliveryStatus = query.value(8).toString();
        messageInfo.createdAt = query.value(9).toDateTime();
        messageInfo.deliveredAt = query.value(10).toDateTime();
        messageInfo.readAt = query.value(11).toDateTime();
    }
    
    return messageInfo;
}

QList<Database::MessageInfo> Database::getMessagesBetweenUsers(qint64 userId1, qint64 userId2, int limit, int offset)
{
    QMutexLocker locker(&_mutex);
    QList<MessageInfo> messages;
    
    if (!isConnected() && !initialize()) {
        return messages;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT id, message_id, sender_id, receiver_id, message_type, content, "
                  "file_url, file_size, delivery_status, created_at, delivered_at, read_at "
                  "FROM messages WHERE (sender_id = ? AND receiver_id = ?) OR (sender_id = ? AND receiver_id = ?) "
                  "ORDER BY created_at DESC LIMIT ? OFFSET ?");
    query.addBindValue(userId1);
    query.addBindValue(userId2);
    query.addBindValue(userId2);
    query.addBindValue(userId1);
    query.addBindValue(limit);
    query.addBindValue(offset);
    
    if (executeQuery(query)) {
        while (query.next()) {
            MessageInfo messageInfo;
            messageInfo.id = query.value(0).toLongLong();
            messageInfo.messageId = query.value(1).toString();
            messageInfo.senderId = query.value(2).toLongLong();
            messageInfo.receiverId = query.value(3).toLongLong();
            messageInfo.messageType = query.value(4).toString();
            messageInfo.content = query.value(5).toString();
            messageInfo.fileUrl = query.value(6).toString();
            messageInfo.fileSize = query.value(7).toLongLong();
            messageInfo.deliveryStatus = query.value(8).toString();
            messageInfo.createdAt = query.value(9).toDateTime();
            messageInfo.deliveredAt = query.value(10).toDateTime();
            messageInfo.readAt = query.value(11).toDateTime();
            messages.append(messageInfo);
        }
    }
    
    return messages;
}

bool Database::updateMessageStatus(const QString &messageId, const QString &status)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QString sql = "UPDATE messages SET delivery_status = ?";
    QVariantList values;
    values << status;
    
    if (status == "delivered") {
        sql += ", delivered_at = NOW()";
    } else if (status == "read") {
        sql += ", read_at = NOW()";
    }
    
    sql += " WHERE message_id = ?";
    values << messageId;
    
    QSqlQuery query(_database);
    query.prepare(sql);
    
    for (const QVariant &value : values) {
        query.addBindValue(value);
    }
    
    return executeQuery(query);
}

QList<Database::MessageInfo> Database::getRecentMessages(int limit)
{
    QMutexLocker locker(&_mutex);
    QList<MessageInfo> messages;
    
    if (!isConnected() && !initialize()) {
        return messages;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT id, message_id, sender_id, receiver_id, message_type, content, "
                  "file_url, file_size, delivery_status, created_at, delivered_at, read_at "
                  "FROM messages ORDER BY created_at DESC LIMIT ?");
    query.addBindValue(limit);
    
    if (executeQuery(query)) {
        while (query.next()) {
            MessageInfo messageInfo;
            messageInfo.id = query.value(0).toLongLong();
            messageInfo.messageId = query.value(1).toString();
            messageInfo.senderId = query.value(2).toLongLong();
            messageInfo.receiverId = query.value(3).toLongLong();
            messageInfo.messageType = query.value(4).toString();
            messageInfo.content = query.value(5).toString();
            messageInfo.fileUrl = query.value(6).toString();
            messageInfo.fileSize = query.value(7).toLongLong();
            messageInfo.deliveryStatus = query.value(8).toString();
            messageInfo.createdAt = query.value(9).toDateTime();
            messageInfo.deliveredAt = query.value(10).toDateTime();
            messageInfo.readAt = query.value(11).toDateTime();
            messages.append(messageInfo);
        }
    }
    
    return messages;
}

bool Database::logEvent(LogLevel level, const QString &module, const QString &message, 
                       qint64 userId, const QString &ipAddress, 
                       const QString &userAgent, const QVariantMap &extraData)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QStringList levelNames = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
    QString levelName = levelNames.value(level, "INFO");
    
    QJsonDocument extraDoc;
    if (!extraData.isEmpty()) {
        extraDoc = QJsonDocument::fromVariant(extraData);
    }
    
    QSqlQuery query(_database);
    query.prepare("INSERT INTO system_logs (log_level, module, message, user_id, ip_address, "
                  "user_agent, extra_data, created_at) VALUES (?, ?, ?, ?, ?, ?, ?, NOW())");
    query.addBindValue(levelName);
    query.addBindValue(module);
    query.addBindValue(message);
    query.addBindValue(userId > 0 ? userId : QVariant());
    query.addBindValue(ipAddress.isEmpty() ? QVariant() : ipAddress);
    query.addBindValue(userAgent.isEmpty() ? QVariant() : userAgent);
    query.addBindValue(extraDoc.isEmpty() ? QVariant() : extraDoc.toJson(QJsonDocument::Compact));
    
    return executeQuery(query);
}

QList<QVariantMap> Database::getSystemLogs(LogLevel minLevel, int limit, int offset)
{
    QMutexLocker locker(&_mutex);
    QList<QVariantMap> logs;
    
    if (!isConnected() && !initialize()) {
        return logs;
    }
    
    QStringList levelNames = {"DEBUG", "INFO", "WARNING", "ERROR", "CRITICAL"};
    QStringList validLevels;
    
    for (int i = minLevel; i < levelNames.size(); ++i) {
        validLevels << QString("'%1'").arg(levelNames[i]);
    }
    
    QString sql = QString("SELECT log_level, module, message, user_id, ip_address, "
                         "user_agent, extra_data, created_at FROM system_logs "
                         "WHERE log_level IN (%1) ORDER BY created_at DESC LIMIT ? OFFSET ?")
                  .arg(validLevels.join(","));
    
    QSqlQuery query(_database);
    query.prepare(sql);
    query.addBindValue(limit);
    query.addBindValue(offset);
    
    if (executeQuery(query)) {
        while (query.next()) {
            QVariantMap log;
            log["log_level"] = query.value(0).toString();
            log["module"] = query.value(1).toString();
            log["message"] = query.value(2).toString();
            log["user_id"] = query.value(3).toLongLong();
            log["ip_address"] = query.value(4).toString();
            log["user_agent"] = query.value(5).toString();
            
            QString extraDataJson = query.value(6).toString();
            if (!extraDataJson.isEmpty()) {
                QJsonDocument doc = QJsonDocument::fromJson(extraDataJson.toUtf8());
                log["extra_data"] = doc.toVariant();
            }
            
            log["created_at"] = query.value(7).toDateTime();
            logs.append(log);
        }
    }
    
    return logs;
}

bool Database::executeQuery(QSqlQuery &query)
{
    if (!query.exec()) {
        QString error = QString("SQL execution failed: %1 - %2")
                       .arg(query.lastError().text())
                       .arg(query.lastQuery());
        qCWarning(database) << error;
        emit databaseError(error);
        
        // 如果是连接错误，尝试重连
        if (query.lastError().type() == QSqlError::ConnectionError) {
            _isConnected = false;
            emit connectionLost();
        }
        
        return false;
    }
    
    return true;
}

QString Database::getConnectionName() const
{
    return QString("Database_%1_%2")
           .arg(reinterpret_cast<quintptr>(this))
           .arg(reinterpret_cast<quintptr>(QThread::currentThread()));
}

int Database::getTotalUserCount() const
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected()) {
        // 在const方法中不能调用非const方法，所以我们需要特殊处理
        Database* nonConstThis = const_cast<Database*>(this);
        if (!nonConstThis->initialize()) {
            return 0;
        }
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT COUNT(*) FROM users");
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt();
    }
    
    return 0;
} 