#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QLoggingCategory>

#include "admin/AdminWindow.h"
#include "admin/LoginDialog.h"
#include "core/ChatServer.h"
#include "config/ServerConfig.h"

Q_LOGGING_CATEGORY(server, "qkchat.server")

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("QK Chat Server");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("QK Team");
    
    // 初始化日志系统
    QLoggingCategory::setFilterRules("qkchat.*.debug=true");
    
    // 加载服务器配置
    ServerConfig *config = ServerConfig::instance();
    if (!config->loadConfig()) {
        QMessageBox::critical(nullptr, "配置错误", "无法加载服务器配置文件");
        return -1;
    }
    
    // 显示管理员登录对话框
    LoginDialog loginDialog;
    if (loginDialog.exec() != QDialog::Accepted) {
        qCInfo(server) << "管理员登录取消，程序退出";
        return 0;
    }
    
    // 创建并显示管理界面
    AdminWindow adminWindow;
    adminWindow.show();
    
    // 启动聊天服务器
    ChatServer *chatServer = new ChatServer(&app);
    if (!chatServer->startServer()) {
        QMessageBox::critical(&adminWindow, "服务器错误", "无法启动聊天服务器");
        return -1;
    }
    
    qCInfo(server) << "QK Chat Server 启动成功";
    qCInfo(server) << "管理界面端口:" << config->getAdminPort();
    qCInfo(server) << "聊天服务器端口:" << config->getServerPort();
    
    return app.exec();
} 