#include "UserController.h"
#include "../models/UserModel.h"
#include "../database/LocalDatabase.h"
#include "../network/NetworkClient.h"
#include "../utils/Validator.h"
#include "../utils/ThreadPool.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariant>
#include <QVariantMap>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QLoggingCategory>
#include <QDebug>
#include <QRandomGenerator>
#include <QTimer>

Q_LOGGING_CATEGORY(userController, "qkchat.client.usercontroller")

// 常量定义
static const int MAX_LOGIN_ATTEMPTS = 3;
static const int ATTEMPT_RESET_TIMEOUT = 300000; // 5分钟

UserController::UserController(QObject *parent)
    : QObject(parent)
    , _userModel(new UserModel(this))
    , _database(nullptr)
    , _networkClient(new NetworkClient(this)) // 在构造函数中初始化
    , _validator(new Validator(this))
    , _threadPool(nullptr)
    , _isLoading(false)
    , _loginAttempts(0)
    , _needCaptcha(false)
    , _resetTimer(new QTimer(this))
{
    // 配置重置定时器
    _resetTimer->setSingleShot(true);
    _resetTimer->setInterval(ATTEMPT_RESET_TIMEOUT);
    connect(_resetTimer, &QTimer::timeout, this, &UserController::resetLoginAttempts);

    // 连接所有NetworkClient的信号
    connect(_networkClient, &NetworkClient::loginResponse, this, &UserController::onLoginResponse);
    connect(_networkClient, &NetworkClient::registerResponse, this, &UserController::onRegisterResponse);

    connect(_networkClient, &NetworkClient::usernameAvailability, this, &UserController::onUsernameAvailability);

    connect(_networkClient, &NetworkClient::connectionError, this, &UserController::onNetworkError);
    connect(_networkClient, &NetworkClient::captchaReceived, this, [this](const QString &captchaImage) {
        _captchaImage = captchaImage;
        emit captchaImageChanged();
    });
    
    // 连接邮箱验证相关信号
    connect(_networkClient, &NetworkClient::emailAvailability, this, &UserController::onEmailAvailability);
    connect(_networkClient, &NetworkClient::emailVerificationCodeSent, this, &UserController::onEmailVerificationCodeSent);
    connect(_networkClient, &NetworkClient::emailVerificationCodeVerified, this, &UserController::onEmailVerificationCodeVerified);


    // 尝试连接到服务器
    _networkClient->connectToServer("localhost", 8443);

    qCInfo(userController) << "UserController created and network client initialized";
}

UserController::~UserController()
{
    qCInfo(userController) << "UserController destroyed";
}

// 依赖注入方法
void UserController::setUserModel(UserModel *userModel)
{
    if (_userModel != userModel) {
        _userModel = userModel;
    }
}

void UserController::setDatabase(LocalDatabase *database)
{
    if (_database != database) {
        _database = database;
    }
}

void UserController::setNetworkClient(NetworkClient *networkClient)
{
    if (_networkClient != networkClient) {
        _networkClient = networkClient;
    }
}

void UserController::setValidator(Validator *validator)
{
    if (_validator != validator) {
        _validator = validator;
    }
}

void UserController::setThreadPool(ThreadPool *threadPool)
{
    if (_threadPool != threadPool) {
        _threadPool = threadPool;
    }
}

// 用户操作方法
void UserController::login(const QString &usernameOrEmail, const QString &password, const QString &captcha)
{
    qCInfo(userController) << "Login attempt for:" << usernameOrEmail;
    
    if (_isLoading) {
        qCWarning(userController) << "Already processing a request";
        return;
    }
    
    // 基本验证
    if (usernameOrEmail.trimmed().isEmpty()) {
        setErrorMessage("用户名或邮箱不能为空");
        emit loginFailed("用户名或邮箱不能为空");
        return;
    }
    
    if (password.isEmpty()) {
        setErrorMessage("密码不能为空");
        emit loginFailed("密码不能为空");
        return;
    }
    
    // 验证码检查
    if (_needCaptcha && captcha.trimmed().isEmpty()) {
        setErrorMessage("请输入验证码");
        emit loginFailed("请输入验证码");
        return;
    }
    
    setIsLoading(true);
    setErrorMessage("");
    
    ensureNetworkClient();
    
    // 异步发送登录请求
    if (_threadPool) {
        _threadPool->enqueue([this, usernameOrEmail, password, captcha]() {
            _networkClient->login(usernameOrEmail.trimmed(), password, captcha);
        });
    } else {
        _networkClient->login(usernameOrEmail.trimmed(), password, captcha);
    }
    
    qCInfo(userController) << "Login request sent";
}

void UserController::registerUser(const QString &username, const QString &email, const QString &verificationCode, const QString &password, const QUrl &avatar)
{
    qCInfo(userController) << "Register attempt for:" << username << "email:" << email;
    
    if (_isLoading) {
        qCWarning(userController) << "Already processing a request";
        return;
    }
    
    // 验证输入
    if (!validateUsername(username)) {
        emit registerFailed("用户名格式不正确");
        return;
    }
    
    if (!validateEmail(email)) {
        emit registerFailed("邮箱格式不正确");
        return;
    }
    
    if (verificationCode.isEmpty() || verificationCode.length() != 6) {
        emit registerFailed("验证码必须是6位数字");
        return;
    }
    
    if (!validatePassword(password)) {
        emit registerFailed("密码格式不正确");
        return;
    }
    
    setIsLoading(true);
    setErrorMessage("");
    
    ensureNetworkClient();
    
    // 异步发送注册请求
    if (_threadPool) {
        _threadPool->enqueue([this, username, email, verificationCode, password, avatar]() {
            _networkClient->registerUser(username.trimmed(), email, verificationCode, password, avatar);
        });
    } else {
        _networkClient->registerUser(username.trimmed(), email, verificationCode, password, avatar);
    }
    
    qCInfo(userController) << "Register request sent";
}

void UserController::logout()
{
    qCInfo(userController) << "Logout requested";
    
    if (_networkClient) {
        _networkClient->logout();
    }
    
    // 清除本地用户数据
    _userModel->clear();
    
    // 清除保存的登录凭据
    if (_database) {
        // TODO: 清除本地存储的登录信息
    }
    
    // 重置状态
    _loginAttempts = 0;
    _needCaptcha = false;
    setErrorMessage("");
    
    emit logoutSuccess();
    qCInfo(userController) << "Logout completed";
}

void UserController::refreshCaptcha()
{
    qCInfo(userController) << "Refreshing captcha";
    
    ensureNetworkClient();
    _networkClient->requestCaptcha();
    generateCaptcha();
}

// 验证方法
bool UserController::validateUsername(const QString &username)
{
    if (!_validator) {
        return false;
    }
    
    bool isValid = _validator->isValidUsername(username);
    QString error = isValid ? "" : "用户名必须是3-20个字符，只能包含字母、数字和下划线，且不能以数字开头";
    
    emit usernameValidationResult(isValid, error);
    return isValid;
}



bool UserController::validatePassword(const QString &password)
{
    if (!_validator) {
        return false;
    }
    
    bool isValid = _validator->isValidPassword(password);
    QString error = isValid ? "" : "密码必须至少8个字符，包含字母和数字";
    
    emit passwordValidationResult(isValid, error);
    return isValid;
}

bool UserController::checkUsernameAvailability(const QString &username)
{
    qCInfo(userController) << "Checking username availability:" << username;
    
    if (!validateUsername(username)) {
        emit usernameAvailabilityResult(false);
        return false;
    }
    
    ensureNetworkClient();
    _networkClient->checkUsernameAvailability(username.trimmed());
    return true; // 实际结果通过信号返回
}

bool UserController::checkEmailAvailability(const QString &email)
{
    qCInfo(userController) << "Checking email availability:" << email;
    
    if (!validateEmail(email)) {
        emit emailAvailabilityResult(false);
        return false;
    }
    
    ensureNetworkClient();
    _networkClient->checkEmailAvailability(email.trimmed());
    return true; // 实际结果通过信号返回
}

bool UserController::validateEmail(const QString &email)
{
    if (!_validator) {
        return false;
    }
    
    bool isValid = _validator->validateEmail(email);
    QString error = isValid ? "" : "邮箱格式不正确";
    emit emailValidationResult(isValid, error);
    return isValid;
}

void UserController::sendEmailVerificationCode(const QString &email)
{
    qCInfo(userController) << "=== USER CONTROLLER: SENDING EMAIL VERIFICATION CODE ===";
    qCInfo(userController) << "Email:" << email;
    qCInfo(userController) << "Network client exists:" << (_networkClient != nullptr);
    
    if (!_networkClient) {
        qCWarning(userController) << "Network client not initialized";
        emit emailVerificationCodeSent(false, "网络客户端未初始化");
        return;
    }
    
    qCInfo(userController) << "Network client is valid, calling sendEmailVerificationCode";
    _networkClient->sendEmailVerificationCode(email);
    qCInfo(userController) << "=== END USER CONTROLLER: EMAIL VERIFICATION CODE ===";
}

QString UserController::testMethod(const QString &input)
{
    qCInfo(userController) << "=== TEST METHOD CALLED ===";
    qCInfo(userController) << "Input:" << input;
    qCInfo(userController) << "Network client exists:" << (_networkClient != nullptr);
    qCInfo(userController) << "=== END TEST METHOD ===";
    return "Test method called with: " + input;
}

void UserController::verifyEmailCode(const QString &email, const QString &code)
{
    if (!_networkClient) {
        emit emailVerificationCodeVerified(false, "网络客户端未初始化");
        return;
    }
    
    _networkClient->verifyEmailCode(email, code);
}





// 头像管理
QStringList UserController::getDefaultAvatars() const
{
    QStringList avatars;
    avatars << "qrc:/icons/avatar1.png"
            << "qrc:/icons/avatar2.png"
            << "qrc:/icons/avatar3.png"
            << "qrc:/icons/avatar4.png"
            << "qrc:/icons/avatar5.png";
    return avatars;
}

bool UserController::uploadCustomAvatar(const QUrl &filePath)
{
    qCInfo(userController) << "Uploading custom avatar:" << filePath.toString();
    
    QString localPath = filePath.toLocalFile();
    QFileInfo fileInfo(localPath);
    
    // 验证文件存在
    if (!fileInfo.exists()) {
        setErrorMessage("文件不存在");
        return false;
    }
    
    // 验证文件类型
    QImageReader reader(localPath);
    if (!reader.canRead()) {
        setErrorMessage("不支持的图片格式");
        return false;
    }
    
    // 验证文件大小（限制2MB）
    if (fileInfo.size() > 2 * 1024 * 1024) {
        setErrorMessage("图片大小不能超过2MB");
        return false;
    }
    
    ensureNetworkClient();
    // The connection for avatarUploaded should be in the constructor now
    // We might need a specific slot for it if the lambda logic is complex
    connect(_networkClient, &NetworkClient::avatarUploaded, this, [this](bool success, const QUrl &avatarUrl) {
        setIsLoading(false);
        if (success) {
            _userModel->setAvatar(avatarUrl);
            qCInfo(userController) << "Avatar uploaded successfully:" << avatarUrl.toString();
        } else {
            setErrorMessage("头像上传失败");
            qCWarning(userController) << "Avatar upload failed";
        }
    });
    
    setIsLoading(true);
    _networkClient->uploadAvatar(filePath);
    
    return true;
}

// 自动登录
void UserController::tryAutoLogin()
{
    qCInfo(userController) << "Attempting auto login";
    
    if (!_database) {
        qCInfo(userController) << "No database available for auto login";
        return;
    }
    
    // TODO: 从本地数据库加载保存的登录凭据
    // 如果有保存的凭据且有效，则自动登录
    
    qCInfo(userController) << "Auto login check completed";
}

void UserController::saveLoginCredentials(const QString &username, const QString &password, bool remember)
{
    qCInfo(userController) << "Saving login credentials, remember:" << remember;
    
    if (!_database || !remember) {
        return;
    }
    
    // TODO: 保存登录凭据到本地数据库（需要加密）
    qCInfo(userController) << "Login credentials saved";
}



// 网络响应处理槽函数
void UserController::onLoginResponse(bool success, const QString &message)
{
    setIsLoading(false);
    
    if (success) {
        qCInfo(userController) << "Login successful";
        
        // 重置登录尝试次数
        _loginAttempts = 0;
        _needCaptcha = false;
        setErrorMessage("");
        
        emit loginSuccess();
    } else {
        qCWarning(userController) << "Login failed:" << message;
        
        increaseLoginAttempts();
        setErrorMessage(message);
        emit loginFailed(message);
    }
}

void UserController::onRegisterResponse(bool success, const QString &message)
{
    setIsLoading(false);
    
    if (success) {
        qCInfo(userController) << "Registration successful";
        setErrorMessage("");
        emit registerSuccess("", 0);
    } else {
        qCWarning(userController) << "Registration failed:" << message;
        setErrorMessage(message);
        emit registerFailed(message);
    }
}

void UserController::onNetworkError(const QString &error)
{
    setIsLoading(false);
    setErrorMessage("网络错误: " + error);
    qCWarning(userController) << "Network error:" << error;
    
    emit loginFailed("网络连接失败");
}



void UserController::resetLoginAttempts()
{
    _loginAttempts = 0;
    _needCaptcha = false;
    emit loginAttemptsChanged();
    emit needCaptchaChanged();
    
    qCInfo(userController) << "Login attempts reset";
}

// 私有方法
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
        
        // 启动重置定时器
        _resetTimer->start();
        
        qCWarning(userController) << "Max login attempts reached, captcha required";
    }
}

void UserController::generateCaptcha()
{
    // 生成简单的数学验证码
    int a = QRandomGenerator::global()->bounded(1, 10);
    int b = QRandomGenerator::global()->bounded(1, 10);
    QString captchaText = QString("%1 + %2 = ?").arg(a).arg(b);
    
    // TODO: 生成验证码图片
    // 这里可以使用QPainter绘制验证码图片
    // 或者从服务器获取验证码图片
    
    _captchaImage = QString("data:image/svg+xml;base64,%1")
                   .arg(QString("PHN2ZyB3aWR0aD0iMTAwIiBoZWlnaHQ9IjMwIj4KICA8dGV4dCB4PSI1IiB5PSIyMCI+%1</dGV4dD4KPC9zdmc+")
                        .arg(captchaText.toUtf8().toBase64()));
    
    emit captchaImageChanged();
}


// 确保网络客户端已初始化的辅助方法
void UserController::ensureNetworkClient()
{
    // The client is now initialized in the constructor, so this function just checks the connection.
    if (!_networkClient->isConnected()) {
        qCInfo(userController) << "NetworkClient is not connected. Attempting to connect.";
        _networkClient->connectToServer("localhost", 8443);
    }
}

// Implement the new slots
void UserController::onUsernameAvailability(bool available)
{
    emit usernameAvailabilityResult(available);
}

void UserController::onEmailAvailability(bool available)
{
    qCInfo(userController) << "Email availability result:" << available;
    emit emailAvailabilityResult(available);
}

void UserController::onEmailVerificationCodeSent(bool success, const QString &message)
{
    qCInfo(userController) << "Email verification code sent:" << success << message;
    emit emailVerificationCodeSent(success, message);
}

void UserController::onEmailVerificationCodeVerified(bool success, const QString &message)
{
    qCInfo(userController) << "Email verification code verified:" << success << message;
    emit emailVerificationCodeVerified(success, message);
}



void UserController::connectToServer(const QString &host, int port)
{
    qCInfo(userController) << "Connecting to server:" << host << ":" << port;
    if (!_networkClient) {
        _networkClient = new NetworkClient(this);
        connect(_networkClient, &NetworkClient::loginResponse, this, &UserController::onLoginResponse);
        connect(_networkClient, &NetworkClient::registerResponse, this, &UserController::onRegisterResponse);
        connect(_networkClient, &NetworkClient::connectionError, this, &UserController::onNetworkError);
    }
    _networkClient->connectToServer(host, port);
}