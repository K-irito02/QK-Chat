#include "Database.h"
#include "../config/ServerConfig.h"
#include "../utils/LogManager.h"
#include "../utils/StackTraceLogger.h"
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
#include <QDateTime>
#include <QCryptographicHash>

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
        LogManager::instance()->writeDatabaseLog("INIT_SKIP", "Database already connected", "Database");
        return true;
    }

    LogManager::instance()->writeDatabaseLog("INIT_START", "Starting database initialization", "Database");

    // 检查可用的数据库驱动
    QStringList availableDrivers = QSqlDatabase::drivers();
    LogManager::instance()->writeDatabaseLog("DRIVER_CHECK",
                                           QString("Available drivers: %1").arg(availableDrivers.join(", ")),
                                           "Database");

    if (!availableDrivers.contains("QMYSQL")) {
        QString error = "QMYSQL driver not available. Available drivers: " + availableDrivers.join(", ");
        LogManager::instance()->writeErrorLog(error, "Database");
        emit databaseError(error);
        return false;
    }

    // 从配置中获取数据库连接参数
    ServerConfig *config = ServerConfig::instance();
    _host = config->getDatabaseHost();
    _port = config->getDatabasePort();
    _databaseName = config->getDatabaseName();
    _username = config->getDatabaseUsername();
    _password = config->getDatabasePassword();

    LogManager::instance()->writeDatabaseLog("CONNECTION_ATTEMPT",
                                           QString("Host: %1:%2, Database: %3, User: %4")
                                           .arg(_host).arg(_port).arg(_databaseName).arg(_username),
                                           "Database");
    
    // 创建数据库连接
    _database = QSqlDatabase::addDatabase("QMYSQL", _connectionName);
    _database.setHostName(_host);
    _database.setPort(_port);
    _database.setDatabaseName(_databaseName);
    _database.setUserName(_username);
    _database.setPassword(_password);
    
    // 设置连接选项 - 减少超时时间防止阻塞
    _database.setConnectOptions(QString("MYSQL_OPT_CONNECT_TIMEOUT=3;MYSQL_OPT_READ_TIMEOUT=5"));
    
    // 使用非阻塞连接方式，设置超时
    if (!_database.open()) {
        QString error = QString("Failed to connect to database: %1").arg(_database.lastError().text());
        LogManager::instance()->writeErrorLog(error, "Database");
        LogManager::instance()->writeDatabaseLog("CONNECTION_FAILED", error, "Database");
        emit databaseError(error);
        
        // 记录堆栈追踪日志
        StackTraceLogger::instance().logStackTrace("DATABASE_CONNECTION_FAILED", "Database::initialize");
        
        return false;
    }

    _isConnected = true;

    // 设置字符集
    QSqlQuery query(_database);
    query.exec("SET NAMES utf8mb4");
    query.exec("SET CHARACTER SET utf8mb4");

    LogManager::instance()->writeDatabaseLog("CONNECTION_SUCCESS",
                                           QString("Connected to %1:%2").arg(_host).arg(_port),
                                           "Database");
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

QSqlDatabase Database::getDatabase() const
{
    return _database;
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

    // 生成盐值和哈希密码
    QString salt = QUuid::createUuid().toString().remove('{').remove('}').left(64);
    QString hashedPassword = QString(QCryptographicHash::hash((passwordHash + salt).toUtf8(), QCryptographicHash::Sha256).toHex());

    QSqlQuery query(_database);
    query.prepare("INSERT INTO users (username, email, password_hash, salt, avatar_url, display_name, bio, status, email_verified) "
                  "VALUES (?, ?, ?, ?, ?, ?, '', 'inactive', FALSE)");
    query.addBindValue(username);
    query.addBindValue(email);
    query.addBindValue(hashedPassword);
    query.addBindValue(salt);
    query.addBindValue(avatarUrl.isEmpty() ? QVariant() : avatarUrl);
    query.addBindValue(username); // 默认显示名为用户名

    return executeQuery(query);
}

// 邮箱验证相关方法
bool Database::createEmailVerification(qint64 userId, const QString &email, const QString &token, const QString &tokenType, int expiryHours)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("INSERT INTO email_verifications (user_id, email, verification_token, token_type, expires_at) "
                  "VALUES (?, ?, ?, ?, DATE_ADD(NOW(), INTERVAL ? HOUR))");
    query.addBindValue(userId);
    query.addBindValue(email);
    query.addBindValue(token);
    query.addBindValue(tokenType);
    query.addBindValue(expiryHours);
    
    return executeQuery(query);
}

bool Database::verifyEmailToken(const QString &token, QString *email)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT user_id, email FROM email_verifications "
                  "WHERE verification_token = ? AND expires_at > NOW() AND used = FALSE");
    query.addBindValue(token);
    
    if (executeQuery(query) && query.next()) {
        if (email) {
            *email = query.value("email").toString();
        }
        return true;
    }
    
    return false;
}

bool Database::verifyEmailCode(const QString &email, const QString &code)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT user_id FROM email_verification_codes "
                  "WHERE email = ? AND verification_code = ? AND expires_at > NOW() AND used = FALSE");
    query.addBindValue(email);
    query.addBindValue(code);
    
    if (executeQuery(query) && query.next()) {
        // 标记验证码为已使用
        QSqlQuery updateQuery(_database);
        updateQuery.prepare("UPDATE email_verification_codes SET used = TRUE, used_at = NOW() "
                           "WHERE email = ? AND verification_code = ?");
        updateQuery.addBindValue(email);
        updateQuery.addBindValue(code);
        executeQuery(updateQuery);
        
        return true;
    }
    
    return false;
}

bool Database::saveEmailVerificationCode(const QString &email, const QString &code)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    // 先删除该邮箱的旧验证码
    QSqlQuery deleteQuery(_database);
    deleteQuery.prepare("DELETE FROM email_verification_codes WHERE email = ?");
    deleteQuery.addBindValue(email);
    executeQuery(deleteQuery);
    
    // 插入新的验证码
    QSqlQuery query(_database);
    query.prepare("INSERT INTO email_verification_codes (email, verification_code, expires_at, created_at) "
                  "VALUES (?, ?, DATE_ADD(NOW(), INTERVAL 10 MINUTE), NOW())");
    query.addBindValue(email);
    query.addBindValue(code);
    
    return executeQuery(query);
}

bool Database::markEmailVerificationUsed(const QString &token)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("UPDATE email_verifications SET used = TRUE, used_at = NOW() "
                  "WHERE verification_token = ?");
    query.addBindValue(token);
    
    return executeQuery(query);
}

bool Database::isEmailVerificationValid(const QString &token)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT COUNT(*) FROM email_verifications "
                  "WHERE verification_token = ? AND expires_at > NOW() AND used = FALSE");
    query.addBindValue(token);
    
    if (executeQuery(query) && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

bool Database::updateUserEmailVerification(qint64 userId, bool verified)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("UPDATE users SET email_verified = ?, status = ? WHERE id = ?");
    query.addBindValue(verified);
    query.addBindValue(verified ? "active" : "unverified");
    query.addBindValue(userId);
    
    return executeQuery(query);
}

bool Database::resendEmailVerification(qint64 userId, const QString &email, const QString &token)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    // 使旧的验证令牌失效
    QSqlQuery query(_database);
    query.prepare("UPDATE email_verifications SET used = TRUE WHERE user_id = ? AND token_type = 'register'");
    query.addBindValue(userId);
    if (!executeQuery(query)) {
        return false;
    }
    
    // 创建新的验证令牌
    return createEmailVerification(userId, email, token, "register");
}

QString Database::getEmailVerificationToken(qint64 userId, const QString &tokenType)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return QString();
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT verification_token FROM email_verifications "
                  "WHERE user_id = ? AND token_type = ? AND expires_at > NOW() AND used = FALSE "
                  "ORDER BY created_at DESC LIMIT 1");
    query.addBindValue(userId);
    query.addBindValue(tokenType);
    
    if (executeQuery(query) && query.next()) {
        return query.value(0).toString();
    }
    
    return QString();
}

bool Database::cleanupExpiredVerifications()
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("DELETE FROM email_verifications WHERE expires_at < NOW()");
    
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
    query.prepare("SELECT id, username, email, password_hash, salt, avatar_url, display_name, bio, "
                  "status, last_online, created_at, updated_at FROM users WHERE id = ?");
    query.addBindValue(userId);

    if (executeQuery(query) && query.next()) {
        userInfo.id = query.value(0).toLongLong();
        userInfo.username = query.value(1).toString();
        userInfo.email = query.value(2).toString();
        userInfo.passwordHash = query.value(3).toString();
        userInfo.salt = query.value(4).toString();
        userInfo.avatarUrl = query.value(5).toString();
        userInfo.displayName = query.value(6).toString();
        userInfo.bio = query.value(7).toString();
        userInfo.status = query.value(8).toString();
        userInfo.lastOnline = query.value(9).toDateTime();
        userInfo.createdAt = query.value(10).toDateTime();
        userInfo.updatedAt = query.value(11).toDateTime();
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
    query.prepare("SELECT id, username, email, password_hash, salt, avatar_url, display_name, bio, "
                  "status, last_online, created_at, updated_at FROM users WHERE username = ?");
    query.addBindValue(username);

    if (executeQuery(query) && query.next()) {
        userInfo.id = query.value(0).toLongLong();
        userInfo.username = query.value(1).toString();
        userInfo.email = query.value(2).toString();
        userInfo.passwordHash = query.value(3).toString();
        userInfo.salt = query.value(4).toString();
        userInfo.avatarUrl = query.value(5).toString();
        userInfo.displayName = query.value(6).toString();
        userInfo.bio = query.value(7).toString();
        userInfo.status = query.value(8).toString();
        userInfo.lastOnline = query.value(9).toDateTime();
        userInfo.createdAt = query.value(10).toDateTime();
        userInfo.updatedAt = query.value(11).toDateTime();
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
    query.prepare("SELECT id, username, email, password_hash, salt, avatar_url, display_name, bio, "
                  "status, last_online, created_at, updated_at FROM users WHERE email = ?");
    query.addBindValue(email);

    if (executeQuery(query) && query.next()) {
        userInfo.id = query.value(0).toLongLong();
        userInfo.username = query.value(1).toString();
        userInfo.email = query.value(2).toString();
        userInfo.passwordHash = query.value(3).toString();
        userInfo.salt = query.value(4).toString();
        userInfo.avatarUrl = query.value(5).toString();
        userInfo.displayName = query.value(6).toString();
        userInfo.bio = query.value(7).toString();
        userInfo.status = query.value(8).toString();
        userInfo.lastOnline = query.value(9).toDateTime();
        userInfo.createdAt = query.value(10).toDateTime();
        userInfo.updatedAt = query.value(11).toDateTime();
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
    query.prepare("SELECT id, username, email, password_hash, salt, avatar_url, display_name, bio, "
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
            userInfo.salt = query.value(4).toString();
            userInfo.avatarUrl = query.value(5).toString();
            userInfo.displayName = query.value(6).toString();
            userInfo.bio = query.value(7).toString();
            userInfo.status = query.value(8).toString();
            userInfo.lastOnline = query.value(9).toDateTime();
            userInfo.createdAt = query.value(10).toDateTime();
            userInfo.updatedAt = query.value(11).toDateTime();
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

Database::UserInfo Database::authenticateUser(const QString &usernameOrEmail, const QString &password)
{
    QMutexLocker locker(&_mutex);
    UserInfo userInfo = {};

    if (!isConnected() && !initialize()) {
        return userInfo;
    }

    QSqlQuery query(_database);
    query.prepare("SELECT id, username, email, password_hash, salt, avatar_url, display_name, bio, "
                  "status, last_online, created_at, updated_at FROM users "
                  "WHERE (username = ? OR email = ?) AND status = 'active'");
    query.addBindValue(usernameOrEmail);
    query.addBindValue(usernameOrEmail);

    if (executeQuery(query) && query.next()) {
        QString salt = query.value(4).toString();
        QString dbPasswordHash = query.value(3).toString();
        
        // Use the same hashing algorithm as in createUser
        QByteArray passwordWithSalt = salt.toUtf8() + password.toUtf8();
        QString providedPasswordHash = QString(QCryptographicHash::hash(passwordWithSalt, QCryptographicHash::Sha256).toHex());

        if (dbPasswordHash == providedPasswordHash) {
            userInfo.id = query.value(0).toLongLong();
            userInfo.username = query.value(1).toString();
            userInfo.email = query.value(2).toString();
            userInfo.passwordHash = dbPasswordHash; // Store the hash from DB
            userInfo.salt = salt;
            userInfo.avatarUrl = query.value(5).toString();
            userInfo.displayName = query.value(6).toString();
            userInfo.bio = query.value(7).toString();
            userInfo.status = query.value(8).toString();
            userInfo.lastOnline = query.value(9).toDateTime();
            userInfo.createdAt = query.value(10).toDateTime();
            userInfo.updatedAt = query.value(11).toDateTime();
        }
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
                  "content, file_url, file_size, created_at, updated_at) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, NOW(), NOW())");
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
                  "file_url, file_size, delivery_status, created_at, updated_at "
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
        messageInfo.updatedAt = query.value(10).toDateTime();
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
                  "file_url, file_size, delivery_status, created_at, updated_at "
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
            messageInfo.updatedAt = query.value(10).toDateTime();
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
    
    QString sql = "UPDATE messages SET delivery_status = ?, updated_at = NOW() WHERE message_id = ?";
    QVariantList values;
    values << status;
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
                  "file_url, file_size, delivery_status, created_at, updated_at "
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
            messageInfo.updatedAt = query.value(10).toDateTime();
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
        LogManager::instance()->writeErrorLog(error, "Database");
        LogManager::instance()->writeDatabaseLog("QUERY_FAILED",
                                                QString("Error: %1, Query: %2").arg(query.lastError().text(), query.lastQuery()),
                                                "Database");
        emit databaseError(error);

        // 如果是连接错误，尝试重连
        if (query.lastError().type() == QSqlError::ConnectionError) {
            _isConnected = false;
            LogManager::instance()->writeDatabaseLog("CONNECTION_LOST", "Database connection lost", "Database");
            emit connectionLost();
        }

        return false;
    }

    LogManager::instance()->writeDatabaseLog("QUERY_SUCCESS",
                                           QString("Query executed: %1").arg(query.lastQuery()),
                                           "Database");
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
            LogManager::instance()->writeErrorLog("Failed to initialize database in getTotalUserCount", "Database");
            return 0;
        }
    }

    QSqlQuery query(_database);
    query.prepare("SELECT COUNT(*) FROM users");

    // 使用executeQuery来确保错误处理
    Database* nonConstThis = const_cast<Database*>(this);
    if (nonConstThis->executeQuery(query) && query.next()) {
        int count = query.value(0).toInt();
        LogManager::instance()->writeDatabaseLog("GET_TOTAL_USER_COUNT", QString("Total users: %1").arg(count), "Database");
        return count;
    }

    LogManager::instance()->writeErrorLog("Failed to get total user count", "Database");
    return 0;
}



int Database::getOnlineUserCount() const
{
    QMutexLocker locker(&_mutex);

    if (!isConnected()) {
        Database* nonConstThis = const_cast<Database*>(this);
        if (!nonConstThis->initialize()) {
            LogManager::instance()->writeErrorLog("Failed to initialize database in getOnlineUserCount", "Database");
            return 0;
        }
    }

    QSqlQuery query(_database);
    query.prepare("SELECT COUNT(DISTINCT user_id) FROM user_sessions WHERE expires_at > NOW()");

    Database* nonConstThis = const_cast<Database*>(this);
    if (nonConstThis->executeQuery(query) && query.next()) {
        int count = query.value(0).toInt();
        LogManager::instance()->writeDatabaseLog("GET_ONLINE_USER_COUNT", QString("Online users: %1").arg(count), "Database");
        return count;
    }

    LogManager::instance()->writeErrorLog("Failed to get online user count", "Database");
    return 0;
}

qint64 Database::getTotalMessageCount() const
{
    QMutexLocker locker(&_mutex);

    if (!isConnected()) {
        Database* nonConstThis = const_cast<Database*>(this);
        if (!nonConstThis->initialize()) {
            LogManager::instance()->writeErrorLog("Failed to initialize database in getTotalMessageCount", "Database");
            return 0;
        }
    }

    QSqlQuery query(_database);
    query.prepare("SELECT COUNT(*) FROM messages");

    Database* nonConstThis = const_cast<Database*>(this);
    if (nonConstThis->executeQuery(query) && query.next()) {
        qint64 count = query.value(0).toLongLong();
        LogManager::instance()->writeDatabaseLog("GET_TOTAL_MESSAGE_COUNT", QString("Total messages: %1").arg(count), "Database");
        return count;
    }

    LogManager::instance()->writeErrorLog("Failed to get total message count", "Database");
    return 0;
}

// 好友关系管理
bool Database::addFriendship(qint64 userId1, qint64 userId2, const QString &remark)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    // 检查是否已经是好友
    QSqlQuery checkQuery(_database);
    checkQuery.prepare("SELECT COUNT(*) FROM friendships WHERE "
                      "(user_id = ? AND friend_id = ?) OR (user_id = ? AND friend_id = ?)");
    checkQuery.addBindValue(userId1);
    checkQuery.addBindValue(userId2);
    checkQuery.addBindValue(userId2);
    checkQuery.addBindValue(userId1);
    
    if (executeQuery(checkQuery) && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        return false; // 已经是好友
    }
    
    // 添加双向好友关系
    QSqlQuery query1(_database);
    query1.prepare("INSERT INTO friendships (user_id, friend_id, remark, status, created_at) "
                   "VALUES (?, ?, ?, 'accepted', NOW())");
    query1.addBindValue(userId1);
    query1.addBindValue(userId2);
    query1.addBindValue(remark);
    
    QSqlQuery query2(_database);
    query2.prepare("INSERT INTO friendships (user_id, friend_id, remark, status, created_at) "
                   "VALUES (?, ?, ?, 'accepted', NOW())");
    query2.addBindValue(userId2);
    query2.addBindValue(userId1);
    query2.addBindValue(QString()); // 对方没有备注
    
    return executeQuery(query1) && executeQuery(query2);
}



QList<Database::FriendInfo> Database::getUserFriends(qint64 userId)
{
    QMutexLocker locker(&_mutex);
    QList<FriendInfo> friends;
    
    if (!isConnected() && !initialize()) {
        return friends;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT u.id, u.username, u.display_name, u.avatar_url, u.status, "
                  "u.last_online, f.remark, f.created_at FROM users u "
                  "JOIN friendships f ON u.id = f.friend_id "
                  "WHERE f.user_id = ? AND f.status = 'accepted' "
                  "ORDER BY u.display_name");
    query.addBindValue(userId);
    
    if (executeQuery(query)) {
        while (query.next()) {
            FriendInfo friendInfo;
            friendInfo.userId = query.value(0).toLongLong();
            friendInfo.username = query.value(1).toString();
            friendInfo.displayName = query.value(2).toString();
            friendInfo.avatarUrl = query.value(3).toString();
            friendInfo.status = query.value(4).toString();
            friendInfo.lastOnline = query.value(5).toDateTime();
            friendInfo.remark = query.value(6).toString();
            friendInfo.createdAt = query.value(7).toDateTime();
            friends.append(friendInfo);
        }
    }
    
    return friends;
}



// 群组管理
qint64 Database::createGroup(const QString &groupName, const QString &description, 
                            qint64 creatorId, const QString &avatarUrl, 
                            bool isPublic, bool isEncrypted)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return -1;
    }
    
    // 开始事务
    _database.transaction();
    
    // 创建群组
    QSqlQuery groupQuery(_database);
    groupQuery.prepare("INSERT INTO groups (name, description, creator_id, avatar_url, is_public, is_encrypted, created_at) "
                       "VALUES (?, ?, ?, ?, ?, ?, NOW())");
    groupQuery.addBindValue(groupName);
    groupQuery.addBindValue(description);
    groupQuery.addBindValue(creatorId);
    groupQuery.addBindValue(avatarUrl.isEmpty() ? QVariant() : avatarUrl);
    groupQuery.addBindValue(isPublic);
    groupQuery.addBindValue(isEncrypted);
    
    if (!executeQuery(groupQuery)) {
        _database.rollback();
        return -1;
    }
    
    // 获取群组ID
    qint64 groupId = groupQuery.lastInsertId().toLongLong();
    
    // 添加创建者为群组成员和管理员
    QSqlQuery memberQuery(_database);
    memberQuery.prepare("INSERT INTO group_members (group_id, user_id, role, joined_at) "
                        "VALUES (?, ?, 'owner', NOW())");
    memberQuery.addBindValue(groupId);
    memberQuery.addBindValue(creatorId);
    
    if (!executeQuery(memberQuery)) {
        _database.rollback();
        return -1;
    }
    
    _database.commit();
    return groupId;
}

bool Database::deleteGroup(qint64 groupId)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    // 开始事务
    _database.transaction();
    
    // 删除群组成员
    QSqlQuery memberQuery(_database);
    memberQuery.prepare("DELETE FROM group_members WHERE group_id = ?");
    memberQuery.addBindValue(groupId);
    
    if (!executeQuery(memberQuery)) {
        _database.rollback();
        return false;
    }
    
    // 删除群组消息
    QSqlQuery messageQuery(_database);
    messageQuery.prepare("DELETE FROM group_messages WHERE group_id = ?");
    messageQuery.addBindValue(groupId);
    
    if (!executeQuery(messageQuery)) {
        _database.rollback();
        return false;
    }
    
    // 删除群组
    QSqlQuery groupQuery(_database);
    groupQuery.prepare("DELETE FROM groups WHERE id = ?");
    groupQuery.addBindValue(groupId);
    
    if (!executeQuery(groupQuery)) {
        _database.rollback();
        return false;
    }
    
    _database.commit();
    return true;
}

Database::GroupInfo Database::getGroupById(qint64 groupId)
{
    QMutexLocker locker(&_mutex);
    GroupInfo groupInfo = {};
    
    if (!isConnected() && !initialize()) {
        return groupInfo;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT id, name, description, creator_id, avatar_url, member_count, "
                  "is_public, is_encrypted, created_at, updated_at FROM groups WHERE id = ?");
    query.addBindValue(groupId);
    
    if (executeQuery(query) && query.next()) {
        groupInfo.id = query.value(0).toLongLong();
        groupInfo.name = query.value(1).toString();
        groupInfo.description = query.value(2).toString();
        groupInfo.creatorId = query.value(3).toLongLong();
        groupInfo.avatarUrl = query.value(4).toString();
        groupInfo.memberCount = query.value(5).toInt();
        groupInfo.is_public = query.value(6).toBool();
        groupInfo.is_encrypted = query.value(7).toBool();
        groupInfo.createdAt = query.value(8).toDateTime();
        groupInfo.updatedAt = query.value(9).toDateTime();
    }
    
    return groupInfo;
}

QList<Database::GroupInfo> Database::getUserGroups(qint64 userId)
{
    QMutexLocker locker(&_mutex);
    QList<GroupInfo> groups;
    
    if (!isConnected() && !initialize()) {
        return groups;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT g.id, g.name, g.description, g.creator_id, g.avatar_url, "
                  "g.member_count, g.is_public, g.is_encrypted, g.created_at, g.updated_at FROM groups g "
                  "JOIN group_members gm ON g.id = gm.group_id "
                  "WHERE gm.user_id = ? ORDER BY g.name");
    query.addBindValue(userId);
    
    if (executeQuery(query)) {
        while (query.next()) {
            GroupInfo groupInfo;
            groupInfo.id = query.value(0).toLongLong();
            groupInfo.name = query.value(1).toString();
            groupInfo.description = query.value(2).toString();
            groupInfo.creatorId = query.value(3).toLongLong();
            groupInfo.avatarUrl = query.value(4).toString();
            groupInfo.memberCount = query.value(5).toInt();
            groupInfo.is_public = query.value(6).toBool();
            groupInfo.is_encrypted = query.value(7).toBool();
            groupInfo.createdAt = query.value(8).toDateTime();
            groupInfo.updatedAt = query.value(9).toDateTime();
            groups.append(groupInfo);
        }
    }
    
    return groups;
}

bool Database::addGroupMember(qint64 groupId, qint64 userId, const QString &role)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    // 检查是否已经是成员
    QSqlQuery checkQuery(_database);
    checkQuery.prepare("SELECT COUNT(*) FROM group_members WHERE group_id = ? AND user_id = ?");
    checkQuery.addBindValue(groupId);
    checkQuery.addBindValue(userId);
    
    if (executeQuery(checkQuery) && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        return false; // 已经是成员
    }
    
    // 开始事务
    _database.transaction();
    
    // 添加成员
    QSqlQuery memberQuery(_database);
    memberQuery.prepare("INSERT INTO group_members (group_id, user_id, role, joined_at) "
                        "VALUES (?, ?, ?, NOW())");
    memberQuery.addBindValue(groupId);
    memberQuery.addBindValue(userId);
    memberQuery.addBindValue(role);
    
    if (!executeQuery(memberQuery)) {
        _database.rollback();
        return false;
    }
    
    // 更新群组成员数量
    QSqlQuery updateQuery(_database);
    updateQuery.prepare("UPDATE groups SET member_count = member_count + 1, updated_at = NOW() "
                        "WHERE id = ?");
    updateQuery.addBindValue(groupId);
    
    if (!executeQuery(updateQuery)) {
        _database.rollback();
        return false;
    }
    
    _database.commit();
    return true;
}

bool Database::removeGroupMember(qint64 groupId, qint64 userId)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    // 开始事务
    _database.transaction();
    
    // 删除成员
    QSqlQuery memberQuery(_database);
    memberQuery.prepare("DELETE FROM group_members WHERE group_id = ? AND user_id = ?");
    memberQuery.addBindValue(groupId);
    memberQuery.addBindValue(userId);
    
    if (!executeQuery(memberQuery)) {
        _database.rollback();
        return false;
    }
    
    // 更新群组成员数量
    QSqlQuery updateQuery(_database);
    updateQuery.prepare("UPDATE groups SET member_count = member_count - 1, updated_at = NOW() "
                        "WHERE id = ?");
    updateQuery.addBindValue(groupId);
    
    if (!executeQuery(updateQuery)) {
        _database.rollback();
        return false;
    }
    
    _database.commit();
    return true;
}

QList<Database::GroupMemberInfo> Database::getGroupMembers(qint64 groupId)
{
    QMutexLocker locker(&_mutex);
    QList<GroupMemberInfo> members;
    
    if (!isConnected() && !initialize()) {
        return members;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT u.id, u.username, u.display_name, u.avatar_url, u.status, "
                  "u.last_online, gm.role, gm.joined_at FROM users u "
                  "JOIN group_members gm ON u.id = gm.user_id "
                  "WHERE gm.group_id = ? ORDER BY gm.role DESC, u.display_name");
    query.addBindValue(groupId);
    
    if (executeQuery(query)) {
        while (query.next()) {
            GroupMemberInfo memberInfo;
            memberInfo.userId = query.value(0).toLongLong();
            memberInfo.username = query.value(1).toString();
            memberInfo.displayName = query.value(2).toString();
            memberInfo.avatarUrl = query.value(3).toString();
            memberInfo.status = query.value(4).toString();
            memberInfo.lastOnline = query.value(5).toDateTime();
            memberInfo.role = query.value(6).toString();
            memberInfo.joinedAt = query.value(7).toDateTime();
            memberInfo.isOnline = (memberInfo.status == "online");
            members.append(memberInfo);
        }
    }
    
    return members;
}

bool Database::updateGroupMemberRole(qint64 groupId, qint64 userId, const QString &role)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("UPDATE group_members SET role = ? WHERE group_id = ? AND user_id = ?");
    query.addBindValue(role);
    query.addBindValue(groupId);
    query.addBindValue(userId);
    
    return executeQuery(query);
}

bool Database::updateGroupInfo(qint64 groupId, const QVariantMap &info)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    if (info.isEmpty()) {
        return false;
    }
    
    QStringList setParts;
    QVariantList values;
    
    for (auto it = info.begin(); it != info.end(); ++it) {
        setParts << QString("%1 = ?").arg(it.key());
        values << it.value();
    }
    
    QString sql = QString("UPDATE groups SET %1, updated_at = NOW() WHERE id = ?")
                      .arg(setParts.join(", "));
    
    QSqlQuery query(_database);
    query.prepare(sql);
    
    for (const QVariant &value : values) {
        query.addBindValue(value);
    }
    query.addBindValue(groupId);
    
    return executeQuery(query);
}

// 群组消息
bool Database::saveGroupMessage(const QString &messageId, qint64 senderId, qint64 groupId,
                               const QString &messageType, const QString &content,
                               const QString &fileUrl, qint64 fileSize)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("INSERT INTO group_messages (message_id, sender_id, group_id, message_type, "
                  "content, file_url, file_size, created_at, updated_at) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, NOW(), NOW())");
    query.addBindValue(messageId);
    query.addBindValue(senderId);
    query.addBindValue(groupId);
    query.addBindValue(messageType);
    query.addBindValue(content);
    query.addBindValue(fileUrl.isEmpty() ? QVariant() : fileUrl);
    query.addBindValue(fileSize > 0 ? fileSize : QVariant());
    
    return executeQuery(query);
}

QList<Database::GroupMessageInfo> Database::getGroupMessages(qint64 groupId, int limit, int offset)
{
    QMutexLocker locker(&_mutex);
    QList<GroupMessageInfo> messages;
    
    if (!isConnected() && !initialize()) {
        return messages;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT gm.id, gm.message_id, gm.sender_id, gm.group_id, gm.message_type, "
                  "gm.content, gm.file_url, gm.file_size, gm.created_at, "
                  "u.username, u.display_name, u.avatar_url "
                  "FROM group_messages gm "
                  "JOIN users u ON gm.sender_id = u.id "
                  "WHERE gm.group_id = ? "
                  "ORDER BY gm.created_at DESC LIMIT ? OFFSET ?");
    query.addBindValue(groupId);
    query.addBindValue(limit);
    query.addBindValue(offset);
    
    if (executeQuery(query)) {
        while (query.next()) {
            GroupMessageInfo messageInfo;
            messageInfo.id = query.value(0).toLongLong();
            messageInfo.messageId = query.value(1).toString();
            messageInfo.senderId = query.value(2).toLongLong();
            messageInfo.groupId = query.value(3).toLongLong();
            messageInfo.messageType = query.value(4).toString();
            messageInfo.content = query.value(5).toString();
            messageInfo.fileUrl = query.value(6).toString();
            messageInfo.fileSize = query.value(7).toLongLong();
            messageInfo.createdAt = query.value(8).toDateTime();
            messageInfo.senderUsername = query.value(9).toString();
            messageInfo.senderDisplayName = query.value(10).toString();
            messageInfo.senderAvatarUrl = query.value(11).toString();
            messages.append(messageInfo);
        }
    }
    
    return messages;
}


bool Database::removeFriendship(qint64 userId1, qint64 userId2)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("DELETE FROM friendships WHERE "
                  "(user_id = ? AND friend_id = ?) OR (user_id = ? AND friend_id = ?)");
    query.addBindValue(userId1);
    query.addBindValue(userId2);
    query.addBindValue(userId2);
    query.addBindValue(userId1);
    
    return executeQuery(query);
}


bool Database::updateFriendRemark(qint64 userId, qint64 friendId, const QString &remark)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("UPDATE friendships SET remark = ? WHERE user_id = ? AND friend_id = ?");
    query.addBindValue(remark);
    query.addBindValue(userId);
    query.addBindValue(friendId);
    
    return executeQuery(query);
}

// 统计数据方法
bool Database::updateDailyStats(const ServerStats &stats)
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        return false;
    }
    
    QSqlQuery query(_database);
    query.prepare("INSERT INTO daily_stats (stat_date, online_users, new_registrations, "
                  "messages_sent, files_transferred, total_users, active_users, created_at) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, NOW()) "
                  "ON DUPLICATE KEY UPDATE "
                  "online_users = VALUES(online_users), "
                  "new_registrations = VALUES(new_registrations), "
                  "messages_sent = VALUES(messages_sent), "
                  "files_transferred = VALUES(files_transferred), "
                  "total_users = VALUES(total_users), "
                  "active_users = VALUES(active_users), "
                  "updated_at = NOW()");
    
    query.addBindValue(stats.statDate);
    query.addBindValue(stats.onlineUsers);
    query.addBindValue(stats.newRegistrations);
    query.addBindValue(stats.messagesSent);
    query.addBindValue(stats.filesTransferred);
    query.addBindValue(stats.totalUsers);
    query.addBindValue(stats.activeUsers);
    
    return executeQuery(query);
}

Database::ServerStats Database::getTodayStats()
{
    QMutexLocker locker(&_mutex);
    ServerStats stats = {};
    
    if (!isConnected() && !initialize()) {
        return stats;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT stat_date, online_users, new_registrations, messages_sent, "
                  "files_transferred, total_users, active_users, created_at, updated_at "
                  "FROM daily_stats WHERE stat_date = CURDATE()");
    
    if (executeQuery(query) && query.next()) {
        stats.statDate = query.value(0).toDate();
        stats.onlineUsers = query.value(1).toInt();
        stats.newRegistrations = query.value(2).toInt();
        stats.messagesSent = query.value(3).toInt();
        stats.filesTransferred = query.value(4).toInt();
        stats.totalUsers = query.value(5).toInt();
        stats.activeUsers = query.value(6).toInt();
        stats.createdAt = query.value(7).toDateTime();
        stats.updatedAt = query.value(8).toDateTime();
    }
    
    return stats;
}

QList<Database::ServerStats> Database::getStatsHistory(const QDate &startDate, const QDate &endDate)
{
    QMutexLocker locker(&_mutex);
    QList<ServerStats> statsHistory;
    
    if (!isConnected() && !initialize()) {
        return statsHistory;
    }
    
    QSqlQuery query(_database);
    query.prepare("SELECT stat_date, online_users, new_registrations, messages_sent, "
                  "files_transferred, total_users, active_users, created_at, updated_at "
                  "FROM daily_stats WHERE stat_date BETWEEN ? AND ? ORDER BY stat_date");
    query.addBindValue(startDate);
    query.addBindValue(endDate);
    
    if (executeQuery(query)) {
        while (query.next()) {
            ServerStats stats;
            stats.statDate = query.value(0).toDate();
            stats.onlineUsers = query.value(1).toInt();
            stats.newRegistrations = query.value(2).toInt();
            stats.messagesSent = query.value(3).toInt();
            stats.filesTransferred = query.value(4).toInt();
            stats.totalUsers = query.value(5).toInt();
            stats.activeUsers = query.value(6).toInt();
            stats.createdAt = query.value(7).toDateTime();
            stats.updatedAt = query.value(8).toDateTime();
            statsHistory.append(stats);
        }
    }
    
    return statsHistory;
}

bool Database::createTables()
{
    QMutexLocker locker(&_mutex);

    if (!isConnected() && !initialize()) {
        qCCritical(database) << "Cannot create tables: database not connected";
        return false;
    }

    QSqlQuery query(_database);

    if (!_database.transaction()) {
        qCCritical(database) << "Failed to start transaction for table creation";
        return false;
    }

    try {
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS users (
                id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                username VARCHAR(50) NOT NULL UNIQUE,
                email VARCHAR(100) NOT NULL UNIQUE,
                password_hash VARCHAR(255) NOT NULL,
                salt VARCHAR(64) NOT NULL,
                avatar_url VARCHAR(512) DEFAULT NULL,
                display_name VARCHAR(100) DEFAULT NULL,
                bio TEXT DEFAULT NULL,
                status VARCHAR(20) DEFAULT 'inactive',
                last_online TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
            ) ENGINE=InnoDB COMMENT='用户信息表'
        )")) {
            throw std::runtime_error("Failed to create users table: " + query.lastError().text().toStdString());
        }

        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS user_sessions (
                id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                user_id BIGINT UNSIGNED NOT NULL,
                session_token VARCHAR(255) NOT NULL UNIQUE,
                refresh_token VARCHAR(255) DEFAULT NULL,
                device_info VARCHAR(255) DEFAULT NULL,
                ip_address VARCHAR(45) DEFAULT NULL,
                expires_at TIMESTAMP NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
            ) ENGINE=InnoDB COMMENT='用户会话表'
        )")) {
            throw std::runtime_error("Failed to create user_sessions table: " + query.lastError().text().toStdString());
        }

        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS messages (
                id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                message_id VARCHAR(36) NOT NULL UNIQUE,
                sender_id BIGINT UNSIGNED NOT NULL,
                receiver_id BIGINT UNSIGNED NOT NULL,
                message_type VARCHAR(20) DEFAULT 'text',
                content TEXT NOT NULL,
                file_url VARCHAR(512) DEFAULT NULL,
                file_size BIGINT DEFAULT NULL,
                delivery_status VARCHAR(20) DEFAULT 'sent',
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE,
                FOREIGN KEY (receiver_id) REFERENCES users(id) ON DELETE CASCADE
            ) ENGINE=InnoDB COMMENT='消息表'
        )")) {
            throw std::runtime_error("Failed to create messages table: " + query.lastError().text().toStdString());
        }

        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS friendships (
                id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                user_id BIGINT UNSIGNED NOT NULL,
                friend_id BIGINT UNSIGNED NOT NULL,
                status VARCHAR(20) DEFAULT 'pending',
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                remark VARCHAR(255) DEFAULT NULL,
                FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                FOREIGN KEY (friend_id) REFERENCES users(id) ON DELETE CASCADE,
                UNIQUE KEY uk_friendship (user_id, friend_id)
            ) ENGINE=InnoDB COMMENT='好友关系表'
        )")) {
            throw std::runtime_error("Failed to create friendships table: " + query.lastError().text().toStdString());
        }

        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS chat_groups (
                id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                name VARCHAR(100) NOT NULL,
                description TEXT DEFAULT NULL,
                avatar_url VARCHAR(512) DEFAULT NULL,
                creator_id BIGINT UNSIGNED NOT NULL,
                member_count INT DEFAULT 1,
                is_public BOOLEAN DEFAULT TRUE,
                is_encrypted BOOLEAN DEFAULT FALSE,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                FOREIGN KEY (creator_id) REFERENCES users(id) ON DELETE CASCADE
            ) ENGINE=InnoDB COMMENT='群组信息表'
        )")) {
            throw std::runtime_error("Failed to create chat_groups table: " + query.lastError().text().toStdString());
        }
        
        // 群组成员表
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS group_members (
                id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                group_id BIGINT UNSIGNED NOT NULL,
                user_id BIGINT UNSIGNED NOT NULL,
                role VARCHAR(20) DEFAULT 'member',
                joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (group_id) REFERENCES chat_groups(id) ON DELETE CASCADE,
                FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
                UNIQUE KEY uk_group_member (group_id, user_id)
            ) ENGINE=InnoDB COMMENT='群组成员表'
        )")) {
            throw std::runtime_error("Failed to create group_members table: " + query.lastError().text().toStdString());
        }
        
        // 群组消息表
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS group_messages (
                id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                message_id VARCHAR(36) NOT NULL UNIQUE,
                group_id BIGINT UNSIGNED NOT NULL,
                sender_id BIGINT UNSIGNED NOT NULL,
                message_type VARCHAR(20) DEFAULT 'text',
                content TEXT NOT NULL,
                file_url VARCHAR(512) DEFAULT NULL,
                file_size BIGINT DEFAULT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
                FOREIGN KEY (group_id) REFERENCES chat_groups(id) ON DELETE CASCADE,
                FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE
            ) ENGINE=InnoDB COMMENT='群组消息表'
        )")) {
            throw std::runtime_error("Failed to create group_messages table: " + query.lastError().text().toStdString());
        }
        
        // 系统日志表
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS system_logs (
                id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                log_level VARCHAR(20) NOT NULL,
                module VARCHAR(50) NOT NULL,
                message TEXT NOT NULL,
                user_id BIGINT UNSIGNED DEFAULT NULL,
                ip_address VARCHAR(45) DEFAULT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE SET NULL
            ) ENGINE=InnoDB COMMENT='系统日志表'
        )")) {
            throw std::runtime_error("Failed to create system_logs table: " + query.lastError().text().toStdString());
        }
        
        // 服务器统计表
        if (!query.exec(R"(
            CREATE TABLE IF NOT EXISTS daily_stats (
                id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
                stat_date DATE NOT NULL UNIQUE,
                online_users INT DEFAULT 0,
                new_registrations INT DEFAULT 0,
                messages_sent INT DEFAULT 0,
                files_transferred INT DEFAULT 0,
                total_users INT DEFAULT 0,
                active_users INT DEFAULT 0,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
            ) ENGINE=InnoDB COMMENT='每日统计表'
        )")) {
            throw std::runtime_error("Failed to create server_stats table: " + query.lastError().text().toStdString());
        }
        
        // 提交事务
        if (!_database.commit()) {
            qCCritical(database) << "Failed to commit table creation transaction";
            return false;
        }
        
        qCInfo(database) << "Database tables created successfully";
        return true;
        
    } catch (const std::exception &e) {
        qCCritical(database) << "Exception during table creation:" << e.what();
        _database.rollback();
        return false;
    }
}

void Database::setupDatabase()
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected() && !initialize()) {
        qCCritical(database) << "Cannot setup database: database not connected";
        return;
    }
    
    // 创建表
    if (!createTables()) {
        qCCritical(database) << "Failed to create database tables";
        return;
    }
    
    // 创建默认管理员账号
    createDefaultAdminAccount();
    
    // 创建系统日志记录
    logEvent(Info, "Database", "Database setup completed successfully");
    
    qCInfo(database) << "Database setup completed successfully";
}

bool Database::createDefaultAdminAccount()
{
    QMutexLocker locker(&_mutex);
    
    if (!isConnected()) {
        qCCritical(database) << "Cannot create admin account: database not connected";
        return false;
    }
    
    // 检查是否已存在管理员账号
    QSqlQuery checkQuery(_database);
    checkQuery.prepare("SELECT COUNT(*) FROM users WHERE username = 'admin'");
    
    if (!executeQuery(checkQuery) || !checkQuery.next()) {
        qCWarning(database) << "Failed to check existing admin account";
        return false;
    }
    
    if (checkQuery.value(0).toInt() > 0) {
        qCInfo(database) << "Admin account already exists";
        return true;
    }
    
    // 从配置中获取管理员凭据
    ServerConfig *config = ServerConfig::instance();
    QString adminUsername = config->getAdminUsername();
    QString adminPassword = config->getAdminPassword();
    QString adminEmail = "admin@qkchat.com";
    
    // 生成安全的盐值
    QString salt = QUuid::createUuid().toString(QUuid::WithoutBraces);
    
    // 使用SHA-256哈希密码
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData((adminPassword + salt).toUtf8());
    QString passwordHash = QString(hash.result().toHex());
    
    // 创建管理员账号
    QSqlQuery insertQuery(_database);
    insertQuery.prepare(R"(
        INSERT INTO users (username, email, password_hash, salt, display_name, status, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, 'active', NOW(), NOW())
    )");
    
    insertQuery.addBindValue(adminUsername);
    insertQuery.addBindValue(adminEmail);
    insertQuery.addBindValue(passwordHash);
    insertQuery.addBindValue(salt);
    insertQuery.addBindValue("系统管理员");
    
    if (!executeQuery(insertQuery)) {
        qCCritical(database) << "Failed to create admin account:" << insertQuery.lastError().text();
        return false;
    }
    
    qint64 adminId = insertQuery.lastInsertId().toLongLong();
    
    // 记录管理员账号创建日志
    logEvent(Info, "Database", QString("Default admin account created with ID: %1").arg(adminId));
    
    qCInfo(database) << "Default admin account created successfully";
    qCInfo(database) << "Admin username:" << adminUsername;
    qCInfo(database) << "Admin email:" << adminEmail;
    qCInfo(database) << "Please change the default password after first login";
    
    return true;
}