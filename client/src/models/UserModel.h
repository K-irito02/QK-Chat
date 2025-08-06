#ifndef USERMODEL_H
#define USERMODEL_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QDateTime>
#include <QVariantMap>

/**
 * @brief 用户数据模型类
 * 
 * 用于存储和管理用户信息，包括用户名、邮箱、头像等
 * 遵循Qt的属性系统，支持数据绑定
 */
class UserModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qint64 userId READ userId WRITE setUserId NOTIFY userIdChanged)
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)

    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged)
    Q_PROPERTY(QUrl avatar READ avatar WRITE setAvatar NOTIFY avatarChanged)
    Q_PROPERTY(QString status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn WRITE setIsLoggedIn NOTIFY isLoggedInChanged)
    Q_PROPERTY(QDateTime lastOnline READ lastOnline WRITE setLastOnline NOTIFY lastOnlineChanged)
    
public:
    explicit UserModel(QObject *parent = nullptr);
    ~UserModel();
    
    // Getter方法
    qint64 userId() const;
    QString username() const;

    QString password() const { return _password; }
    QString displayName() const;
    QUrl avatar() const;
    QString status() const;
    QString token() const;
    bool isLoggedIn() const;
    QDateTime lastOnline() const;
    
    // Setter方法
    void setUserId(qint64 userId);
    void setUsername(const QString &username);

    void setPassword(const QString &password);
    void setDisplayName(const QString &displayName);
    void setAvatar(const QUrl &avatar);
    void setStatus(const QString &status);
    void setToken(const QString &token);
    void setIsLoggedIn(bool isLoggedIn);
    void setLastOnline(const QDateTime &lastOnline);
    
    // 工具方法
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool isValid() const;
    Q_INVOKABLE void updateUserInfo(const QVariantMap &userInfo);
    Q_INVOKABLE QVariantMap toVariantMap() const;
    
signals:
    void userIdChanged();
    void usernameChanged();

    void passwordChanged();
    void displayNameChanged();
    void avatarChanged();
    void statusChanged();
    void isLoggedInChanged();
    void tokenChanged();
    void lastOnlineChanged();
    void userInfoChanged();
    
private:
    qint64 _userId;
    QString _username;

    QString _password;
    QString _displayName;
    QUrl _avatar;
    QString _status;
    QString _token;
    bool _isLoggedIn;
    QDateTime _lastOnline;
};

#endif // USERMODEL_H 