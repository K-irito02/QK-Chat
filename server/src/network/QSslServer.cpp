#include "QSslServer.h"
#include <QSslSocket>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(sslServer, "qkchat.server.sslserver")

CustomSslServer::CustomSslServer(QObject *parent)
    : QTcpServer(parent)
{
    _sslConfiguration = QSslConfiguration::defaultConfiguration();
    qCInfo(sslServer) << "CustomSslServer created";
}

CustomSslServer::~CustomSslServer()
{
    qCInfo(sslServer) << "CustomSslServer destroyed";
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
        
        qCDebug(sslServer) << "New SSL connection from:" << sslSocket->peerAddress().toString();
    } else {
        qCWarning(sslServer) << "Failed to set socket descriptor:" << sslSocket->errorString();
        sslSocket->deleteLater();
    }
}