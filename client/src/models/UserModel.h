#ifndef USERMODEL_H
#define USERMODEL_H

#include <QObject>
#include <QString>
#include <QUrl>

/**
 * @brief 用户数据模型类
 * 
 * 用于存储和管理用户信息，包括用户名、邮箱、头像等
 * 遵循Qt的属性系统，支持数据绑定
 */
class UserModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString username READ username WRITE setUsername NOTIFY usernameChanged)
    Q_PROPERTY(QString email READ email WRITE setEmail NOTIFY emailChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QUrl avatar READ avatar WRITE setAvatar NOTIFY avatarChanged)
    Q_PROPERTY(bool isLoggedIn READ isLoggedIn WRITE setIsLoggedIn NOTIFY isLoggedInChanged)
    Q_PROPERTY(QString token READ token WRITE setToken NOTIFY tokenChanged)
    
public:
    explicit UserModel(QObject *parent = nullptr);
    
    // Getter方法
    QString username() const { return _username; }
    QString email() const { return _email; }
    QString password() const { return _password; }
    QUrl avatar() const { return _avatar; }
    bool isLoggedIn() const { return _isLoggedIn; }
    QString token() const { return _token; }
    
    // Setter方法
    void setUsername(const QString &username);
    void setEmail(const QString &email);
    void setPassword(const QString &password);
    void setAvatar(const QUrl &avatar);
    void setIsLoggedIn(bool isLoggedIn);
    void setToken(const QString &token);
    
    // 工具方法
    Q_INVOKABLE void clear();
    Q_INVOKABLE bool isValid() const;
    
signals:
    void usernameChanged();
    void emailChanged();
    void passwordChanged();
    void avatarChanged();
    void isLoggedInChanged();
    void tokenChanged();
    
private:
    QString _username;
    QString _email;
    QString _password;
    QUrl _avatar;
    bool _isLoggedIn;
    QString _token;
};

#endif // USERMODEL_H 