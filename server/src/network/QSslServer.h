#ifndef CUSTOMSSLSERVER_H
#define CUSTOMSSLSERVER_H

#include <QTcpServer>
#include <QSslSocket>
#include <QSslConfiguration>
#include <QSslCipher>

/**
 * @brief 自定义SSL服务器类
 * 
 * 继承自QTcpServer，提供SSL连接支持
 */
class CustomSslServer : public QTcpServer
{
    Q_OBJECT
    
public:
    explicit CustomSslServer(QObject *parent = nullptr);
    ~CustomSslServer();
    
    void setSslConfiguration(const QSslConfiguration &configuration);
    QSslConfiguration sslConfiguration() const;
    
signals:
    void newConnection();
    
protected:
    void incomingConnection(qintptr socketDescriptor) override;
    
private:
    QSslConfiguration _sslConfiguration;
};

#endif // CUSTOMSSLSERVER_H