#include <QApplication>
#include <QMessageBox>
#include <QDialog>
#include <QDir>
#include <QLoggingCategory>
#include <QSslSocket>
#include <QLibraryInfo>
#include <QCoreApplication>
#include <QTimer>
#include <QThread>
#include <QFuture>
#include <QtConcurrent>
#include <signal.h>
#include <iostream>

#include "admin/AdminWindow.h"
#include "admin/LoginDialog.h"
#include "core/ChatServer.h"
#include "config/ServerConfig.h"
#include "utils/LogManager.h"
#include "utils/StackTraceLogger.h"

Q_LOGGING_CATEGORY(server, "qkchat.server")

// 全局异常处理器
void globalExceptionHandler()
{
    std::cerr << "Unhandled exception occurred!" << std::endl;
    LogManager::instance()->writeErrorLog("Unhandled exception occurred", "ServerMain");
    QApplication::quit();
}

// 信号处理器
void signalHandler(int signal)
{
    QString signalName;
    switch (signal) {
        case SIGSEGV: signalName = "SIGSEGV"; break;
        case SIGABRT: signalName = "SIGABRT"; break;
        case SIGFPE: signalName = "SIGFPE"; break;
        default: signalName = QString("Signal %1").arg(signal); break;
    }
    
    std::cerr << "Received signal: " << signalName.toStdString() << std::endl;
    LogManager::instance()->writeErrorLog(QString("Received signal: %1").arg(signalName), "ServerMain");
    StackTraceLogger::instance().logStackTrace("SIGNAL_HANDLER", "ServerMain");
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
        qDebug() << "[ServerMain] Loading server configuration...";
        ServerConfig *config = ServerConfig::instance();
        if (!config->loadConfig()) {
            logManager->writeErrorLog("Failed to load server configuration", "ServerMain");
            QMessageBox::critical(nullptr, "配置错误", "无法加载服务器配置文件");
            return -1;
        }
        qDebug() << "[ServerMain] Server configuration loaded successfully";

        logManager->writeSystemLog("ServerMain", "CONFIG_LOADED", "Server configuration loaded successfully");
        
        // 显示管理员登录对话框
        qDebug() << "[ServerMain] Showing login dialog...";
        LoginDialog loginDialog;
        if (loginDialog.exec() != QDialog::Accepted) {
            logManager->writeSystemLog("ServerMain", "LOGIN_CANCELLED", "Administrator login cancelled, exiting");
            return 0;
        }
        qDebug() << "[ServerMain] Login successful";

        logManager->writeSystemLog("ServerMain", "LOGIN_SUCCESS", "Administrator login successful");
        
        // 创建并显示管理界面 - 添加超时保护
        qDebug() << "[ServerMain] Creating admin window...";
        AdminWindow *adminWindow = new AdminWindow();
        
        // 设置超时保护，防止界面创建阻塞
        QTimer::singleShot(10000, [adminWindow]() {
            if (adminWindow && !adminWindow->isVisible()) {
                qWarning() << "[ServerMain] Admin window creation timeout, forcing show";
                adminWindow->show();
            }
        });
        
        adminWindow->show();
        logManager->writeSystemLog("ServerMain", "ADMIN_WINDOW_CREATED", "Admin window created and shown");
        qDebug() << "[ServerMain] Admin window created and shown";

        // 启动聊天服务器 - 使用同步方式避免线程问题
        qDebug() << "[ServerMain] Creating ChatServer...";
        ChatServer *chatServer = new ChatServer(nullptr);
        logManager->writeSystemLog("ServerMain", "CHATSERVER_CREATED", "ChatServer instance created");

        // 将ChatServer连接到AdminWindow - 这是关键步骤！
        qDebug() << "[ServerMain] Connecting ChatServer to AdminWindow...";
        adminWindow->setChatServer(chatServer);
        logManager->writeSystemLog("ServerMain", "CHATSERVER_CONNECTED", "ChatServer connected to AdminWindow");

        // 连接数据库
        qDebug() << "[ServerMain] Connecting to database...";
        bool dbInitResult = chatServer->initializeDatabase();
        
        if (!dbInitResult) {
            logManager->writeErrorLog("Failed to connect to database", "ServerMain");
            QMessageBox::critical(adminWindow, "数据库错误", "无法连接数据库，请检查数据库连接配置");
            return -1;
        }
        qDebug() << "[ServerMain] Database connected successfully";

        // 使用同步方式启动服务器，避免线程问题
        qDebug() << "[ServerMain] Starting chat server synchronously...";
        bool serverStartResult = false;
        
        try {
            serverStartResult = chatServer->startServer();
        } catch (const std::exception& e) {
            qWarning() << "[ServerMain] Server startup exception:" << e.what();
            logManager->writeErrorLog(QString("Server startup exception: %1").arg(e.what()), "ServerMain");
            serverStartResult = false;
        } catch (...) {
            qWarning() << "[ServerMain] Unknown server startup exception";
            logManager->writeErrorLog("Unknown server startup exception", "ServerMain");
            serverStartResult = false;
        }

        if (!serverStartResult) {
            logManager->writeErrorLog("Failed to start chat server", "ServerMain");
            QMessageBox::critical(adminWindow, "服务器错误", "无法启动聊天服务器");
            return -1;
        }
        qDebug() << "[ServerMain] Chat server started successfully";

        // 服务器启动成功后，重新设置ChatServer给UI组件，确保能获取到正确的数据
        qDebug() << "[ServerMain] Reconnecting ChatServer to AdminWindow after server start...";
        adminWindow->setChatServer(chatServer);
        logManager->writeSystemLog("ServerMain", "CHATSERVER_RECONNECTED", "ChatServer reconnected to AdminWindow after server start");

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
        
        qDebug() << "[ServerMain] Entering application event loop...";
        int result = app.exec();
        
        qDebug() << "[ServerMain] Application exiting with code:" << result;
        logManager->writeSystemLog("ServerMain", "SHUTDOWN", "QK Chat Server shutting down");
        
        return result;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception in main: " << e.what() << std::endl;
        LogManager::instance()->writeErrorLog(QString("Exception in main: %1").arg(e.what()), "ServerMain");
        return -1;
    } catch (...) {
        std::cerr << "Unknown exception in main" << std::endl;
        LogManager::instance()->writeErrorLog("Unknown exception in main", "ServerMain");
        return -1;
    }
}