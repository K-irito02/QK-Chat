#include "AdminWindow.h"
#include "DashboardWidget.h"
// #include "UserManagerWidget.h"
// #include "SystemConfigWidget.h"
// #include "LogViewerWidget.h"
// #include "MonitorWidget.h"
#include "../database/Database.h"
#include "../core/ChatServer.h"
#include "../config/ServerConfig.h"
#include "../utils/LogManager.h"
#include "../utils/StackTraceLogger.h"

#include <QTabWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QAction>
#include <QMenu>
#include <QApplication>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QLoggingCategory>
#include <QMutex>
#include <QMutexLocker>
#include <QDateTime>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QIcon>
#include <QKeySequence>
#include <QEvent>
#include <QThread>

Q_LOGGING_CATEGORY(adminWindow, "qkchat.server.admin.adminwindow")

AdminWindow::AdminWindow(QWidget *parent)
    : QMainWindow(parent)
    , _tabWidget(nullptr)
    , _isDarkTheme(false)
    , _isServerRunning(false)
{
    setupUI();
    setupMenuBar();
    setupStatusBar();
    setupSystemTray();
    setupConnections();
    
    // 移除数据库初始化，避免重复初始化
    // 数据库将由ChatServer统一管理
    // _database = new Database(this);
    // _database->initialize();
    
    // 加载设置
    loadSettings();
    updateTheme();
    
    // 启动状态更新计时器
    _statusUpdateTimer = new QTimer(this);
    _statusUpdateTimer->setInterval(1000); // 1秒更新一次，确保运行时间实时更新
    connect(_statusUpdateTimer, &QTimer::timeout, this, &AdminWindow::updateServerStatus);
    _statusUpdateTimer->start();
    
    qCInfo(adminWindow) << "Admin window initialized";
}

AdminWindow::~AdminWindow()
{
    saveSettings();
}

void AdminWindow::setChatServer(ChatServer* chatServer)
{
    try {
        qCInfo(adminWindow) << "Setting ChatServer for AdminWindow";
        
        // 确保在主线程中执行
        if (QThread::currentThread() != qApp->thread()) {
            qCWarning(adminWindow) << "setChatServer called from non-main thread, moving to main thread";
            QMetaObject::invokeMethod(this, [this, chatServer]() {
                setChatServer(chatServer);
            }, Qt::QueuedConnection);
            return;
        }
        
        _chatServer = chatServer;
        
        if (_chatServer) {
            // 检查服务器是否正在运行
            bool serverRunning = false;
            try {
                serverRunning = _chatServer->isRunning();
            } catch (const std::exception& e) {
                qCWarning(adminWindow) << "Exception checking server running status:" << e.what();
                serverRunning = false;
            } catch (...) {
                qCWarning(adminWindow) << "Unknown exception checking server running status";
                serverRunning = false;
            }
            
            if (serverRunning) {
                qCInfo(adminWindow) << "Server is running, setting up UI immediately";
                
                // 设置DashboardWidget的ChatServer
                if (_dashboardWidget) {
                    _dashboardWidget->setChatServer(_chatServer);
                    qCInfo(adminWindow) << "ChatServer set for DashboardWidget";
                }
                
                // 立即更新一次服务器状态
                updateServerStatus();
                
                // 启动服务器状态更新定时器
                if (_statusUpdateTimer) {
                    _statusUpdateTimer->start(1000); // 每秒更新一次
                }
            } else {
                qCInfo(adminWindow) << "Server is not running yet, will setup UI later";
                
                // 设置默认状态
                if (_serverStatusLabel) {
                    _serverStatusLabel->setText("服务器状态: 启动中");
                    _serverStatusLabel->setStyleSheet("color: orange; font-weight: bold;");
                }
                if (_onlineUsersLabel) _onlineUsersLabel->setText("在线用户: 0");
                if (_connectionCountLabel) _connectionCountLabel->setText("连接数: 0");
                if (_uptimeLabel) _uptimeLabel->setText("运行时间: 00:00:00");
                
                // 延迟设置DashboardWidget，等待服务器启动
                QTimer::singleShot(2000, this, [this]() {
                    if (_dashboardWidget && _chatServer) {
                        _dashboardWidget->setChatServer(_chatServer);
                        qCInfo(adminWindow) << "ChatServer set for DashboardWidget (delayed)";
                    }
                });
                
                // 延迟启动状态更新定时器
                QTimer::singleShot(3000, this, [this]() {
                    if (_statusUpdateTimer) {
                        _statusUpdateTimer->start(1000); // 每秒更新一次
                    }
                });
            }
        } else {
            qCWarning(adminWindow) << "ChatServer is null";
        }
        
    } catch (const std::exception& e) {
        qCWarning(adminWindow) << "Exception in setChatServer:" << e.what();
    } catch (...) {
        qCWarning(adminWindow) << "Unknown exception in setChatServer";
    }
}

void AdminWindow::setupUI()
{
    setWindowTitle("QK Chat 服务器管理");
    setMinimumSize(1200, 800);
    resize(1400, 900);
    
    // 创建中央标签页组件
    _tabWidget = new QTabWidget(this);
    setCentralWidget(_tabWidget);
    
    // 创建各个功能模块
    _dashboardWidget = new DashboardWidget(this);
    // _userManagerWidget = new UserManagerWidget(this);
    // _systemConfigWidget = new SystemConfigWidget(this);
    // _logViewerWidget = new LogViewerWidget(this);
    // _monitorWidget = new MonitorWidget(this);
    
    // 添加标签页
    _tabWidget->addTab(_dashboardWidget, "📊 仪表板");
    // _tabWidget->addTab(_monitorWidget, "📈 实时监控");
    // _tabWidget->addTab(_userManagerWidget, "👥 用户管理");
    // _tabWidget->addTab(_systemConfigWidget, "⚙️ 系统配置");
    // _tabWidget->addTab(_logViewerWidget, "📋 日志查看");
    
    // 设置标签页样式
    _tabWidget->setTabPosition(QTabWidget::North);
    _tabWidget->setUsesScrollButtons(true);
    _tabWidget->setElideMode(Qt::ElideNone);
}

void AdminWindow::setupMenuBar()
{
    // 服务器菜单
    QMenu *serverMenu = menuBar()->addMenu("服务器(&S)");
    
    _startServerAction = new QAction("启动服务器(&S)", this);
    _startServerAction->setShortcut(QKeySequence("Ctrl+S"));
    _startServerAction->setIcon(QIcon(":/icons/start.png"));
    connect(_startServerAction, &QAction::triggered, this, [this]() {
        if (_chatServer) {
            _chatServer->startServer();
        }
    });
    
    _stopServerAction = new QAction("停止服务器(&T)", this);
    _stopServerAction->setShortcut(QKeySequence("Ctrl+T"));
    _stopServerAction->setIcon(QIcon(":/icons/stop.png"));
    _stopServerAction->setEnabled(false);
    connect(_stopServerAction, &QAction::triggered, this, [this]() {
        if (_chatServer) {
            _chatServer->stopServer();
        }
    });
    
    _restartServerAction = new QAction("重启服务器(&R)", this);
    _restartServerAction->setShortcut(QKeySequence("Ctrl+R"));
    _restartServerAction->setIcon(QIcon(":/icons/restart.png"));
    _restartServerAction->setEnabled(false);
    connect(_restartServerAction, &QAction::triggered, this, [this]() {
        if (_chatServer) {
            _chatServer->restartServer();
        }
    });
    
    serverMenu->addAction(_startServerAction);
    serverMenu->addAction(_stopServerAction);
    serverMenu->addAction(_restartServerAction);
    serverMenu->addSeparator();
    
    _minimizeToTrayAction = new QAction("最小化到托盘(&M)", this);
    _minimizeToTrayAction->setShortcut(QKeySequence("Ctrl+M"));
    connect(_minimizeToTrayAction, &QAction::triggered, this, &AdminWindow::hideToTray);
    serverMenu->addAction(_minimizeToTrayAction);
    
    serverMenu->addSeparator();
    
    _exitAction = new QAction("退出(&X)", this);
    _exitAction->setShortcut(QKeySequence("Ctrl+Q"));
    _exitAction->setIcon(QIcon(":/icons/exit.png"));
    connect(_exitAction, &QAction::triggered, this, &AdminWindow::exitApplication);
    serverMenu->addAction(_exitAction);
    
    // 视图菜单
    QMenu *viewMenu = menuBar()->addMenu("视图(&V)");
    
    _themeAction = new QAction("切换主题(&T)", this);
    _themeAction->setShortcut(QKeySequence("Ctrl+Shift+T"));
    connect(_themeAction, &QAction::triggered, this, &AdminWindow::toggleTheme);
    viewMenu->addAction(_themeAction);
    
    // 帮助菜单
    QMenu *helpMenu = menuBar()->addMenu("帮助(&H)");
    
    _aboutAction = new QAction("关于(&A)", this);
    _aboutAction->setIcon(QIcon(":/icons/about.png"));
    connect(_aboutAction, &QAction::triggered, this, &AdminWindow::showAbout);
    helpMenu->addAction(_aboutAction);
}

void AdminWindow::setupStatusBar()
{
    _serverStatusLabel = new QLabel("服务器状态: 未启动");
    _onlineUsersLabel = new QLabel("在线用户: 0");
    _connectionCountLabel = new QLabel("连接数: 0");
    _uptimeLabel = new QLabel("运行时间: 00:00:00");
    
    // 设置状态栏样式，避免重叠
    statusBar()->setStyleSheet("QStatusBar { border: none; background-color: #f0f0f0; }");
    
    // 添加服务器状态到左侧
    statusBar()->addWidget(_serverStatusLabel);
    
    // 添加永久部件到右侧，使用固定宽度避免重叠
    _uptimeLabel->setMinimumWidth(150);
    _uptimeLabel->setMaximumWidth(150);
    _connectionCountLabel->setMinimumWidth(100);
    _connectionCountLabel->setMaximumWidth(100);
    _onlineUsersLabel->setMinimumWidth(120);
    _onlineUsersLabel->setMaximumWidth(120);
    
    // 设置文本对齐
    _uptimeLabel->setAlignment(Qt::AlignRight);
    _connectionCountLabel->setAlignment(Qt::AlignRight);
    _onlineUsersLabel->setAlignment(Qt::AlignRight);
    
    statusBar()->addPermanentWidget(_uptimeLabel);
    statusBar()->addPermanentWidget(_connectionCountLabel);
    statusBar()->addPermanentWidget(_onlineUsersLabel);
    
    // 删除状态栏消息，避免左下角显示黑色文字
    // statusBar()->showMessage("QK Chat 服务器管理系统已就绪");
}

void AdminWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }
    
    _systemTray = new QSystemTrayIcon(this);
    _systemTray->setIcon(QIcon(":/icons/server.png"));
    _systemTray->setToolTip("QK Chat 服务器");
    
    // 创建托盘菜单
    _trayMenu = new QMenu(this);
    
    QAction *showAction = new QAction("显示主窗口", this);
    connect(showAction, &QAction::triggered, this, &AdminWindow::showWindow);
    _trayMenu->addAction(showAction);
    
    _trayMenu->addSeparator();
    _trayMenu->addAction(_startServerAction);
    _trayMenu->addAction(_stopServerAction);
    _trayMenu->addAction(_restartServerAction);
    
    _trayMenu->addSeparator();
    QAction *exitAction = new QAction("退出", this);
    connect(exitAction, &QAction::triggered, this, &AdminWindow::exitApplication);
    _trayMenu->addAction(exitAction);
    
    _systemTray->setContextMenu(_trayMenu);
    _systemTray->show();
    
    connect(_systemTray, &QSystemTrayIcon::activated, 
            this, &AdminWindow::onSystemTrayActivated);
}

void AdminWindow::setupConnections()
{
    // 标签页切换时更新数据
    connect(_tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        QWidget *currentWidget = _tabWidget->widget(index);
        
        if (currentWidget == _dashboardWidget) {
            _dashboardWidget->refreshData();
        // } else if (currentWidget == _userManagerWidget) {
        //     _userManagerWidget->refreshUserList();
        // } else if (currentWidget == _logViewerWidget) {
        //     _logViewerWidget->refreshLogs();
        // } else if (currentWidget == _monitorWidget) {
        //     _monitorWidget->startMonitoring();
        }
    });
}

void AdminWindow::updateServerStatus()
{
    try {
        // 确保在主线程中执行
        if (QThread::currentThread() != qApp->thread()) {
            qCWarning(adminWindow) << "updateServerStatus called from non-main thread, moving to main thread";
            QMetaObject::invokeMethod(this, "updateServerStatus", Qt::QueuedConnection);
            return;
        }
        
        if (!_chatServer) {
            qCWarning(adminWindow) << "ChatServer object is null";
            if (_serverStatusLabel) {
                _serverStatusLabel->setText("服务器状态: 未初始化");
                _serverStatusLabel->setStyleSheet("color: orange;");
            }
            if (_onlineUsersLabel) _onlineUsersLabel->setText("在线用户: 0");
            if (_connectionCountLabel) _connectionCountLabel->setText("连接数: 0");
            if (_uptimeLabel) _uptimeLabel->setText("运行时间: 00:00:00");
            return;
        }

        // 获取服务器运行状态 - 使用更可靠的方法
        bool isRunning = false;
        try {
            isRunning = _chatServer->isRunning();
        } catch (const std::exception& e) {
            qCWarning(adminWindow) << "Exception checking server running status:" << e.what();
            isRunning = false;
        } catch (...) {
            qCWarning(adminWindow) << "Unknown exception checking server running status";
            isRunning = false;
        }

        // 更新服务器状态显示
        if (isRunning) {
            if (_serverStatusLabel) {
                _serverStatusLabel->setText("服务器状态: 运行中");
                _serverStatusLabel->setStyleSheet("color: green; font-weight: bold;");
            }
            if (_startServerAction) _startServerAction->setEnabled(false);
            if (_stopServerAction) _stopServerAction->setEnabled(true);
            if (_restartServerAction) _restartServerAction->setEnabled(true);
        } else {
            if (_serverStatusLabel) {
                _serverStatusLabel->setText("服务器状态: 未启动");
                _serverStatusLabel->setStyleSheet("color: red; font-weight: bold;");
            }
            if (_startServerAction) _startServerAction->setEnabled(true);
            if (_stopServerAction) _stopServerAction->setEnabled(false);
            if (_restartServerAction) _restartServerAction->setEnabled(false);
        }

        // 更新缓存状态
        _isServerRunning = isRunning;
        
        if (isRunning) {
            // 更新在线用户数
            int onlineUsers = 0;
            try {
                onlineUsers = _chatServer->getOnlineUserCount();
            } catch (const std::exception& e) {
                qCWarning(adminWindow) << "Exception getting online user count:" << e.what();
                onlineUsers = 0;
            } catch (...) {
                qCWarning(adminWindow) << "Unknown exception getting online user count";
                onlineUsers = 0;
            }
            
            if (_onlineUsersLabel) {
                _onlineUsersLabel->setText(QString("在线用户: %1").arg(onlineUsers));
            }
            
            // 更新连接数
            int connections = 0;
            try {
                connections = _chatServer->getConnectionCount();
            } catch (const std::exception& e) {
                qCWarning(adminWindow) << "Exception getting connection count:" << e.what();
                connections = 0;
            } catch (...) {
                qCWarning(adminWindow) << "Unknown exception getting connection count";
                connections = 0;
            }
            
            if (_connectionCountLabel) {
                _connectionCountLabel->setText(QString("连接数: %1").arg(connections));
            }
            
            // 更新运行时间 - 直接从ChatServer获取，确保实时性
            if (_uptimeLabel) {
                try {
                    QString uptime = _chatServer->getUptime();
                    _uptimeLabel->setText(QString("运行时间: %1").arg(uptime));
                } catch (const std::exception& e) {
                    qCWarning(adminWindow) << "Exception getting uptime:" << e.what();
                    // 如果获取失败，使用本地计算的时间作为备用
                    if (!_serverStartTime.isNull()) {
                        qint64 seconds = _serverStartTime.secsTo(QDateTime::currentDateTime());
                        int hours = seconds / 3600;
                        int minutes = (seconds % 3600) / 60;
                        int secs = seconds % 60;

                        _uptimeLabel->setText(QString("运行时间: %1:%2:%3")
                                             .arg(hours, 2, 10, QChar('0'))
                                             .arg(minutes, 2, 10, QChar('0'))
                                             .arg(secs, 2, 10, QChar('0')));
                    } else {
                        _uptimeLabel->setText("运行时间: 00:00:00");
                    }
                } catch (...) {
                    qCWarning(adminWindow) << "Unknown exception getting uptime";
                    _uptimeLabel->setText("运行时间: 00:00:00");
                }
            }
        } else {
            if (_onlineUsersLabel) _onlineUsersLabel->setText("在线用户: 0");
            if (_connectionCountLabel) _connectionCountLabel->setText("连接数: 0");
            if (_uptimeLabel) _uptimeLabel->setText("运行时间: 00:00:00");
        }
    } catch (const std::exception& e) {
        qCWarning(adminWindow) << "Exception in updateServerStatus:" << e.what();
    } catch (...) {
        qCWarning(adminWindow) << "Unknown exception in updateServerStatus";
    }
}

void AdminWindow::onServerStarted()
{
    // 如果服务器启动时间还没有设置，使用当前时间
    if (_serverStartTime.isNull()) {
        _serverStartTime = QDateTime::currentDateTime();
    }
    _isServerRunning = true;

    // 启动DashboardWidget的统计信息更新
    if (_dashboardWidget) {
        _dashboardWidget->setChatServer(_chatServer);
    }

    // 立即更新一次服务器状态显示
    updateServerStatus();

    statusBar()->showMessage("服务器启动成功", 3000);

    if (_systemTray) {
        _systemTray->showMessage("QK Chat 服务器", "服务器启动成功",
                                QSystemTrayIcon::Information, 3000);
    }

    qCInfo(adminWindow) << "Server started successfully";
}

void AdminWindow::onServerStopped()
{
    _isServerRunning = false;
    _serverStartTime = QDateTime();
    
    statusBar()->showMessage("服务器已停止", 3000);
    
    if (_systemTray) {
        _systemTray->showMessage("QK Chat 服务器", "服务器已停止", 
                                QSystemTrayIcon::Warning, 3000);
    }
    
    qCInfo(adminWindow) << "Server stopped";
}

void AdminWindow::onServerError(const QString &error)
{
    statusBar()->showMessage("服务器错误: " + error, 5000);
    
    QMessageBox::critical(this, "服务器错误", error);
    
    if (_systemTray) {
        _systemTray->showMessage("QK Chat 服务器", "服务器错误: " + error, 
                                QSystemTrayIcon::Critical, 5000);
    }
    
    qCCritical(adminWindow) << "Server error:" << error;
}

void AdminWindow::showAbout()
{
    QMessageBox::about(this, "关于 QK Chat 服务器", 
                      "QK Chat 服务器管理系统\n\n"
                      "版本: 1.0.0\n"
                      "基于 Qt 6 框架开发\n\n"
                      "功能特性:\n"
                      "• 实时监控服务器状态\n"
                      "• 用户管理和权限控制\n"
                      "• 系统配置和日志查看\n"
                      "• 现代化管理界面\n\n"
                      "Copyright © 2024 QK Team");
}

void AdminWindow::toggleTheme()
{
    _isDarkTheme = !_isDarkTheme;
    updateTheme();
    saveSettings();
}

void AdminWindow::updateTheme()
{
    QString styleSheet;
    
    if (_isDarkTheme) {
        // 深色主题
        styleSheet = R"(
            QMainWindow {
                background-color: #2b2b2b;
                color: #ffffff;
            }
            QTabWidget::pane {
                border: 1px solid #555555;
                background-color: #3c3c3c;
            }
            QTabWidget::tab-bar {
                alignment: center;
            }
            QTabBar::tab {
                background-color: #555555;
                color: #ffffff;
                padding: 8px 16px;
                margin: 2px;
                border-radius: 4px;
            }
            QTabBar::tab:selected {
                background-color: #0078d4;
            }
            QTabBar::tab:hover {
                background-color: #666666;
            }
            QMenuBar {
                background-color: #3c3c3c;
                color: #ffffff;
                border-bottom: 1px solid #555555;
            }
            QMenuBar::item {
                background-color: transparent;
                padding: 4px 8px;
            }
            QMenuBar::item:selected {
                background-color: #0078d4;
                border-radius: 4px;
            }
            QMenu {
                background-color: #3c3c3c;
                color: #ffffff;
                border: 1px solid #555555;
            }
            QMenu::item {
                padding: 6px 16px;
            }
            QMenu::item:selected {
                background-color: #0078d4;
            }
            QStatusBar {
                background-color: #3c3c3c;
                color: #ffffff;
                border-top: 1px solid #555555;
            }
        )";
        
        _themeAction->setText("切换到浅色主题");
    } else {
        // 浅色主题
        styleSheet = R"(
            QMainWindow {
                background-color: #ffffff;
                color: #000000;
            }
            QTabWidget::pane {
                border: 1px solid #cccccc;
                background-color: #ffffff;
            }
            QTabBar::tab {
                background-color: #f0f0f0;
                color: #000000;
                padding: 8px 16px;
                margin: 2px;
                border-radius: 4px;
                border: 1px solid #cccccc;
            }
            QTabBar::tab:selected {
                background-color: #0078d4;
                color: #ffffff;
            }
            QTabBar::tab:hover {
                background-color: #e5e5e5;
            }
            QMenuBar {
                background-color: #f8f9fa;
                color: #000000;
                border-bottom: 1px solid #dee2e6;
            }
            QMenuBar::item {
                background-color: transparent;
                padding: 4px 8px;
            }
            QMenuBar::item:selected {
                background-color: #0078d4;
                color: #ffffff;
                border-radius: 4px;
            }
            QMenu {
                background-color: #ffffff;
                color: #000000;
                border: 1px solid #cccccc;
            }
            QMenu::item {
                padding: 6px 16px;
            }
            QMenu::item:selected {
                background-color: #0078d4;
                color: #ffffff;
            }
            QStatusBar {
                background-color: #f8f9fa;
                color: #000000;
                border-top: 1px solid #dee2e6;
            }
        )";
        
        _themeAction->setText("切换到深色主题");
    }
    
    setStyleSheet(styleSheet);
    
    // 更新各个子组件的主题
    try {
        if (_dashboardWidget) {
            _dashboardWidget->updateTheme(_isDarkTheme);
        }
        // _userManagerWidget->updateTheme(_isDarkTheme);
        // _systemConfigWidget->updateTheme(_isDarkTheme);
        // _logViewerWidget->updateTheme(_isDarkTheme);
        // _monitorWidget->updateTheme(_isDarkTheme);
    } catch (const std::exception& e) {
        qCWarning(adminWindow) << "Exception in updateTheme:" << e.what();
    } catch (...) {
        qCWarning(adminWindow) << "Unknown exception in updateTheme";
    }
}

void AdminWindow::onSystemTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        showWindow();
    }
}

void AdminWindow::showWindow()
{
    show();
    raise();
    activateWindow();
}

void AdminWindow::hideToTray()
{
    if (QSystemTrayIcon::isSystemTrayAvailable()) {
        hide();
        if (_systemTray) {
            _systemTray->showMessage("QK Chat 服务器", "应用程序已最小化到系统托盘", 
                                    QSystemTrayIcon::Information, 2000);
        }
    }
}

void AdminWindow::exitApplication()
{
    if (_chatServer && _chatServer->isRunning()) {
        int ret = QMessageBox::question(this, "确认退出", 
                                       "服务器正在运行中，确定要退出吗？",
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);
        
        if (ret == QMessageBox::No) {
            return;
        }
        
        // 停止服务器
        _chatServer->stopServer();
        
        // 等待服务器停止
        QTimer::singleShot(1000, [this]() {
            saveSettings();
            QApplication::quit();
        });
    } else {
        saveSettings();
        QApplication::quit();
    }
}

void AdminWindow::loadSettings()
{
    QSettings settings;
    
    // 加载窗口几何信息
    restoreGeometry(settings.value("admin/geometry").toByteArray());
    restoreState(settings.value("admin/windowState").toByteArray());
    
    // 加载主题设置
    _isDarkTheme = settings.value("admin/dark_theme", false).toBool();
    
    // 加载当前标签页
    int currentTab = settings.value("admin/current_tab", 0).toInt();
    if (currentTab >= 0 && currentTab < _tabWidget->count()) {
        _tabWidget->setCurrentIndex(currentTab);
    }
}

void AdminWindow::saveSettings()
{
    QSettings settings;
    
    // 保存窗口几何信息
    settings.setValue("admin/geometry", saveGeometry());
    settings.setValue("admin/windowState", saveState());
    
    // 保存主题设置
    settings.setValue("admin/dark_theme", _isDarkTheme);
    
    // 保存当前标签页
    settings.setValue("admin/current_tab", _tabWidget->currentIndex());
}

void AdminWindow::closeEvent(QCloseEvent *event)
{
    if (QSystemTrayIcon::isSystemTrayAvailable() && _systemTray) {
        hideToTray();
        event->ignore();
    } else {
        // 询问用户是否确定要退出
        int ret = QMessageBox::question(this, "确认退出", 
                                       "确定要退出QK Chat服务器吗？",
                                       QMessageBox::Yes | QMessageBox::No,
                                       QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            exitApplication();
            event->accept();
        } else {
            event->ignore();
        }
    }
}

void AdminWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized() && QSystemTrayIcon::isSystemTrayAvailable()) {
            hideToTray();
        }
    }
    
    QMainWindow::changeEvent(event);
} 