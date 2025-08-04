#include <QApplication>
#include <QMessageBox>
#include <QDialog>
#include <QDir>
#include <QLoggingCategory>
#include <QSslSocket>
#include <QLibraryInfo>
#include <QCoreApplication>
#include <QTimer>
#include <csignal>

#include "admin/AdminWindow.h"
#include "admin/LoginDialog.h"
#include "core/ChatServer.h"
#include "config/ServerConfig.h"
#include "utils/LogManager.h"

Q_LOGGING_CATEGORY(server, "qkchat.server")

// 全局异常处理器
void globalExceptionHandler()
{
    LogManager::instance()->writeErrorLog("Unhandled exception caught by global handler", "ServerMain");
    QApplication::quit();
}

// 信号处理器
void signalHandler(int signal)
{
    LogManager::instance()->writeErrorLog(QString("Signal received: %1").arg(signal), "ServerMain");
    QApplication::quit();
}


int main(int argc, char *argv[])
{
    try {
        QApplication app(argc, argv);
    
        // 设置全局异常处理器
        std::set_terminate(globalExceptionHandler);
        
        // 设置信号处理器
        signal(SIGSEGV, signalHandler);
        signal(SIGABRT, signalHandler);
        signal(SIGFPE, signalHandler);
        
        // 设置应用程序信息
        app.setApplicationName("QK Chat Server");
        app.setApplicationVersion("1.0.0");
        app.setOrganizationName("QK Team");
    
    // 初始化日志系统
    QLoggingCategory::setFilterRules("qkchat.*.debug=true");

    // 初始化LogManager并清空旧日志
    LogManager *logManager = LogManager::instance();
    logManager->clearLogsOnStartup();
    logManager->writeSystemLog("ServerMain", "STARTUP", "QK Chat Server starting up");

    // 加载服务器配置
    ServerConfig *config = ServerConfig::instance();
    if (!config->loadConfig()) {
        logManager->writeErrorLog("Failed to load server configuration", "ServerMain");
        QMessageBox::critical(nullptr, "配置错误", "无法加载服务器配置文件");
        return -1;
    }

    logManager->writeSystemLog("ServerMain", "CONFIG_LOADED", "Server configuration loaded successfully");
    
    // 显示管理员登录对话框
    LoginDialog loginDialog;
    if (loginDialog.exec() != QDialog::Accepted) {
        logManager->writeSystemLog("ServerMain", "LOGIN_CANCELLED", "Administrator login cancelled, exiting");
        return 0;
    }

    logManager->writeSystemLog("ServerMain", "LOGIN_SUCCESS", "Administrator login successful");
    
    // 创建并显示管理界面
    AdminWindow *adminWindow = new AdminWindow();
    adminWindow->show();
    logManager->writeSystemLog("ServerMain", "ADMIN_WINDOW_CREATED", "Admin window created and shown");

    // 启动聊天服务器
    // 将 chatServer 的父对象设置为 nullptr，手动管理其生命周期
    ChatServer *chatServer = new ChatServer(nullptr);
    logManager->writeSystemLog("ServerMain", "CHATSERVER_CREATED", "ChatServer instance created");

    // 将ChatServer连接到AdminWindow - 这是关键步骤！
    adminWindow->setChatServer(chatServer);
    logManager->writeSystemLog("ServerMain", "CHATSERVER_CONNECTED", "ChatServer connected to AdminWindow");

    // 确保数据库已正确初始化
    if (!chatServer->initializeDatabase()) {
        logManager->writeErrorLog("Failed to initialize database", "ServerMain");
        QMessageBox::critical(adminWindow, "数据库错误", "无法初始化数据库，请检查数据库连接配置");
        return -1;
    }

    if (!chatServer->startServer()) {
        logManager->writeErrorLog("Failed to start chat server", "ServerMain");
        QMessageBox::critical(adminWindow, "服务器错误", "无法启动聊天服务器");
        return -1;
    }

    logManager->writeSystemLog("ServerMain", "STARTUP_COMPLETE",
                             QString("QK Chat Server started successfully - Admin Port: %1, Server Port: %2")
                             .arg(config->getAdminPort()).arg(config->getServerPort()));

    // 连接 AdminWindow 的关闭事件来安全地停止服务器
    QObject::connect(adminWindow, &AdminWindow::destroyed,
                     chatServer, &ChatServer::stopServer);

    // 连接服务器停止信号到 chatServer 的删除
    QObject::connect(chatServer, &ChatServer::serverStopped,
                     chatServer, &QObject::deleteLater);

    // 连接服务器停止信号到应用程序的退出
    QObject::connect(chatServer, &ChatServer::serverStopped, &app, &QCoreApplication::quit);
    
    // 连接应用程序退出信号到AdminWindow的删除
    QObject::connect(&app, &QCoreApplication::aboutToQuit, adminWindow, &QObject::deleteLater);
    
        int result = app.exec();

        qCInfo(server) << "Application finished with exit code:" << result;
        LogManager::instance()->writeSystemLog("ServerMain", "SHUTDOWN",
                                             QString("Application finished with exit code: %1").arg(result));

        return result;

    } catch (const std::exception& e) {
        LogManager::instance()->writeErrorLog(QString("Fatal exception in main: %1").arg(e.what()), "ServerMain");
        QMessageBox::critical(nullptr, "致命错误", QString("应用程序遇到致命错误：%1").arg(e.what()));
        return -1;
    } catch (...) {
        LogManager::instance()->writeErrorLog("Unknown fatal exception in main", "ServerMain");
        QMessageBox::critical(nullptr, "致命错误", "应用程序遇到未知的致命错误");
        return -1;
    }
}