#include "Validator.h"
#include <QRegularExpression>
#include <QFileInfo>
#include <QImageReader>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(validator, "qkchat.client.validator")

Validator::Validator(QObject *parent)
    : QObject(parent)
{
    qCInfo(validator) << "Validator created";
}

Validator::~Validator()
{
    qCInfo(validator) << "Validator destroyed";
}

bool Validator::isValidUsername(const QString &username) const
{
    if (username.length() < MIN_USERNAME_LENGTH || username.length() > MAX_USERNAME_LENGTH) {
        return false;
    }
    
    // 只允许字母、数字、下划线，不能以数字开头
    QRegularExpression regex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    return regex.match(username).hasMatch();
}

QString Validator::getUsernameError(const QString &username) const
{
    if (username.isEmpty()) {
        return "用户名不能为空";
    }
    
    if (username.length() < MIN_USERNAME_LENGTH) {
        return QString("用户名长度不能少于%1个字符").arg(MIN_USERNAME_LENGTH);
    }
    
    if (username.length() > MAX_USERNAME_LENGTH) {
        return QString("用户名长度不能超过%1个字符").arg(MAX_USERNAME_LENGTH);
    }
    
    QRegularExpression regex("^[a-zA-Z_][a-zA-Z0-9_]*$");
    if (!regex.match(username).hasMatch()) {
        return "用户名只能包含字母、数字和下划线，且不能以数字开头";
    }
    
    return QString();
}

bool Validator::isValidEmail(const QString &email) const
{
    // 简单的邮箱验证正则表达式
    QRegularExpression regex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    return regex.match(email).hasMatch();
}

QString Validator::getEmailError(const QString &email) const
{
    if (email.isEmpty()) {
        return "邮箱不能为空";
    }
    
    QRegularExpression regex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!regex.match(email).hasMatch()) {
        return "邮箱格式不正确";
    }
    
    return QString();
}

bool Validator::isValidPassword(const QString &password) const
{
    if (password.length() < MIN_PASSWORD_LENGTH || password.length() > MAX_PASSWORD_LENGTH) {
        return false;
    }
    
    // 检查是否包含字母和数字
    QRegularExpression hasLetter("[a-zA-Z]");
    QRegularExpression hasNumber("[0-9]");
    
    return hasLetter.match(password).hasMatch() && hasNumber.match(password).hasMatch();
}

QString Validator::getPasswordError(const QString &password) const
{
    if (password.isEmpty()) {
        return "密码不能为空";
    }
    
    if (password.length() < MIN_PASSWORD_LENGTH) {
        return QString("密码长度不能少于%1个字符").arg(MIN_PASSWORD_LENGTH);
    }
    
    if (password.length() > MAX_PASSWORD_LENGTH) {
        return QString("密码长度不能超过%1个字符").arg(MAX_PASSWORD_LENGTH);
    }
    
    QRegularExpression hasLetter("[a-zA-Z]");
    QRegularExpression hasNumber("[0-9]");
    
    if (!hasLetter.match(password).hasMatch()) {
        return "密码必须包含至少一个字母";
    }
    
    if (!hasNumber.match(password).hasMatch()) {
        return "密码必须包含至少一个数字";
    }
    
    return QString();
}

bool Validator::isPasswordMatched(const QString &password, const QString &confirmPassword) const
{
    return password == confirmPassword;
}

bool Validator::isValidVerificationCode(const QString &code) const
{
    // 简单验证码验证（4-6位数字或字母）
    if (code.length() < 4 || code.length() > 6) {
        return false;
    }
    
    QRegularExpression regex("^[a-zA-Z0-9]+$");
    return regex.match(code).hasMatch();
}

bool Validator::isValidImageFile(const QString &filePath) const
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return false;
    }
    
    QImageReader reader(filePath);
    return reader.canRead();
}

bool Validator::isValidImageSize(const QString &filePath, int maxSizeMB) const
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        return false;
    }
    
    qint64 maxSize = maxSizeMB * 1024 * 1024; // 转换为字节
    return fileInfo.size() <= maxSize;
} 