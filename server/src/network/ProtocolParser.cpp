#include "ProtocolParser.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDataStream>
#include <QLoggingCategory>
#include <QIODevice>

Q_LOGGING_CATEGORY(protocolParser, "qkchat.server.protocolparser")

const int ProtocolParser::HEADER_SIZE;

ProtocolParser::ProtocolParser(QObject *parent)
    : QObject(parent)
{
}

QVariantMap ProtocolParser::parseMessage(const QByteArray &data)
{
    QVariantMap result;
    
    if (data.size() < HEADER_SIZE) {
        qCWarning(protocolParser) << "Data too small for header:" << data.size();
        return result;
    }
    
    // 解析协议头
    ProtocolHeader header = parseHeader(data);
    
    if (!validateHeader(header, data.size())) {
        qCWarning(protocolParser) << "Invalid protocol header";
        return result;
    }
    
    // 提取消息体
    QByteArray messageBody = data.mid(HEADER_SIZE, header.messageLength);
    
    // 解析JSON消息体
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(messageBody, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qCWarning(protocolParser) << "Failed to parse JSON:" << error.errorString();
        return result;
    }
    
    result = doc.toVariant().toMap();
    
    // 添加协议头信息
    result["_protocol_type"] = header.messageType;
    result["_is_heartbeat"] = (header.heartbeatFlag == 0x01);
    result["_message_length"] = header.messageLength;
    
    // 添加消息类型字符串
    if (!result.contains("type")) {
        result["type"] = getStringFromMessageType(static_cast<MessageType>(header.messageType));
    }
    
    return result;
}

QByteArray ProtocolParser::createMessage(const QVariantMap &data)
{
    QString typeString = data["type"].toString();
    MessageType type = getMessageTypeFromString(typeString);
    
    bool isHeartbeat = data.value("_is_heartbeat", false).toBool();
    
    return createMessage(type, data, isHeartbeat);
}

QByteArray ProtocolParser::createMessage(MessageType type, const QVariantMap &data, bool isHeartbeat)
{
    // 创建JSON消息体
    QJsonDocument doc = QJsonDocument::fromVariant(data);
    QByteArray messageBody = doc.toJson(QJsonDocument::Compact);
    
    // 创建协议头
    QByteArray header = createHeader(type, messageBody.size(), isHeartbeat);
    
    // 组合完整消息
    QByteArray fullMessage = header + messageBody;
    
    return fullMessage;
}

ProtocolParser::MessageType ProtocolParser::getMessageTypeFromString(const QString &typeString)
{
    static QHash<QString, MessageType> typeMap = {
        // 认证相关
        {"login", LOGIN_REQUEST},
        {"login_response", LOGIN_RESPONSE},
        {"logout", LOGOUT_REQUEST},
        {"logout_response", LOGOUT_RESPONSE},
        {"register", REGISTER_REQUEST},
        {"register_response", REGISTER_RESPONSE},
        
        // 消息相关
        {"send_message", SEND_MESSAGE},
        {"message_received", MESSAGE_RECEIVED},
        {"message_delivered", MESSAGE_DELIVERED},
        {"message_read", MESSAGE_READ},
        
        // 用户相关
        {"user_online", USER_ONLINE},
        {"user_offline", USER_OFFLINE},
        {"user_list_request", USER_LIST_REQUEST},
        {"user_list_response", USER_LIST_RESPONSE},
        
        // 文件传输
        {"file_upload_request", FILE_UPLOAD_REQUEST},
        {"file_upload_response", FILE_UPLOAD_RESPONSE},
        {"file_download_request", FILE_DOWNLOAD_REQUEST},
        {"file_download_response", FILE_DOWNLOAD_RESPONSE},
        {"file_chunk", FILE_CHUNK},
        
        // 系统消息
        {"heartbeat", HEARTBEAT},
        {"heartbeat_response", HEARTBEAT_RESPONSE},
        {"error", ERROR_MESSAGE}
    };
    
    return typeMap.value(typeString, ERROR_MESSAGE);
}

QString ProtocolParser::getStringFromMessageType(MessageType type)
{
    static QHash<MessageType, QString> stringMap = {
        // 认证相关
        {LOGIN_REQUEST, "login"},
        {LOGIN_RESPONSE, "login_response"},
        {LOGOUT_REQUEST, "logout"},
        {LOGOUT_RESPONSE, "logout_response"},
        {REGISTER_REQUEST, "register"},
        {REGISTER_RESPONSE, "register_response"},
        
        // 消息相关
        {SEND_MESSAGE, "send_message"},
        {MESSAGE_RECEIVED, "message_received"},
        {MESSAGE_DELIVERED, "message_delivered"},
        {MESSAGE_READ, "message_read"},
        
        // 用户相关
        {USER_ONLINE, "user_online"},
        {USER_OFFLINE, "user_offline"},
        {USER_LIST_REQUEST, "user_list_request"},
        {USER_LIST_RESPONSE, "user_list_response"},
        
        // 文件传输
        {FILE_UPLOAD_REQUEST, "file_upload_request"},
        {FILE_UPLOAD_RESPONSE, "file_upload_response"},
        {FILE_DOWNLOAD_REQUEST, "file_download_request"},
        {FILE_DOWNLOAD_RESPONSE, "file_download_response"},
        {FILE_CHUNK, "file_chunk"},
        
        // 系统消息
        {HEARTBEAT, "heartbeat"},
        {HEARTBEAT_RESPONSE, "heartbeat_response"},
        {ERROR_MESSAGE, "error"}
    };
    
    return stringMap.value(type, "unknown");
}

bool ProtocolParser::isHeartbeatMessage(const QByteArray &data)
{
    if (data.size() < 1) {
        return false;
    }
    
    return data.at(0) == 0x01;
}

ProtocolParser::ProtocolHeader ProtocolParser::parseHeader(const QByteArray &data)
{
    ProtocolHeader header = {};
    
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::BigEndian);
    
    stream >> header.heartbeatFlag;
    stream >> header.messageType;
    stream >> header.messageLength;
    
    return header;
}

QByteArray ProtocolParser::createHeader(MessageType type, quint32 messageLength, bool isHeartbeat)
{
    QByteArray header;
    QDataStream stream(&header, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    
    quint8 heartbeatFlag = isHeartbeat ? 0x01 : 0x00;
    
    stream << heartbeatFlag;
    stream << static_cast<quint16>(type);
    stream << messageLength;
    
    return header;
}

bool ProtocolParser::validateHeader(const ProtocolHeader &header, int totalDataSize)
{
    // 检查心跳标志位
    if (header.heartbeatFlag != 0x00 && header.heartbeatFlag != 0x01) {
        qCWarning(protocolParser) << "Invalid heartbeat flag:" << header.heartbeatFlag;
        return false;
    }
    
    // 检查消息类型范围
    if (static_cast<quint32>(header.messageType) > 0xFFFF) {
        qCWarning(protocolParser) << "Invalid message type:" << header.messageType;
        return false;
    }
    
    // 检查消息长度
    if (header.messageLength == 0) {
        qCWarning(protocolParser) << "Empty message body";
        return false;
    }
    
    // 检查数据完整性
    if (totalDataSize < HEADER_SIZE + static_cast<int>(header.messageLength)) {
        qCWarning(protocolParser) << "Incomplete message data. Expected:" 
                                  << (HEADER_SIZE + header.messageLength) 
                                  << "Got:" << totalDataSize;
        return false;
    }
    
    // 检查消息长度合理性（最大16MB）
    const quint32 MAX_MESSAGE_SIZE = 16 * 1024 * 1024;
    if (header.messageLength > MAX_MESSAGE_SIZE) {
        qCWarning(protocolParser) << "Message too large:" << header.messageLength;
        return false;
    }
    
    return true;
} 