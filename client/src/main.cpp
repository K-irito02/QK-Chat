#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QFont>
#include <QFontDatabase>
#include <QLoggingCategory>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QUrl>
#include <QObject>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include "controllers/UserController.h"
#include "controllers/ChatController.h"
#include "models/UserModel.h"
#include "config/ConfigManager.h"
#include "network/NetworkClient.h"
#include "database/LocalDatabase.h"
#include "utils/Validator.h"
#include "utils/ThreadPool.h"
#include "utils/LogManager.h"

Q_DECLARE_LOGGING_CATEGORY(userController)
Q_DECLARE_LOGGING_CATEGORY(networkClient)

// 添加日志清理函数
void clearLogFiles()
{
    QString appDir = QCoreApplication::applicationDirPath();
    QString logDir = QDir(appDir).absoluteFilePath("../../../../logs/client");
    
    QDir dir(logDir);
    if (!dir.exists()) {
        qDebug() << "Log directory does not exist:" << logDir;
        return;
    }
    
    qDebug() << "Clearing log files in:" << logDir;
    
    int removedCount = 0;
    
    // 删除所有.log文件
    QStringList filters;
    filters << "*.log" << "*.log.*";
    QFileInfoList logFiles = dir.entryInfoList(filters, QDir::Files);
    
    for (const QFileInfo &fileInfo : logFiles) {
        if (QFile::remove(fileInfo.absoluteFilePath())) {
            qDebug() << "Removed log file:" << fileInfo.fileName();
            removedCount++;
        }
    }
    
    // 删除监控和诊断文件
    QStringList additionalFiles = {"metrics.json", "monitoring_metrics.json"};
    for (const QString &fileName : additionalFiles) {
        QString filePath = dir.absoluteFilePath(fileName);
        if (QFile::exists(filePath) && QFile::remove(filePath)) {
            qDebug() << "Removed file:" << fileName;
            removedCount++;
        }
    }
    
    // 删除诊断JSON文件
    QFileInfoList diagnosticFiles = dir.entryInfoList(QStringList() << "diagnostic_*.json", QDir::Files);
    for (const QFileInfo &fileInfo : diagnosticFiles) {
        if (QFile::remove(fileInfo.absoluteFilePath())) {
            qDebug() << "Removed diagnostic file:" << fileInfo.fileName();
            removedCount++;
        }
    }
    
    qDebug() << "Log cleanup completed. Removed" << removedCount << "files";
    
    // 记录清理操作到日志
    if (removedCount > 0) {
        LogManager::instance()->writeDiagnosticLog("LogCleanup", "Completed", 
            QString("Removed %1 log files").arg(removedCount));
    }
}

int main(int argc, char *argv[])
{
    // 设置环境变量以启用调试输出
    qputenv("QT_LOGGING_TO_CONSOLE", "1");
    qputenv("QT_DEBUG_PLUGINS", "1");
    
    QGuiApplication app(argc, argv);
    
    // 在程序启动时自动清理日志文件
    clearLogFiles();
    
    // 初始化日志管理器
    LogManager::instance();
    
    // 记录程序启动信息
    LogManager::instance()->writeDiagnosticLog("Application", "Started", 
        QString("QK Chat Client v%1 started").arg(app.applicationVersion()));
    
    // 配置调试输出
    QLoggingCategory::setFilterRules("qkchat.client.*=true");
    qSetMessagePattern("[%{time yyyy-MM-dd hh:mm:ss.zzz}] %{category}: %{message}");
    
    // 测试调试输出
    qCInfo(userController) << "QK Chat Client starting...";
    qCInfo(networkClient) << "Network client initialized";
    
    // 设置应用程序信息
    app.setApplicationName("QK Chat Client");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("QK Team");
    
    // 设置Quick Controls样式
    QQuickStyle::setStyle("Material");
    
    // 注册C++类型到QML
    qmlRegisterType<UserController>("QKChatClient", 1, 0, "UserController");
    qmlRegisterType<ChatController>("QKChatClient", 1, 0, "ChatController");
    qmlRegisterType<UserModel>("QKChatClient", 1, 0, "UserModel");
    qmlRegisterType<ConfigManager>("QKChatClient", 1, 0, "ConfigManager");
    qmlRegisterType<NetworkClient>("QKChatClient", 1, 0, "NetworkClient");
    qmlRegisterType<LocalDatabase>("QKChatClient", 1, 0, "LocalDatabase");
    qmlRegisterType<Validator>("QKChatClient", 1, 0, "Validator");
    

    
    QQmlApplicationEngine engine;
    
    // 创建全局对象
    auto userController = new UserController(&app);
    auto chatController = new ChatController(&app);
    auto configManager = new ConfigManager(&app);
    auto networkClient = new NetworkClient(&app);
    auto localDatabase = new LocalDatabase(&app);
    auto userModel = userController->userModel();
    auto threadPool = new ThreadPool(QThread::idealThreadCount(), &app);
    
    // 设置依赖关系
    chatController->setUserModel(userModel);
    chatController->setNetworkClient(networkClient);
    chatController->setLocalDatabase(localDatabase);
    chatController->setThreadPool(threadPool);
    userController->setThreadPool(threadPool);
    userController->setNetworkClient(networkClient);  // 添加这行
    userController->setDatabase(localDatabase);      // 添加这行
    
    // 初始化组件
    localDatabase->initialize();
    
    // 加载配置
    configManager->loadConfig();
    
    // 将C++对象暴露给QML
    engine.rootContext()->setContextProperty("userController", userController);
    engine.rootContext()->setContextProperty("chatController", chatController);
    engine.rootContext()->setContextProperty("configManager", configManager);
    engine.rootContext()->setContextProperty("networkClient", networkClient);
    engine.rootContext()->setContextProperty("localDatabase", localDatabase);
    engine.rootContext()->setContextProperty("userModel", userModel);
    
    // 加载主QML文件
    const QUrl url(QStringLiteral("qrc:/QKChatClient/qml/main.qml"));
    
    // 添加导入路径
    engine.addImportPath("qrc:/");
    engine.addImportPath("qrc:/QKChatClient");
    
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    
    engine.load(url);
    
    return app.exec();
}