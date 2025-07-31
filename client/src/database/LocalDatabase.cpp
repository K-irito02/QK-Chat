#include "LocalDatabase.h"
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QVariant>
#include <QLoggingCategory>
#include <QCryptographicHash>

Q_LOGGING_CATEGORY(localDatabase, "qkchat.client.localdatabase")

const QString LocalDatabase::DATABASE_NAME = "qkchat_client.db";
const int LocalDatabase::DATABASE_VERSION = 1;

LocalDatabase::LocalDatabase(QObject *parent)
    : QObject(parent)
    , _isInitialized(false)
{
}

LocalDatabase::~LocalDatabase()
{
    close();
}

bool LocalDatabase::initialize()
{
    if (_isInitialized) {
        return true;
    }
    
    // 创建数据库目录
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    
    // 设置数据库路径
    QString dbPath = dataPath + "/" + DATABASE_NAME;
    
    // 创建数据库连接
    _database = QSqlDatabase::addDatabase("QSQLITE", getConnectionName());
    _database.setDatabaseName(dbPath);
    
    if (!_database.open()) {
        QString error = QString("Failed to open database: %1").arg(_database.lastError().text());
        qCCritical(localDatabase) << error;
        emit databaseError(error);
        return false;
    }
    
    // 创建表结构
    if (!createTables()) {
        close();
        return false;
    }
    
    _isInitialized = true;
    emit databaseReady();
    
    qCInfo(localDatabase) << "Database initialized successfully at:" << dbPath;
    return true;
}

void LocalDatabase::close()
{
    if (_database.isOpen()) {
        _database.close();
    }
    QSqlDatabase::removeDatabase(getConnectionName());
    _isInitialized = false;
}

bool LocalDatabase::saveUserSession(const QString &token)
{
    if (!initialize()) return false;
    
    QString sql = "INSERT OR REPLACE INTO user_session (id, token, created_at) VALUES (1, ?, ?)";
    QVariantList params;
    params << token << QDateTime::currentSecsSinceEpoch();
    
    return executeQuery(sql, params);
}

QString LocalDatabase::getUserSession()
{
    if (!initialize()) return QString();
    
    QString sql = "SELECT token FROM user_session WHERE id = 1";
    QSqlQuery query = prepareQuery(sql);
    
    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    
    return QString();
}

void LocalDatabase::clearUserSession()
{
    if (!initialize()) return;
    
    QString sql = "DELETE FROM user_session";
    executeQuery(sql);
}

bool LocalDatabase::storeCredentials(const QString &username, const QString &password)
{
    if (!initialize()) return false;
    
    // 简单加密存储
    QByteArray encryptedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256);
    
    QString sql = "INSERT OR REPLACE INTO stored_credentials (id, username, password, created_at) VALUES (1, ?, ?, ?)";
    QVariantList params;
    params << username << encryptedPassword.toHex() << QDateTime::currentSecsSinceEpoch();
    
    return executeQuery(sql, params);
}

QPair<QString, QString> LocalDatabase::getStoredCredentials()
{
    if (!initialize()) return QPair<QString, QString>();
    
    QString sql = "SELECT username, password FROM stored_credentials WHERE id = 1";
    QSqlQuery query = prepareQuery(sql);
    
    if (query.exec() && query.next()) {
        QString username = query.value(0).toString();
        QString encryptedPassword = query.value(1).toString();
        // 注意：这里返回加密后的密码，实际使用时需要相应处理
        return QPair<QString, QString>(username, encryptedPassword);
    }
    
    return QPair<QString, QString>();
}

bool LocalDatabase::hasStoredCredentials()
{
    if (!initialize()) return false;
    
    QString sql = "SELECT COUNT(*) FROM stored_credentials WHERE id = 1";
    QSqlQuery query = prepareQuery(sql);
    
    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    
    return false;
}

void LocalDatabase::clearStoredCredentials()
{
    if (!initialize()) return;
    
    QString sql = "DELETE FROM stored_credentials";
    executeQuery(sql);
}

bool LocalDatabase::saveUserInfo(const QString &username, const QString &email, const QUrl &avatar)
{
    if (!initialize()) return false;
    
    QString sql = "INSERT OR REPLACE INTO user_info (id, username, email, avatar, updated_at) VALUES (1, ?, ?, ?, ?)";
    QVariantList params;
    params << username << email << avatar.toString() << QDateTime::currentSecsSinceEpoch();
    
    return executeQuery(sql, params);
}

QVariantMap LocalDatabase::getUserInfo()
{
    if (!initialize()) return QVariantMap();
    
    QString sql = "SELECT username, email, avatar FROM user_info WHERE id = 1";
    QSqlQuery query = prepareQuery(sql);
    
    QVariantMap userInfo;
    if (query.exec() && query.next()) {
        userInfo["username"] = query.value(0).toString();
        userInfo["email"] = query.value(1).toString();
        userInfo["avatar"] = query.value(2).toString();
    }
    
    return userInfo;
}

void LocalDatabase::clearUserInfo()
{
    if (!initialize()) return;
    
    QString sql = "DELETE FROM user_info";
    executeQuery(sql);
}

bool LocalDatabase::saveMessage(const QString &messageId, const QString &sender, const QString &receiver, 
                               const QString &content, const QString &messageType, qint64 timestamp)
{
    if (!initialize()) return false;
    
    QString sql = "INSERT OR REPLACE INTO messages (message_id, sender, receiver, content, message_type, timestamp, status) "
                  "VALUES (?, ?, ?, ?, ?, ?, 'sent')";
    QVariantList params;
    params << messageId << sender << receiver << content << messageType << timestamp;
    
    return executeQuery(sql, params);
}

QVariantList LocalDatabase::getMessages(const QString &chatId, int limit, int offset)
{
    if (!initialize()) return QVariantList();
    
    QString sql = "SELECT message_id, sender, receiver, content, message_type, timestamp, status "
                  "FROM messages WHERE sender = ? OR receiver = ? "
                  "ORDER BY timestamp DESC LIMIT ? OFFSET ?";
    QSqlQuery query = prepareQuery(sql);
    query.addBindValue(chatId);
    query.addBindValue(chatId);
    query.addBindValue(limit);
    query.addBindValue(offset);
    
    QVariantList messages;
    if (query.exec()) {
        while (query.next()) {
            QVariantMap message;
            message["messageId"] = query.value(0).toString();
            message["sender"] = query.value(1).toString();
            message["receiver"] = query.value(2).toString();
            message["content"] = query.value(3).toString();
            message["messageType"] = query.value(4).toString();
            message["timestamp"] = query.value(5).toLongLong();
            message["status"] = query.value(6).toString();
            messages.append(message);
        }
    }
    
    return messages;
}

bool LocalDatabase::updateMessageStatus(const QString &messageId, const QString &status)
{
    if (!initialize()) return false;
    
    QString sql = "UPDATE messages SET status = ? WHERE message_id = ?";
    QVariantList params;
    params << status << messageId;
    
    return executeQuery(sql, params);
}

void LocalDatabase::clearOldMessages(int days)
{
    if (!initialize()) return;
    
    qint64 cutoffTime = QDateTime::currentSecsSinceEpoch() - (days * 24 * 60 * 60);
    QString sql = "DELETE FROM messages WHERE timestamp < ?";
    QVariantList params;
    params << cutoffTime;
    
    executeQuery(sql, params);
}

bool LocalDatabase::saveContact(const QString &userId, const QString &username, const QString &nickname, const QUrl &avatar)
{
    if (!initialize()) return false;
    
    QString sql = "INSERT OR REPLACE INTO contacts (user_id, username, nickname, avatar, updated_at) "
                  "VALUES (?, ?, ?, ?, ?)";
    QVariantList params;
    params << userId << username << nickname << avatar.toString() << QDateTime::currentSecsSinceEpoch();
    
    return executeQuery(sql, params);
}

QVariantList LocalDatabase::getContacts()
{
    if (!initialize()) return QVariantList();
    
    QString sql = "SELECT user_id, username, nickname, avatar FROM contacts ORDER BY username";
    QSqlQuery query = prepareQuery(sql);
    
    QVariantList contacts;
    if (query.exec()) {
        while (query.next()) {
            QVariantMap contact;
            contact["userId"] = query.value(0).toString();
            contact["username"] = query.value(1).toString();
            contact["nickname"] = query.value(2).toString();
            contact["avatar"] = query.value(3).toString();
            contacts.append(contact);
        }
    }
    
    return contacts;
}

bool LocalDatabase::updateContact(const QString &userId, const QVariantMap &data)
{
    if (!initialize()) return false;
    
    QString sql = "UPDATE contacts SET ";
    QVariantList params;
    QStringList setParts;
    
    if (data.contains("nickname")) {
        setParts << "nickname = ?";
        params << data["nickname"];
    }
    if (data.contains("avatar")) {
        setParts << "avatar = ?";
        params << data["avatar"];
    }
    
    if (setParts.isEmpty()) return false;
    
    sql += setParts.join(", ") + " WHERE user_id = ?";
    params << userId;
    
    return executeQuery(sql, params);
}

void LocalDatabase::clearContacts()
{
    if (!initialize()) return;
    
    QString sql = "DELETE FROM contacts";
    executeQuery(sql);
}

bool LocalDatabase::createTables()
{
    QStringList createStatements;
    
    // 用户会话表
    createStatements << R"(
        CREATE TABLE IF NOT EXISTS user_session (
            id INTEGER PRIMARY KEY,
            token TEXT NOT NULL,
            created_at INTEGER NOT NULL
        )
    )";
    
    // 存储的登录凭据
    createStatements << R"(
        CREATE TABLE IF NOT EXISTS stored_credentials (
            id INTEGER PRIMARY KEY,
            username TEXT NOT NULL,
            password TEXT NOT NULL,
            created_at INTEGER NOT NULL
        )
    )";
    
    // 用户信息
    createStatements << R"(
        CREATE TABLE IF NOT EXISTS user_info (
            id INTEGER PRIMARY KEY,
            username TEXT NOT NULL,
            email TEXT NOT NULL,
            avatar TEXT,
            updated_at INTEGER NOT NULL
        )
    )";
    
    // 消息缓存
    createStatements << R"(
        CREATE TABLE IF NOT EXISTS messages (
            message_id TEXT PRIMARY KEY,
            sender TEXT NOT NULL,
            receiver TEXT NOT NULL,
            content TEXT NOT NULL,
            message_type TEXT NOT NULL DEFAULT 'text',
            timestamp INTEGER NOT NULL,
            status TEXT NOT NULL DEFAULT 'sent'
        )
    )";
    
    // 联系人
    createStatements << R"(
        CREATE TABLE IF NOT EXISTS contacts (
            user_id TEXT PRIMARY KEY,
            username TEXT NOT NULL,
            nickname TEXT,
            avatar TEXT,
            updated_at INTEGER NOT NULL
        )
    )";
    
    // 执行所有创建语句
    for (const QString &sql : createStatements) {
        if (!executeQuery(sql)) {
            qCCritical(localDatabase) << "Failed to create table";
            return false;
        }
    }
    
    // 创建索引
    QStringList indexStatements;
    indexStatements << "CREATE INDEX IF NOT EXISTS idx_messages_timestamp ON messages (timestamp)";
    indexStatements << "CREATE INDEX IF NOT EXISTS idx_messages_sender ON messages (sender)";
    indexStatements << "CREATE INDEX IF NOT EXISTS idx_messages_receiver ON messages (receiver)";
    
    for (const QString &sql : indexStatements) {
        executeQuery(sql);
    }
    
    return true;
}

bool LocalDatabase::executeQuery(const QString &sql, const QVariantList &params)
{
    QSqlQuery query = prepareQuery(sql);
    
    for (const QVariant &param : params) {
        query.addBindValue(param);
    }
    
    if (!query.exec()) {
        QString error = QString("SQL execution failed: %1 - %2").arg(query.lastError().text(), sql);
        qCWarning(localDatabase) << error;
        emit databaseError(error);
        return false;
    }
    
    return true;
}

QSqlQuery LocalDatabase::prepareQuery(const QString &sql)
{
    QSqlQuery query(_database);
    query.prepare(sql);
    return query;
}

QString LocalDatabase::getConnectionName() const
{
    return QString("LocalDatabase_%1").arg(reinterpret_cast<quintptr>(this));
} 