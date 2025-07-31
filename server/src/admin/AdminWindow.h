#ifndef ADMINWINDOW_H
#define ADMINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QDateTime>
#include <QCloseEvent>
#include <QEvent>
#include <QWidget>
#include <QString>

QT_BEGIN_NAMESPACE
class QTabWidget;
class QStatusBar;
class QLabel;
class QAction;
class QMenu;
QT_END_NAMESPACE

class DashboardWidget;
// class UserManagerWidget;
// class SystemConfigWidget;
// class LogViewerWidget;
// class MonitorWidget;
class Database;
class ChatServer;

/**
 * @brief 服务器管理主窗口
 * 
 * 提供服务器管理功能，包括：
 * - 实时监控数据展示
 * - 用户管理
 * - 系统配置
 * - 日志查看
 */
class AdminWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit AdminWindow(QWidget *parent = nullptr);
    ~AdminWindow();
    
    void setChatServer(ChatServer *server);
    
protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    
private slots:
    void updateServerStatus();
    void onServerStarted();
    void onServerStopped();
    void onServerError(const QString &error);
    void showAbout();
    void toggleTheme();
    void onSystemTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void showWindow();
    void hideToTray();
    void exitApplication();
    
private:
    void setupUI();
    void setupMenuBar();
    void setupStatusBar();
    void setupSystemTray();
    void setupConnections();
    void updateTheme();
    void loadSettings();
    void saveSettings();
    
    // UI组件
    QTabWidget *_tabWidget;
    
    // 功能模块
    DashboardWidget *_dashboardWidget;
    // UserManagerWidget *_userManagerWidget;
    // SystemConfigWidget *_systemConfigWidget;
    // LogViewerWidget *_logViewerWidget;
    // MonitorWidget *_monitorWidget;
    
    // 菜单和工具栏
    QAction *_startServerAction;
    QAction *_stopServerAction;
    QAction *_restartServerAction;
    QAction *_exitAction;
    QAction *_aboutAction;
    QAction *_themeAction;
    QAction *_minimizeToTrayAction;
    
    // 状态栏
    QLabel *_serverStatusLabel;
    QLabel *_onlineUsersLabel;
    QLabel *_connectionCountLabel;
    QLabel *_uptimeLabel;
    
    // 系统托盘
    QSystemTrayIcon *_systemTray;
    QMenu *_trayMenu;
    
    // 计时器
    QTimer *_statusUpdateTimer;
    
    // 业务对象
    Database *_database;
    ChatServer *_chatServer;
    
    // 状态变量
    bool _isDarkTheme;
    QDateTime _serverStartTime;
    bool _isServerRunning;
};

#endif // ADMINWINDOW_H 