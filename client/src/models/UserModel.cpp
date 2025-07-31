#include "UserModel.h"

UserModel::UserModel(QObject *parent)
    : QObject(parent)
    , _isLoggedIn(false)
{
}

void UserModel::setUsername(const QString &username)
{
    if (_username != username) {
        _username = username;
        emit usernameChanged();
    }
}

void UserModel::setEmail(const QString &email)
{
    if (_email != email) {
        _email = email;
        emit emailChanged();
    }
}

void UserModel::setPassword(const QString &password)
{
    if (_password != password) {
        _password = password;
        emit passwordChanged();
    }
}

void UserModel::setAvatar(const QUrl &avatar)
{
    if (_avatar != avatar) {
        _avatar = avatar;
        emit avatarChanged();
    }
}

void UserModel::setIsLoggedIn(bool isLoggedIn)
{
    if (_isLoggedIn != isLoggedIn) {
        _isLoggedIn = isLoggedIn;
        emit isLoggedInChanged();
    }
}

void UserModel::setToken(const QString &token)
{
    if (_token != token) {
        _token = token;
        emit tokenChanged();
    }
}

void UserModel::clear()
{
    setUsername("");
    setEmail("");
    setPassword("");
    setAvatar(QUrl());
    setIsLoggedIn(false);
    setToken("");
}

bool UserModel::isValid() const
{
    return !_username.isEmpty() && !_email.isEmpty();
} 