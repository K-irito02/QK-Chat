#include "QSslServer.h"
#include <QSslSocket>
#include <QLoggingCategory>
#include "../utils/LogManager.h"

Q_LOGGING_CATEGORY(sslServer, "qkchat.server.sslserver")

CustomSslServer::CustomSslServer(QObject *parent)
    : QTcpServer(parent)
{
    _sslConfiguration = QSslConfiguration::defaultConfiguration();
    LogManager::instance()->writeSystemLog("SSLServer", "CREATED", "CustomSslServer instance created");
}

CustomSslServer::~CustomSslServer()
{
    LogManager::instance()->writeSystemLog("SSLServer", "DESTROYED", "CustomSslServer instance destroyed");
}

void CustomSslServer::setSslConfiguration(const QSslConfiguration &configuration)
{
    _sslConfiguration = configuration;
}

QSslConfiguration CustomSslServer::sslConfiguration() const
{
    return _sslConfiguration;
}

void CustomSslServer::incomingConnection(qintptr socketDescriptor)
{
    // 创建SSL套接字
    QSslSocket *sslSocket = new QSslSocket(this);

    // 设置SSL配置
    sslSocket->setSslConfiguration(_sslConfiguration);

    // 将套接字描述符设置到SSL套接字
    if (sslSocket->setSocketDescriptor(socketDescriptor)) {
        // 添加到待处理连接列表
        addPendingConnection(sslSocket);

        // 启动SSL握手
        sslSocket->startServerEncryption();

        // 发出新连接信号
        emit newConnection();

        LogManager::instance()->writeSslLog(QString::number(socketDescriptor), "NEW_CONNECTION",
                                          QString("From: %1").arg(sslSocket->peerAddress().toString()));
    } else {
        LogManager::instance()->writeErrorLog(QString("Failed to set socket descriptor: %1").arg(sslSocket->errorString()),
                                            "SSLServer");
        sslSocket->deleteLater();
    }
}