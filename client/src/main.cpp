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

#include "controllers/UserController.h"
#include "controllers/ChatController.h"
#include "models/UserModel.h"
#include "config/ConfigManager.h"
#include "network/NetworkClient.h"
#include "database/LocalDatabase.h"
#include "utils/Validator.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    
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
    
    // 设置依赖关系
    chatController->setUserModel(userModel);
    chatController->setNetworkClient(networkClient);
    chatController->setLocalDatabase(localDatabase);
    
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