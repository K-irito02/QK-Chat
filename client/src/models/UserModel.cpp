#include "UserModel.h"
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(userModel, "qkchat.client.usermodel")

UserModel::UserModel(QObject *parent)
    : QObject(parent)
    , _userId(0)
    , _isLoggedIn(false)
{
    qCInfo(userModel) << "UserModel created";
}

UserModel::~UserModel()
{
    qCInfo(userModel) << "UserModel destroyed";
}

// 属性访问器
qint64 UserModel::userId() const
{
    return _userId;
}

QString UserModel::username() const
{
    return _username;
}



QString UserModel::displayName() const
{
    return _displayName;
}

QUrl UserModel::avatar() const
{
    return _avatar;
}

QString UserModel::status() const
{
    return _status;
}

QString UserModel::token() const
{
    return _token;
}

bool UserModel::isLoggedIn() const
{
    return _isLoggedIn;
}

QDateTime UserModel::lastOnline() const
{
    return _lastOnline;
}

// 属性设置器
void UserModel::setUserId(qint64 userId)
{
    if (_userId != userId) {
        _userId = userId;
        emit userIdChanged();
    }
}

void UserModel::setUsername(const QString &username)
{
    if (_username != username) {
        _username = username;
        emit usernameChanged();
    }
}



void UserModel::setPassword(const QString &password)
{
    if (_password != password) {
        _password = password;
        emit passwordChanged();
    }
}

void UserModel::setDisplayName(const QString &displayName)
{
    if (_displayName != displayName) {
        _displayName = displayName;
        emit displayNameChanged();
    }
}

void UserModel::setAvatar(const QUrl &avatar)
{
    if (_avatar != avatar) {
        _avatar = avatar;
        emit avatarChanged();
    }
}

void UserModel::setStatus(const QString &status)
{
    if (_status != status) {
        _status = status;
        emit statusChanged();
    }
}

void UserModel::setToken(const QString &token)
{
    if (_token != token) {
        _token = token;
        emit tokenChanged();
        
        // 当token设置时，认为用户已登录
        setIsLoggedIn(!token.isEmpty());
    }
}

void UserModel::setIsLoggedIn(bool isLoggedIn)
{
    if (_isLoggedIn != isLoggedIn) {
        _isLoggedIn = isLoggedIn;
        emit isLoggedInChanged();
        
        if (isLoggedIn) {
            _lastOnline = QDateTime::currentDateTime();
            emit lastOnlineChanged();
        }
    }
}

void UserModel::setLastOnline(const QDateTime &lastOnline)
{
    if (_lastOnline != lastOnline) {
        _lastOnline = lastOnline;
        emit lastOnlineChanged();
    }
}

// 便捷方法
void UserModel::updateUserInfo(const QVariantMap &userInfo)
{
    bool changed = false;
    
    if (userInfo.contains("id")) {
        qint64 newUserId = userInfo["id"].toLongLong();
        if (_userId != newUserId) {
            _userId = newUserId;
            changed = true;
            emit userIdChanged();
        }
    }
    
    if (userInfo.contains("username")) {
        QString newUsername = userInfo["username"].toString();
        if (_username != newUsername) {
            _username = newUsername;
            changed = true;
            emit usernameChanged();
        }
    }
    

    
    if (userInfo.contains("displayName")) {
        QString newDisplayName = userInfo["displayName"].toString();
        if (_displayName != newDisplayName) {
            _displayName = newDisplayName;
            changed = true;
            emit displayNameChanged();
        }
    }
    
    if (userInfo.contains("avatarUrl")) {
        QUrl newAvatar = userInfo["avatarUrl"].toUrl();
        if (_avatar != newAvatar) {
            _avatar = newAvatar;
            changed = true;
            emit avatarChanged();
        }
    }
    
    if (userInfo.contains("status")) {
        QString newStatus = userInfo["status"].toString();
        if (_status != newStatus) {
            _status = newStatus;
            changed = true;
            emit statusChanged();
        }
    }
    
    if (changed) {
        emit userInfoChanged();
    }
}

QVariantMap UserModel::toVariantMap() const
{
    QVariantMap userInfo;
    userInfo["id"] = _userId;
    userInfo["username"] = _username;
    
    userInfo["displayName"] = _displayName;
    userInfo["avatarUrl"] = _avatar;
    userInfo["status"] = _status;
    userInfo["token"] = _token;
    userInfo["isLoggedIn"] = _isLoggedIn;
    userInfo["lastOnline"] = _lastOnline;
    
    return userInfo;
}

void UserModel::clear()
{
    bool hadData = _userId > 0 || !_username.isEmpty() || !_token.isEmpty();
    
    _userId = 0;
    _username.clear();
    
    _displayName.clear();
    _avatar = QUrl();
    _status.clear();
    _token.clear();
    _isLoggedIn = false;
    _lastOnline = QDateTime();
    
    if (hadData) {
        emit userIdChanged();
        emit usernameChanged();

        emit displayNameChanged();
        emit avatarChanged();
        emit statusChanged();
        emit tokenChanged();
        emit isLoggedInChanged();
        emit lastOnlineChanged();
        emit userInfoChanged();
        
        qCInfo(userModel) << "User data cleared";
    }
}

bool UserModel::isValid() const
{
    return !_username.isEmpty();
} 