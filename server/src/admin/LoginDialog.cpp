#include "LoginDialog.h"
#include "ui_LoginDialog.h"
#include "../utils/AdminAuth.h"
#include <QMessageBox>
#include <QTimer>
#include <QApplication>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(loginDialog, "qkchat.server.admin.logindialog")

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LoginDialog)
    , _adminAuth(new AdminAuth(this))
    , _lockoutTimer(new QTimer(this))
    , _failedAttempts(0)
    , _isDarkTheme(false)
{
    ui->setupUi(this);
    
    setupUI();
    connectSignals();
    loadSettings();
    
    // 加载管理员配置
    _adminAuth->loadConfig();
    
    qCInfo(loginDialog) << "Login dialog initialized";
}

LoginDialog::~LoginDialog()
{
    delete ui;
}

void LoginDialog::setupUI()
{
    setWindowTitle("QK Chat 服务器管理");
    setFixedSize(400, 300);
    setModal(true);
    
    // 设置窗口图标
    setWindowIcon(QIcon(":/icons/server.png"));
    
    // 初始化UI组件
    ui->usernameLineEdit->setPlaceholderText("管理员用户名");
    ui->passwordLineEdit->setPlaceholderText("管理员密码");
    ui->passwordLineEdit->setEchoMode(QLineEdit::Password);
    
    // 设置按钮
    ui->loginButton->setText("登录");
    ui->loginButton->setDefault(true);
    ui->cancelButton->setText("取消");
    
    // 主题切换按钮
    ui->themeButton->setText(_isDarkTheme ? "🌞" : "🌙");
    ui->themeButton->setToolTip("切换主题");
    
    // 状态标签
    ui->statusLabel->setText("");
    ui->statusLabel->setStyleSheet("color: red;");
    
    // 记住密码复选框
    ui->rememberCheckBox->setText("记住密码");
    
    // 锁定状态标签（初始隐藏）
    ui->lockoutLabel->setVisible(false);
    ui->lockoutLabel->setStyleSheet("color: red; font-weight: bold;");
    
    updateTheme();
}

void LoginDialog::connectSignals()
{
    // 按钮信号
    connect(ui->loginButton, &QPushButton::clicked, this, &LoginDialog::onLoginClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui->themeButton, &QPushButton::clicked, this, &LoginDialog::onToggleTheme);
    
    // 回车键登录
    connect(ui->usernameLineEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    connect(ui->passwordLineEdit, &QLineEdit::returnPressed, this, &LoginDialog::onLoginClicked);
    
    // 管理员认证信号
    connect(_adminAuth, &AdminAuth::accountLocked, this, &LoginDialog::onAccountLocked);
    connect(_adminAuth, &AdminAuth::accountUnlocked, this, &LoginDialog::onAccountUnlocked);
    connect(_adminAuth, &AdminAuth::authenticationFailed, this, &LoginDialog::onAuthenticationFailed);
    
    // 锁定计时器
    _lockoutTimer->setSingleShot(false);
    _lockoutTimer->setInterval(1000); // 每秒更新
    connect(_lockoutTimer, &QTimer::timeout, this, &LoginDialog::updateLockoutStatus);
}

void LoginDialog::loadSettings()
{
    QSettings settings;
    
    // 加载主题设置
    _isDarkTheme = settings.value("admin/dark_theme", false).toBool();
    
    // 加载记住的用户名
    if (settings.value("admin/remember_password", false).toBool()) {
        ui->rememberCheckBox->setChecked(true);
        ui->usernameLineEdit->setText(settings.value("admin/username", "").toString());
        // 注意：密码不保存，仅保存用户名
    }
    
    // 检查是否有锁定状态
    if (_adminAuth->isAccountLocked()) {
        onAccountLocked();
    }
}

void LoginDialog::saveSettings()
{
    QSettings settings;
    
    // 保存主题设置
    settings.setValue("admin/dark_theme", _isDarkTheme);
    
    // 保存记住密码设置
    settings.setValue("admin/remember_password", ui->rememberCheckBox->isChecked());
    
    if (ui->rememberCheckBox->isChecked()) {
        settings.setValue("admin/username", ui->usernameLineEdit->text());
    } else {
        settings.remove("admin/username");
    }
}

void LoginDialog::onLoginClicked()
{
    QString username = ui->usernameLineEdit->text().trimmed();
    QString password = ui->passwordLineEdit->text();
    
    // 基本验证
    if (username.isEmpty() || password.isEmpty()) {
        showStatusMessage("请输入用户名和密码", true);
        return;
    }
    
    // 检查账户是否被锁定
    if (_adminAuth->isAccountLocked()) {
        int remainingTime = _adminAuth->getRemainingLockoutTime();
        showStatusMessage(QString("账户已被锁定，请等待 %1 秒").arg(remainingTime), true);
        return;
    }
    
    // 禁用登录按钮防止重复点击
    ui->loginButton->setEnabled(false);
    ui->loginButton->setText("登录中...");
    
    // 执行认证
    bool authResult = _adminAuth->authenticate(username, password);
    
    if (authResult) {
        // 认证成功
        saveSettings();
        showStatusMessage("登录成功", false);
        
        qCInfo(loginDialog) << "Admin login successful:" << username;
        
        // 短暂延迟后接受对话框
        QTimer::singleShot(500, this, &QDialog::accept);
    } else {
        // 认证失败
        _failedAttempts = _adminAuth->getFailedAttempts();
        
        // 清空密码字段
        ui->passwordLineEdit->clear();
        ui->passwordLineEdit->setFocus();
        
        // 重新启用登录按钮
        ui->loginButton->setEnabled(true);
        ui->loginButton->setText("登录");
        
        qCWarning(loginDialog) << "Admin login failed for user:" << username 
                              << "Attempts:" << _failedAttempts;
    }
}

void LoginDialog::onToggleTheme()
{
    _isDarkTheme = !_isDarkTheme;
    ui->themeButton->setText(_isDarkTheme ? "🌞" : "🌙");
    updateTheme();
}

void LoginDialog::onAccountLocked()
{
    ui->loginButton->setEnabled(false);
    ui->usernameLineEdit->setEnabled(false);
    ui->passwordLineEdit->setEnabled(false);
    
    ui->lockoutLabel->setVisible(true);
    
    // 启动倒计时计时器
    _lockoutTimer->start();
    updateLockoutStatus();
    
    showStatusMessage("账户已被锁定", true);
    
    qCWarning(loginDialog) << "Account locked due to too many failed attempts";
}

void LoginDialog::onAccountUnlocked()
{
    ui->loginButton->setEnabled(true);
    ui->usernameLineEdit->setEnabled(true);
    ui->passwordLineEdit->setEnabled(true);
    
    ui->lockoutLabel->setVisible(false);
    _lockoutTimer->stop();
    
    _failedAttempts = 0;
    showStatusMessage("账户已解锁", false);
    
    qCInfo(loginDialog) << "Account unlocked";
}

void LoginDialog::onAuthenticationFailed(const QString &reason)
{
    showStatusMessage(reason, true);
    
    // 显示剩余尝试次数
    int remainingAttempts = MAX_FAILED_ATTEMPTS - _adminAuth->getFailedAttempts();
    if (remainingAttempts > 0 && !_adminAuth->isAccountLocked()) {
        QString message = QString("剩余尝试次数: %1").arg(remainingAttempts);
        ui->statusLabel->setText(ui->statusLabel->text() + "\n" + message);
    }
}

void LoginDialog::updateLockoutStatus()
{
    if (!_adminAuth->isAccountLocked()) {
        onAccountUnlocked();
        return;
    }
    
    int remainingTime = _adminAuth->getRemainingLockoutTime();
    if (remainingTime <= 0) {
        _adminAuth->unlockAccount();
        return;
    }
    
    int minutes = remainingTime / 60;
    int seconds = remainingTime % 60;
    
    QString lockoutText = QString("账户锁定中，剩余时间: %1:%2")
                         .arg(minutes, 2, 10, QChar('0'))
                         .arg(seconds, 2, 10, QChar('0'));
    
    ui->lockoutLabel->setText(lockoutText);
}

void LoginDialog::showStatusMessage(const QString &message, bool isError)
{
    ui->statusLabel->setText(message);
    ui->statusLabel->setStyleSheet(isError ? "color: red;" : "color: green;");
    
    // 如果是成功消息，3秒后清空
    if (!isError) {
        QTimer::singleShot(3000, this, [this]() {
            ui->statusLabel->clear();
        });
    }
}

void LoginDialog::updateTheme()
{
    QString styleSheet;
    
    if (_isDarkTheme) {
        // 深色主题
        styleSheet = R"(
            QDialog {
                background-color: #2b2b2b;
                color: #ffffff;
            }
            QLineEdit {
                background-color: #3c3c3c;
                border: 1px solid #555555;
                border-radius: 4px;
                padding: 8px;
                color: #ffffff;
            }
            QLineEdit:focus {
                border: 2px solid #0078d4;
            }
            QPushButton {
                background-color: #0078d4;
                border: none;
                border-radius: 4px;
                padding: 8px 16px;
                color: #ffffff;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #106ebe;
            }
            QPushButton:pressed {
                background-color: #005a9e;
            }
            QPushButton:disabled {
                background-color: #555555;
                color: #999999;
            }
            QCheckBox {
                color: #ffffff;
            }
            QCheckBox::indicator {
                width: 16px;
                height: 16px;
            }
            QCheckBox::indicator:unchecked {
                background-color: #3c3c3c;
                border: 1px solid #555555;
            }
            QCheckBox::indicator:checked {
                background-color: #0078d4;
                border: 1px solid #0078d4;
            }
        )";
    } else {
        // 浅色主题
        styleSheet = R"(
            QDialog {
                background-color: #ffffff;
                color: #000000;
            }
            QLineEdit {
                background-color: #ffffff;
                border: 1px solid #cccccc;
                border-radius: 4px;
                padding: 8px;
                color: #000000;
            }
            QLineEdit:focus {
                border: 2px solid #0078d4;
            }
            QPushButton {
                background-color: #0078d4;
                border: none;
                border-radius: 4px;
                padding: 8px 16px;
                color: #ffffff;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #106ebe;
            }
            QPushButton:pressed {
                background-color: #005a9e;
            }
            QPushButton:disabled {
                background-color: #cccccc;
                color: #666666;
            }
            QCheckBox {
                color: #000000;
            }
            QCheckBox::indicator {
                width: 16px;
                height: 16px;
            }
            QCheckBox::indicator:unchecked {
                background-color: #ffffff;
                border: 1px solid #cccccc;
            }
            QCheckBox::indicator:checked {
                background-color: #0078d4;
                border: 1px solid #0078d4;
            }
        )";
    }
    
    setStyleSheet(styleSheet);
} 