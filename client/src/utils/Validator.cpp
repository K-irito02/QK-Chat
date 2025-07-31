#include "Validator.h"
#include <QFileInfo>
#include <QImageReader>

Validator::Validator(QObject *parent)
    : QObject(parent)
{
    // 用户名正则：支持中英文、数字、下划线，不允许空格
    _usernameRegex.setPattern("^[\\u4e00-\\u9fa5a-zA-Z0-9_]+$");
    
    // 邮箱正则：标准邮箱格式
    _emailRegex.setPattern("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    
    // 密码正则：至少包含一个大写字母、一个小写字母、一个数字，支持常见符号
    _passwordRegex.setPattern("^(?=.*[a-z])(?=.*[A-Z])(?=.*\\d)[a-zA-Z\\d@$!%*?&_-]+$");
}

bool Validator::isValidUsername(const QString &username) const
{
    if (username.length() < MIN_USERNAME_LENGTH || username.length() > MAX_USERNAME_LENGTH) {
        return false;
    }
    
    // 检查是否包含空格
    if (username.contains(' ')) {
        return false;
    }
    
    return _usernameRegex.match(username).hasMatch();
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
    
    if (username.contains(' ')) {
        return "用户名不能包含空格";
    }
    
    if (!_usernameRegex.match(username).hasMatch()) {
        return "用户名只能包含中文、英文、数字和下划线";
    }
    
    return "";
}

bool Validator::isValidEmail(const QString &email) const
{
    return _emailRegex.match(email).hasMatch();
}

QString Validator::getEmailError(const QString &email) const
{
    if (email.isEmpty()) {
        return "邮箱不能为空";
    }
    
    if (!_emailRegex.match(email).hasMatch()) {
        return "请输入有效的邮箱地址";
    }
    
    return "";
}

bool Validator::isValidPassword(const QString &password) const
{
    if (password.length() < MIN_PASSWORD_LENGTH || password.length() > MAX_PASSWORD_LENGTH) {
        return false;
    }
    
    // 检查是否包含空格
    if (password.contains(' ')) {
        return false;
    }
    
    return _passwordRegex.match(password).hasMatch();
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
    
    if (password.contains(' ')) {
        return "密码不能包含空格";
    }
    
    if (!password.contains(QRegularExpression("[a-z]"))) {
        return "密码必须包含至少一个小写字母";
    }
    
    if (!password.contains(QRegularExpression("[A-Z]"))) {
        return "密码必须包含至少一个大写字母";
    }
    
    if (!password.contains(QRegularExpression("\\d"))) {
        return "密码必须包含至少一个数字";
    }
    
    if (!_passwordRegex.match(password).hasMatch()) {
        return "密码只能包含英文、数字和常见符号(@$!%*?&_-)";
    }
    
    return "";
}

bool Validator::isPasswordMatched(const QString &password, const QString &confirmPassword) const
{
    return password == confirmPassword && !password.isEmpty();
}

bool Validator::isValidVerificationCode(const QString &code) const
{
    // 验证码通常是4-6位数字
    QRegularExpression codeRegex("^\\d{4,6}$");
    return codeRegex.match(code).hasMatch();
}

bool Validator::isValidImageFile(const QString &filePath) const
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        return false;
    }
    
    QString suffix = fileInfo.suffix().toLower();
    return (suffix == "jpg" || suffix == "jpeg" || suffix == "png");
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