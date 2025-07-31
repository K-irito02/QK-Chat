#include "UserController.h"
#include "../models/UserModel.h"
#include "../database/LocalDatabase.h"
#include "../network/NetworkClient.h"
#include "../utils/Validator.h"
#include <QTimer>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(userController, "qkchat.client.usercontroller")

UserController::UserController(QObject *parent)
    : QObject(parent)
    , _isLoading(false)
    , _loginAttempts(0)
    , _needCaptcha(false)
{
    // 初始化依赖组件
    _userModel = new UserModel(this);
    _database = new LocalDatabase(this);
    _networkClient = new NetworkClient(this);
    _validator = new Validator(this);
    
    // 初始化计时器
    _attemptResetTimer = new QTimer(this);
    _attemptResetTimer->setSingleShot(true);
    _attemptResetTimer->setInterval(ATTEMPT_RESET_TIMEOUT);
    
    // 连接信号槽
    connect(_networkClient, &NetworkClient::loginResponse,
            this, &UserController::onLoginResponse);
    connect(_networkClient, &NetworkClient::registerResponse,
            this, &UserController::onRegisterResponse);
    connect(_networkClient, &NetworkClient::networkError,
            this, &UserController::onNetworkError);
    connect(_attemptResetTimer, &QTimer::timeout,
            this, &UserController::resetLoginAttempts);
    
    qCInfo(userController) << "UserController initialized";
}

UserController::~UserController() = default;

void UserController::login(const QString &usernameOrEmail, const QString &password, const QString &captcha)
{
    if (_isLoading) {
        qCWarning(userController) << "Login already in progress";
        return;
    }
    
    // 基础验证
    if (usernameOrEmail.isEmpty() || password.isEmpty()) {
        setErrorMessage("用户名和密码不能为空");
        return;
    }
    
    // 验证码检查
    if (_needCaptcha && !_validator->isValidVerificationCode(captcha)) {
        setErrorMessage("请输入有效的验证码");
        return;
    }
    
    setIsLoading(true);
    setErrorMessage("");
    
    // 加密密码
    QString hashedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
    
    // 发送登录请求
    _networkClient->login(usernameOrEmail, hashedPassword, captcha);
    
    qCInfo(userController) << "Login request sent for user:" << usernameOrEmail;
}

void UserController::registerUser(const QString &username, const QString &email, const QString &password, const QUrl &avatar)
{
    if (_isLoading) {
        qCWarning(userController) << "Registration already in progress";
        return;
    }
    
    // 验证输入
    if (!validateUsername(username) || !validateEmail(email) || !validatePassword(password)) {
        return;
    }
    
    setIsLoading(true);
    setErrorMessage("");
    
    // 加密密码
    QString hashedPassword = QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
    
    // 发送注册请求
    _networkClient->registerUser(username, email, hashedPassword, avatar);
    
    qCInfo(userController) << "Registration request sent for user:" << username;
}

void UserController::logout()
{
    _userModel->clear();
    _database->clearUserSession();
    _networkClient->disconnect();
    
    // 重置状态
    setErrorMessage("");
    _loginAttempts = 0;
    _needCaptcha = false;
    
    emit logoutSuccess();
    qCInfo(userController) << "User logged out successfully";
}

void UserController::refreshCaptcha()
{
    _networkClient->requestCaptcha();
}

bool UserController::validateUsername(const QString &username)
{
    bool isValid = _validator->isValidUsername(username);
    QString error = isValid ? "" : _validator->getUsernameError(username);
    
    emit usernameValidationResult(isValid, error);
    
    if (!isValid) {
        setErrorMessage(error);
    }
    
    return isValid;
}

bool UserController::validateEmail(const QString &email)
{
    bool isValid = _validator->isValidEmail(email);
    QString error = isValid ? "" : _validator->getEmailError(email);
    
    emit emailValidationResult(isValid, error);
    
    if (!isValid) {
        setErrorMessage(error);
    }
    
    return isValid;
}

bool UserController::validatePassword(const QString &password)
{
    bool isValid = _validator->isValidPassword(password);
    QString error = isValid ? "" : _validator->getPasswordError(password);
    
    emit passwordValidationResult(isValid, error);
    
    if (!isValid) {
        setErrorMessage(error);
    }
    
    return isValid;
}

bool UserController::checkUsernameAvailability(const QString &username)
{
    if (!validateUsername(username)) {
        return false;
    }
    
    _networkClient->checkUsernameAvailability(username);
    return true;
}

bool UserController::checkEmailAvailability(const QString &email)
{
    if (!validateEmail(email)) {
        return false;
    }
    
    _networkClient->checkEmailAvailability(email);
    return true;
}

QStringList UserController::getDefaultAvatars() const
{
    return QStringList{
        "qrc:/QKChatClient/resources/icons/avatar1.png",
        "qrc:/QKChatClient/resources/icons/avatar2.png",
        "qrc:/QKChatClient/resources/icons/avatar3.png",
        "qrc:/QKChatClient/resources/icons/avatar4.png",
        "qrc:/QKChatClient/resources/icons/avatar5.png"
    };
}

bool UserController::uploadCustomAvatar(const QUrl &filePath)
{
    QString path = filePath.toLocalFile();
    
    if (!_validator->isValidImageFile(path)) {
        setErrorMessage("请选择有效的图片文件（JPG/PNG格式）");
        return false;
    }
    
    if (!_validator->isValidImageSize(path, 2)) {
        setErrorMessage("图片大小不能超过2MB");
        return false;
    }
    
    // TODO: 实现头像上传到服务器
    _networkClient->uploadAvatar(filePath);
    
    return true;
}

void UserController::tryAutoLogin()
{
    if (!_database->hasStoredCredentials()) {
        qCInfo(userController) << "No stored credentials found";
        return;
    }
    
    auto credentials = _database->getStoredCredentials();
    if (!credentials.first.isEmpty() && !credentials.second.isEmpty()) {
        qCInfo(userController) << "Attempting auto login";
        login(credentials.first, credentials.second);
    }
}

void UserController::saveLoginCredentials(const QString &username, const QString &password, bool remember)
{
    if (remember) {
        _database->storeCredentials(username, password);
    } else {
        _database->clearStoredCredentials();
    }
}

void UserController::onLoginResponse(bool success, const QString &message, const QString &token)
{
    setIsLoading(false);
    
    if (success) {
        _userModel->setToken(token);
        _userModel->setIsLoggedIn(true);
        
        // 保存会话信息
        _database->saveUserSession(token);
        
        // 重置登录尝试次数
        _loginAttempts = 0;
        _needCaptcha = false;
        emit needCaptchaChanged();
        
        emit loginSuccess();
        qCInfo(userController) << "Login successful";
    } else {
        increaseLoginAttempts();
        setErrorMessage(message);
        emit loginFailed(message);
        qCWarning(userController) << "Login failed:" << message;
    }
}

void UserController::onRegisterResponse(bool success, const QString &message)
{
    setIsLoading(false);
    
    if (success) {
        emit registerSuccess();
        qCInfo(userController) << "Registration successful";
    } else {
        setErrorMessage(message);
        emit registerFailed(message);
        qCWarning(userController) << "Registration failed:" << message;
    }
}

void UserController::onNetworkError(const QString &error)
{
    setIsLoading(false);
    setErrorMessage("网络连接错误: " + error);
    qCWarning(userController) << "Network error:" << error;
}

void UserController::resetLoginAttempts()
{
    _loginAttempts = 0;
    _needCaptcha = false;
    emit loginAttemptsChanged();
    emit needCaptchaChanged();
    qCInfo(userController) << "Login attempts reset";
}

void UserController::setIsLoading(bool loading)
{
    if (_isLoading != loading) {
        _isLoading = loading;
        emit isLoadingChanged();
    }
}

void UserController::setErrorMessage(const QString &message)
{
    if (_errorMessage != message) {
        _errorMessage = message;
        emit errorMessageChanged();
    }
}

void UserController::increaseLoginAttempts()
{
    _loginAttempts++;
    emit loginAttemptsChanged();
    
    if (_loginAttempts >= MAX_LOGIN_ATTEMPTS) {
        _needCaptcha = true;
        emit needCaptchaChanged();
        generateCaptcha();
        
        // 启动重置计时器
        _attemptResetTimer->start();
        
        qCWarning(userController) << "Max login attempts reached, captcha required";
    }
}

void UserController::generateCaptcha()
{
    refreshCaptcha();
} 