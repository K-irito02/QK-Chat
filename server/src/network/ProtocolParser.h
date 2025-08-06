#ifndef PROTOCOLPARSER_H
#define PROTOCOLPARSER_H

#include <QObject>
#include <QByteArray>
#include <QVariantMap>

/**
 * @brief 自定义二进制协议解析器
 * 
 * 协议格式：
 * - 心跳标志位(1字节): 0x00=普通消息, 0x01=心跳消息
 * - 消息类型(2字节): 消息类型枚举
 * - 消息长度(4字节): 消息体长度(大端序)
 * - 消息体(可变): JSON格式的消息内容
 */
class ProtocolParser : public QObject
{
    Q_OBJECT
    
public:
    explicit ProtocolParser(QObject *parent = nullptr);
    
    // 消息类型枚举
    enum MessageType : quint16 {
        // 认证相关
        LOGIN_REQUEST = 0x0001,
        LOGIN_RESPONSE = 0x0002,
        LOGOUT_REQUEST = 0x0003,
        LOGOUT_RESPONSE = 0x0004,
        REGISTER_REQUEST = 0x0005,
        REGISTER_RESPONSE = 0x0006,
        
        // 消息相关
        SEND_MESSAGE = 0x0101,
        MESSAGE_RECEIVED = 0x0102,
        MESSAGE_DELIVERED = 0x0103,
        MESSAGE_READ = 0x0104,
        
        // 用户相关
        USER_ONLINE = 0x0201,
        USER_OFFLINE = 0x0202,
        USER_LIST_REQUEST = 0x0203,
        USER_LIST_RESPONSE = 0x0204,
        
        // 文件传输
        FILE_UPLOAD_REQUEST = 0x0301,
        FILE_UPLOAD_RESPONSE = 0x0302,
        FILE_DOWNLOAD_REQUEST = 0x0303,
        FILE_DOWNLOAD_RESPONSE = 0x0304,
        FILE_CHUNK = 0x0305,
        
        // 系统消息
        HEARTBEAT = 0x0F01,
        HEARTBEAT_RESPONSE = 0x0F02,
        ERROR_MESSAGE = 0x0FFF,
        
        // 邮箱验证
        EMAIL_VERIFICATION = 0x0A01
    };
    
    // 解析消息
    QVariantMap parseMessage(const QByteArray &data);
    
    // 创建消息
    QByteArray createMessage(const QVariantMap &data);
    QByteArray createMessage(MessageType type, const QVariantMap &data, bool isHeartbeat = false);
    
    // 工具方法
    static MessageType getMessageTypeFromString(const QString &typeString);
    static QString getStringFromMessageType(MessageType type);
    static bool isHeartbeatMessage(const QByteArray &data);
    
private:
    struct ProtocolHeader {
        quint8 heartbeatFlag;
        quint16 messageType;
        quint32 messageLength;
    };
    
    static const int HEADER_SIZE = 7; // 1 + 2 + 4 bytes
    
    ProtocolHeader parseHeader(const QByteArray &data);
    QByteArray createHeader(MessageType type, quint32 messageLength, bool isHeartbeat = false);
    bool validateHeader(const ProtocolHeader &header, int totalDataSize);
};

#endif // PROTOCOLPARSER_H 