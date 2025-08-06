#ifndef LOCALDATABASE_H
#define LOCALDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QUrl>
#include <QPair>

/**
 * @brief 本地数据库管理类
 * 
 * 负责管理SQLite数据库，包括：
 * - 用户会话信息存储
 * - 登录凭据存储
 * - 消息缓存
 * - 用户配置
 */
class LocalDatabase : public QObject
{
    Q_OBJECT
    
public:
    explicit LocalDatabase(QObject *parent = nullptr);
    ~LocalDatabase();
    
    // 数据库初始化
    bool initialize();
    void close();
    
    // 用户会话管理
    bool saveUserSession(const QString &token);
    QString getUserSession();
    void clearUserSession();
    
    // 登录凭据管理
    bool storeCredentials(const QString &username, const QString &password);
    QPair<QString, QString> getStoredCredentials();
    bool hasStoredCredentials();
    void clearStoredCredentials();
    
    // 用户信息管理
    bool saveUserInfo(const QString &username, const QUrl &avatar);
    QVariantMap getUserInfo();
    void clearUserInfo();
    
    // 消息缓存管理
    bool saveMessage(const QString &messageId, const QString &sender, const QString &receiver, 
                    const QString &content, const QString &messageType, qint64 timestamp);
    QVariantList getMessages(const QString &chatId, int limit = 50, int offset = 0);
    bool updateMessageStatus(const QString &messageId, const QString &status);
    void clearOldMessages(int days = 90);
    
    // 联系人管理
    bool saveContact(const QString &userId, const QString &username, const QString &nickname, const QUrl &avatar);
    QVariantList getContacts();
    bool updateContact(const QString &userId, const QVariantMap &data);
    void clearContacts();
    
signals:
    void databaseError(const QString &error);
    void databaseReady();
    
private:
    bool createTables();
    bool executeQuery(const QString &sql, const QVariantList &params = QVariantList());
    QSqlQuery prepareQuery(const QString &sql);
    QString getConnectionName() const;
    
    QSqlDatabase _database;
    bool _isInitialized;
    
    static const QString DATABASE_NAME;
    static const int DATABASE_VERSION;
};

#endif // LOCALDATABASE_H 