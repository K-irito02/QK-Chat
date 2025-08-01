#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <QObject>
#include <QString>
#include <QRegularExpression>

/**
 * @brief 数据验证工具类
 * 
 * 提供用户名、邮箱、密码等字段的验证功能
 * 支持实时验证和错误提示
 */
class Validator : public QObject
{
    Q_OBJECT
    
public:
    explicit Validator(QObject *parent = nullptr);
    ~Validator();
    
    // 用户名验证规则
    Q_INVOKABLE bool isValidUsername(const QString &username) const;
    Q_INVOKABLE QString getUsernameError(const QString &username) const;
    
    // 邮箱验证规则
    Q_INVOKABLE bool isValidEmail(const QString &email) const;
    Q_INVOKABLE QString getEmailError(const QString &email) const;
    
    // 密码验证规则
    Q_INVOKABLE bool isValidPassword(const QString &password) const;
    Q_INVOKABLE QString getPasswordError(const QString &password) const;
    
    // 密码确认验证
    Q_INVOKABLE bool isPasswordMatched(const QString &password, const QString &confirmPassword) const;
    
    // 验证码验证
    Q_INVOKABLE bool isValidVerificationCode(const QString &code) const;
    
    // 文件验证
    Q_INVOKABLE bool isValidImageFile(const QString &filePath) const;
    Q_INVOKABLE bool isValidImageSize(const QString &filePath, int maxSizeMB = 2) const;
    
private:
    QRegularExpression _usernameRegex;
    QRegularExpression _emailRegex;
    QRegularExpression _passwordRegex;
    
    static const int MIN_USERNAME_LENGTH = 3;
    static const int MAX_USERNAME_LENGTH = 20;
    static const int MIN_PASSWORD_LENGTH = 8;
    static const int MAX_PASSWORD_LENGTH = 20;
};

#endif // VALIDATOR_H 