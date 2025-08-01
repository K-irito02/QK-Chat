#ifndef USERCONTROLLER_H
#define USERCONTROLLER_H

#include <QObject>
#include <QString>
#include <QUrl>
#include <QTimer>

class UserModel;
class LocalDatabase;
class NetworkClient;
class Validator;

/**
 * @brief 用户控制器类
 * 
 * 处理用户登录、注册、验证等业务逻辑
 * 协调Model、View和Service层的交互
 */
class UserController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(int loginAttempts READ loginAttempts NOTIFY loginAttemptsChanged)
    Q_PROPERTY(bool needCaptcha READ needCaptcha NOTIFY needCaptchaChanged)
    Q_PROPERTY(QString captchaImage READ captchaImage NOTIFY captchaImageChanged)
    
public:
    explicit UserController(QObject *parent = nullptr);
    ~UserController();
    
    // 属性getter方法
    bool isLoading() const { return _isLoading; }
    QString errorMessage() const { return _errorMessage; }
    int loginAttempts() const { return _loginAttempts; }
    bool needCaptcha() const { return _needCaptcha; }
    QString captchaImage() const { return _captchaImage; }
    
    // Model访问器
    UserModel* userModel() const { return _userModel; }
    
    // 依赖注入方法
    void setUserModel(UserModel *userModel);
    void setDatabase(LocalDatabase *database);
    void setNetworkClient(NetworkClient *networkClient);
    void setValidator(Validator *validator);
    
    // 用户操作方法
    Q_INVOKABLE void login(const QString &usernameOrEmail, const QString &password, const QString &captcha = "");
    Q_INVOKABLE void registerUser(const QString &username, const QString &email, const QString &password, const QUrl &avatar);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void refreshCaptcha();
    
    // 验证方法
    Q_INVOKABLE bool validateUsername(const QString &username);
    Q_INVOKABLE bool validateEmail(const QString &email);
    Q_INVOKABLE bool validatePassword(const QString &password);
    Q_INVOKABLE bool checkUsernameAvailability(const QString &username);
    Q_INVOKABLE bool checkEmailAvailability(const QString &email);
    
    // 头像管理
    Q_INVOKABLE QStringList getDefaultAvatars() const;
    Q_INVOKABLE bool uploadCustomAvatar(const QUrl &filePath);
    
    // 自动登录
    Q_INVOKABLE void tryAutoLogin();
    Q_INVOKABLE void saveLoginCredentials(const QString &username, const QString &password, bool remember);
    
signals:
    void isLoadingChanged();
    void errorMessageChanged();
    void loginAttemptsChanged();
    void needCaptchaChanged();
    void captchaImageChanged();
    
    void loginSuccess();
    void loginFailed(const QString &error);
    void registerSuccess();
    void registerFailed(const QString &error);
    void logoutSuccess();
    
    void usernameValidationResult(bool isValid, const QString &error);
    void emailValidationResult(bool isValid, const QString &error);
    void passwordValidationResult(bool isValid, const QString &error);
    void usernameAvailabilityResult(bool isAvailable);
    void emailAvailabilityResult(bool isAvailable);
    
private slots:
    void onLoginResponse(bool success, const QString &message, const QString &token);
    void onRegisterResponse(bool success, const QString &message);
    void onNetworkError(const QString &error);
    void resetLoginAttempts();
    
private:
    void setIsLoading(bool loading);
    void setErrorMessage(const QString &message);
    void increaseLoginAttempts();
    void generateCaptcha();
    
    UserModel *_userModel;
    LocalDatabase *_database;
    NetworkClient *_networkClient;
    Validator *_validator;
    
    bool _isLoading;
    QString _errorMessage;
    int _loginAttempts;
    bool _needCaptcha;
    QString _captchaImage;
    QTimer *_resetTimer;
};

#endif // USERCONTROLLER_H 