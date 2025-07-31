#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QTimer>
#include <QDateTime>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class LoginDialog; }
QT_END_NAMESPACE

class AdminAuth;

/**
 * @brief 管理员登录对话框
 * 
 * 提供管理员身份验证功能，包括：
 * - 用户名密码验证
 * - 防暴力破解机制（5次错误后锁定30分钟）
 * - 记住登录状态
 * - 主题切换功能
 */
class LoginDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit LoginDialog(QWidget *parent = nullptr);
    ~LoginDialog();
    
private slots:
    void onLoginClicked();
    void onToggleTheme();
    void onAccountLocked();
    void onAccountUnlocked();
    void onAuthenticationFailed(const QString &reason);
    void updateLockoutStatus();

private:
    void setupUI();
    void connectSignals();
    void loadSettings();
    void saveSettings();
    void showStatusMessage(const QString &message, bool isError);
    void updateTheme();
    
    Ui::LoginDialog *ui;
    AdminAuth *_adminAuth;
    QTimer *_lockoutTimer;
    int _failedAttempts;
    QDateTime _lockoutTime;
    bool _isDarkTheme;
    
    static const int MAX_FAILED_ATTEMPTS = 5;
    static const int LOCKOUT_DURATION_MINUTES = 30;
};

#endif // LOGINDIALOG_H 