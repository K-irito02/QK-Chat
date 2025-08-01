#include "ChatController.h"
#include "UserController.h"
#include "../models/UserModel.h"
#include "../network/NetworkClient.h"
#include "../database/LocalDatabase.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QVariantMap>
#include <QVariantList>
#include <QUuid>
#include <QDateTime>
#include <QLoggingCategory>
#include <QDebug>

Q_LOGGING_CATEGORY(chatController, "qkchat.client.chatcontroller")

ChatController::ChatController(QObject *parent)
    : QObject(parent)
    , m_userModel(nullptr)
    , m_networkClient(nullptr)
    , m_localDatabase(nullptr)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_isConnected(false)
    , m_connectionStatus("disconnected")
    , m_heartbeatTimer(new QTimer(this))
    , m_reconnectTimer(new QTimer(this))
    , m_statusUpdateTimer(new QTimer(this))
    , m_retryCount(0)
{
    // 设置定时器
    m_heartbeatTimer->setInterval(HEARTBEAT_INTERVAL);
    m_heartbeatTimer->setSingleShot(false);
    
    m_reconnectTimer->setInterval(RECONNECT_INTERVAL);
    m_reconnectTimer->setSingleShot(true);
    
    m_statusUpdateTimer->setInterval(STATUS_UPDATE_INTERVAL);
    m_statusUpdateTimer->setSingleShot(false);
    
    // 连接定时器信号
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ChatController::onHeartbeatTimer);
    connect(m_reconnectTimer, &QTimer::timeout, this, &ChatController::onReconnectTimer);
    connect(m_statusUpdateTimer, &QTimer::timeout, this, &ChatController::onStatusUpdateTimer);
    
    qCInfo(chatController) << "ChatController created";
}

ChatController::~ChatController()
{
    disconnectFromServer();
    qCInfo(chatController) << "ChatController destroyed";
}

// 属性访问器
bool ChatController::isConnected() const
{
    return m_isConnected;
}

QString ChatController::connectionStatus() const
{
    return m_connectionStatus;
}

QVariantList ChatController::recentChats() const
{
    return m_recentChats;
}

QVariantList ChatController::contacts() const
{
    return m_contacts;
}

QVariantList ChatController::groups() const
{
    return m_groups;
}

// 初始化和连接管理
void ChatController::initialize()
{
    qCInfo(chatController) << "Initializing ChatController";
    
    setupConnections();
    
    // 加载本地数据
    loadRecentChats();
    loadContacts();
    loadGroups();
    
    // 启动状态更新定时器
    m_statusUpdateTimer->start();
    
    // 自动连接到服务器
    connectToServer();
}

void ChatController::connectToServer()
{
    if (m_isConnected || !m_networkClient) {
        return;
    }
    
    qCInfo(chatController) << "Connecting to server...";
    m_connectionStatus = "connecting";
    emit connectionStatusChanged();
    
    // 这里应该从配置中获取服务器地址和端口
    if (m_networkClient->connectToServer("localhost", 8888)) {
        qCInfo(chatController) << "Connection request sent";
    } else {
        qCWarning(chatController) << "Failed to initiate connection";
        m_connectionStatus = "failed";
        emit connectionStatusChanged();
        
        // 重试连接
        if (m_retryCount < MAX_RETRY_COUNT) {
            m_retryCount++;
            m_reconnectTimer->start();
        }
    }
}

void ChatController::disconnectFromServer()
{
    if (!m_isConnected) {
        return;
    }
    
    qCInfo(chatController) << "Disconnecting from server...";
    
    stopHeartbeat();
    m_statusUpdateTimer->stop();
    
    if (m_networkClient) {
        m_networkClient->disconnect();
    }
    
    m_isConnected = false;
    m_connectionStatus = "disconnected";
    emit connectionStatusChanged();
    emit disconnected();
}

// 消息管理
void ChatController::sendMessage(qint64 receiverId, const QString &message, int messageType)
{
    if (!m_isConnected || !m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    QString messageId = generateMessageId();
    QVariantMap messageData = createMessageObject(receiverId, message, messageType);
    messageData["messageId"] = messageId;
    
    // 保存到本地数据库
    saveMessageToLocal(messageData);
    
    // 发送到服务器
    m_networkClient->sendMessage(QString::number(receiverId), message, 
                                 messageType == 0 ? "text" : (messageType == 1 ? "image" : "file"));
    
    // 添加到待发送列表
    m_pendingMessages[messageId] = messageData;
    
    // 更新最近聊天
    updateRecentChat(messageData);
    
    qCInfo(chatController) << "Message sent:" << messageId << "to" << receiverId;
}

void ChatController::sendGroupMessage(qint64 groupId, const QString &message, int messageType)
{
    if (!m_isConnected || !m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    QString messageId = generateMessageId();
    QVariantMap messageData = createMessageObject(0, message, messageType, true, groupId);
    messageData["messageId"] = messageId;
    
    // 保存到本地数据库
    saveMessageToLocal(messageData);
    
    // 发送到服务器 (这里需要扩展NetworkClient支持群组消息)
    // TODO: 实现群组消息发送协议
    
    qCInfo(chatController) << "Group message sent:" << messageId << "to group" << groupId;
}

void ChatController::markMessageAsRead(const QString &messageId)
{
    if (!m_networkClient) {
        return;
    }
    
    // 发送已读状态到服务器
    QVariantMap data;
    data["messageId"] = messageId;
    data["status"] = "read";
    
    // TODO: 发送已读状态
    
    qCInfo(chatController) << "Marked message as read:" << messageId;
}

QVariantList ChatController::getMessageHistory(qint64 userId, int limit, int offset)
{
    if (!m_localDatabase) {
        return QVariantList();
    }
    
    // 从本地数据库获取消息历史
    // TODO: 实现LocalDatabase的消息历史查询
    
    return QVariantList();
}

QVariantList ChatController::getGroupMessageHistory(qint64 groupId, int limit, int offset)
{
    if (!m_localDatabase) {
        return QVariantList();
    }
    
    // 从本地数据库获取群组消息历史
    // TODO: 实现LocalDatabase的群组消息历史查询
    
    return QVariantList();
}

// 联系人管理
void ChatController::addContact(const QString &username, const QString &remark, const QString &group)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    // 发送添加联系人请求到服务器
    QVariantMap data;
    data["username"] = username;
    data["remark"] = remark;
    data["group"] = group;
    
    // TODO: 实现添加联系人协议
    
    qCInfo(chatController) << "Add contact request sent for:" << username;
}

void ChatController::removeContact(qint64 contactId)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    // 发送删除联系人请求到服务器
    QVariantMap data;
    data["contactId"] = contactId;
    
    // TODO: 实现删除联系人协议
    
    qCInfo(chatController) << "Remove contact request sent for:" << contactId;
}

void ChatController::updateContact(qint64 contactId, const QString &remark, const QString &group)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    // 发送更新联系人请求到服务器
    QVariantMap data;
    data["contactId"] = contactId;
    data["remark"] = remark;
    data["group"] = group;
    
    // TODO: 实现更新联系人协议
    
    qCInfo(chatController) << "Update contact request sent for:" << contactId;
}

QVariantMap ChatController::getContactInfo(qint64 contactId)
{
    // 从本地缓存或数据库获取联系人信息
    for (const QVariant &contact : m_contacts) {
        QVariantMap contactMap = contact.toMap();
        if (contactMap["id"].toLongLong() == contactId) {
            return contactMap;
        }
    }
    
    return QVariantMap();
}

void ChatController::refreshContacts()
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    // 从服务器刷新联系人列表
    // TODO: 实现刷新联系人协议
    
    qCInfo(chatController) << "Refresh contacts request sent";
}

// 群组管理
void ChatController::createGroup(const QString &groupName, const QString &description, const QString &avatarUrl)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    QVariantMap data;
    data["groupName"] = groupName;
    data["description"] = description;
    data["avatarUrl"] = avatarUrl;
    
    // TODO: 实现创建群组协议
    
    qCInfo(chatController) << "Create group request sent:" << groupName;
}

void ChatController::joinGroup(qint64 groupId)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    QVariantMap data;
    data["groupId"] = groupId;
    
    // TODO: 实现加入群组协议
    
    qCInfo(chatController) << "Join group request sent:" << groupId;
}

void ChatController::leaveGroup(qint64 groupId)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    QVariantMap data;
    data["groupId"] = groupId;
    
    // TODO: 实现离开群组协议
    
    qCInfo(chatController) << "Leave group request sent:" << groupId;
}

void ChatController::inviteToGroup(qint64 groupId, qint64 userId)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    QVariantMap data;
    data["groupId"] = groupId;
    data["userId"] = userId;
    
    // TODO: 实现邀请加入群组协议
    
    qCInfo(chatController) << "Invite to group request sent:" << userId << "to" << groupId;
}

void ChatController::removeFromGroup(qint64 groupId, qint64 userId)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    QVariantMap data;
    data["groupId"] = groupId;
    data["userId"] = userId;
    
    // TODO: 实现移除群组成员协议
    
    qCInfo(chatController) << "Remove from group request sent:" << userId << "from" << groupId;
}

void ChatController::updateGroupMemberRole(qint64 groupId, qint64 userId, const QString &role)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    QVariantMap data;
    data["groupId"] = groupId;
    data["userId"] = userId;
    data["role"] = role;
    
    // TODO: 实现更新群组成员角色协议
    
    qCInfo(chatController) << "Update group member role request sent:" << userId << "in" << groupId << "to" << role;
}

QVariantMap ChatController::getGroupInfo(qint64 groupId)
{
    // 从本地缓存或数据库获取群组信息
    for (const QVariant &group : m_groups) {
        QVariantMap groupMap = group.toMap();
        if (groupMap["id"].toLongLong() == groupId) {
            return groupMap;
        }
    }
    
    return QVariantMap();
}

QVariantList ChatController::getGroupMembers(qint64 groupId)
{
    // TODO: 从本地数据库或服务器获取群组成员列表
    return QVariantList();
}

void ChatController::refreshGroups()
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    // 从服务器刷新群组列表
    // TODO: 实现刷新群组协议
    
    qCInfo(chatController) << "Refresh groups request sent";
}

// 文件传输
void ChatController::sendFile(qint64 receiverId, const QString &filePath)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    // TODO: 实现文件传输功能
    qCInfo(chatController) << "Send file request:" << filePath << "to" << receiverId;
}

void ChatController::sendFileToGroup(qint64 groupId, const QString &filePath)
{
    if (!m_networkClient) {
        emit error("未连接到服务器");
        return;
    }
    
    // TODO: 实现群组文件传输功能
    qCInfo(chatController) << "Send file to group request:" << filePath << "to" << groupId;
}

void ChatController::downloadFile(const QString &fileUrl, const QString &savePath)
{
    // TODO: 实现文件下载功能
    qCInfo(chatController) << "Download file request:" << fileUrl << "to" << savePath;
}

// 在线状态管理
void ChatController::updateOnlineStatus(const QString &status)
{
    if (!m_networkClient) {
        return;
    }
    
    QVariantMap data;
    data["status"] = status;
    
    // TODO: 实现更新在线状态协议
    
    qCInfo(chatController) << "Update online status request:" << status;
}

QVariantList ChatController::getOnlineContacts()
{
    QVariantList onlineContacts;
    
    for (const QVariant &contact : m_contacts) {
        QVariantMap contactMap = contact.toMap();
        if (contactMap["isOnline"].toBool()) {
            onlineContacts.append(contact);
        }
    }
    
    return onlineContacts;
}

// 实用函数
QString ChatController::formatTime(const QDateTime &dateTime)
{
    QDateTime now = QDateTime::currentDateTime();
    QDate today = now.date();
    QDate messageDate = dateTime.date();
    
    if (messageDate == today) {
        return dateTime.toString("hh:mm");
    } else if (messageDate == today.addDays(-1)) {
        return QString("昨天 %1").arg(dateTime.toString("hh:mm"));
    } else if (messageDate.year() == today.year()) {
        return dateTime.toString("MM/dd hh:mm");
    } else {
        return dateTime.toString("yyyy/MM/dd hh:mm");
    }
}

bool ChatController::isToday(const QDateTime &dateTime)
{
    return dateTime.date() == QDate::currentDate();
}

QString ChatController::generateMessageId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// 设置依赖关系
void ChatController::setUserModel(UserModel *userModel)
{
    m_userModel = userModel;
    qCInfo(chatController) << "UserModel set";
}

void ChatController::setNetworkClient(NetworkClient *networkClient)
{
    m_networkClient = networkClient;
    setupConnections();
    qCInfo(chatController) << "NetworkClient set";
}

void ChatController::setLocalDatabase(LocalDatabase *localDatabase)
{
    m_localDatabase = localDatabase;
    qCInfo(chatController) << "LocalDatabase set";
}

// 网络消息处理槽函数
void ChatController::onNetworkMessageReceived(const QString &messageType, const QVariantMap &data)
{
    qCInfo(chatController) << "Network message received:" << messageType;
    
    if (messageType == "message") {
        processIncomingMessage(data);
    } else if (messageType == "groupMessage") {
        processGroupMessage(data);
    } else if (messageType == "contactUpdate") {
        processContactUpdate(data);
    } else if (messageType == "groupUpdate") {
        processGroupUpdate(data);
    }
}

void ChatController::onNetworkConnectionChanged(bool isConnected)
{
    qCInfo(chatController) << "Network connection changed:" << isConnected;
    
    m_isConnected = isConnected;
    
    if (isConnected) {
        m_retryCount = 0;
        startHeartbeat();
        emit connected();
    } else {
        stopHeartbeat();
        emit disconnected();
        
        // 尝试重连
        if (m_retryCount < MAX_RETRY_COUNT) {
            m_retryCount++;
            m_reconnectTimer->start();
        }
    }
    
    emit connectionStatusChanged();
}

void ChatController::onNetworkError(const QString &error)
{
    qCWarning(chatController) << "Network error:" << error;
    emit connectionError(error);
}

// 定时器槽函数
void ChatController::onHeartbeatTimer()
{
    if (m_networkClient && m_isConnected) {
        m_networkClient->sendHeartbeat();
    }
}

void ChatController::onReconnectTimer()
{
    qCInfo(chatController) << "Attempting to reconnect... (attempt" << m_retryCount << ")";
    connectToServer();
}

void ChatController::onStatusUpdateTimer()
{
    if (m_isConnected) {
        updateOnlineStatus("online");
    }
}

// 私有方法实现
void ChatController::setupConnections()
{
    if (!m_networkClient) {
        return;
    }
    
    // 连接网络客户端信号
    connect(m_networkClient, &NetworkClient::connected, this, [this]() {
        onNetworkConnectionChanged(true);
    });
    
    connect(m_networkClient, &NetworkClient::disconnected, this, [this]() {
        onNetworkConnectionChanged(false);
    });
    
    connect(m_networkClient, &NetworkClient::connectionError, this, &ChatController::onNetworkError);
    
    connect(m_networkClient, &NetworkClient::messageReceived, this, [this](const QString &sender, const QString &content, const QString &messageType, qint64 timestamp) {
        QVariantMap data;
        data["sender"] = sender;
        data["content"] = content;
        data["messageType"] = messageType;
        data["timestamp"] = timestamp;
        onNetworkMessageReceived("message", data);
    });
}

void ChatController::loadRecentChats()
{
    // TODO: 从本地数据库加载最近聊天
    m_recentChats.clear();
    emit recentChatsChanged();
}

void ChatController::loadContacts()
{
    // TODO: 从本地数据库加载联系人列表
    m_contacts.clear();
    emit contactsChanged();
}

void ChatController::loadGroups()
{
    // TODO: 从本地数据库加载群组列表
    m_groups.clear();
    emit groupsChanged();
}

void ChatController::saveMessageToLocal(const QVariantMap &message)
{
    if (!m_localDatabase) {
        return;
    }
    
    // TODO: 实现保存消息到本地数据库
}

void ChatController::updateRecentChat(const QVariantMap &message)
{
    // 更新最近聊天列表
    // TODO: 实现最近聊天更新逻辑
    emit recentChatsChanged();
}

void ChatController::processIncomingMessage(const QVariantMap &messageData)
{
    // 处理收到的消息
    QString senderId = messageData["sender"].toString();
    QString content = messageData["content"].toString();
    QString messageType = messageData["messageType"].toString();
    qint64 timestamp = messageData["timestamp"].toLongLong();
    
    // 保存到本地数据库
    saveMessageToLocal(messageData);
    
    // 更新最近聊天
    updateRecentChat(messageData);
    
    // 发出信号
    emit messageReceived(senderId.toLongLong(), content, 
                        messageType == "text" ? 0 : (messageType == "image" ? 1 : 2), 
                        QDateTime::fromMSecsSinceEpoch(timestamp));
}

void ChatController::processGroupMessage(const QVariantMap &messageData)
{
    // 处理收到的群组消息
    qint64 groupId = messageData["groupId"].toLongLong();
    qint64 senderId = messageData["senderId"].toLongLong();
    QString content = messageData["content"].toString();
    QString messageType = messageData["messageType"].toString();
    qint64 timestamp = messageData["timestamp"].toLongLong();
    
    // 保存到本地数据库
    saveMessageToLocal(messageData);
    
    // 发出信号
    emit groupMessageReceived(groupId, senderId, content,
                             messageType == "text" ? 0 : (messageType == "image" ? 1 : 2),
                             QDateTime::fromMSecsSinceEpoch(timestamp));
}

void ChatController::processContactUpdate(const QVariantMap &contactData)
{
    // 处理联系人更新
    // TODO: 实现联系人更新逻辑
    emit contactsChanged();
}

void ChatController::processGroupUpdate(const QVariantMap &groupData)
{
    // 处理群组更新
    // TODO: 实现群组更新逻辑
    emit groupsChanged();
}

void ChatController::startHeartbeat()
{
    if (!m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->start();
        qCInfo(chatController) << "Heartbeat started";
    }
}

void ChatController::stopHeartbeat()
{
    if (m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->stop();
        qCInfo(chatController) << "Heartbeat stopped";
    }
}

QVariantMap ChatController::createMessageObject(qint64 receiverId, const QString &content, int messageType, bool isGroup, qint64 groupId)
{
    QVariantMap message;
    message["messageId"] = generateMessageId();
    message["senderId"] = m_userModel ? m_userModel->userId() : 0;
    message["receiverId"] = receiverId;
    message["content"] = content;
    message["messageType"] = messageType;
    message["isGroup"] = isGroup;
    message["groupId"] = groupId;
    message["timestamp"] = QDateTime::currentMSecsSinceEpoch();
    message["deliveryStatus"] = "sending";
    
    return message;
}

QVariantMap ChatController::contactToVariant(const QVariantMap &contactData)
{
    // 转换联系人数据格式
    return contactData;
}

QVariantMap ChatController::groupToVariant(const QVariantMap &groupData)
{
    // 转换群组数据格式
    return groupData;
}

QVariantMap ChatController::messageToVariant(const QVariantMap &messageData)
{
    // 转换消息数据格式
    return messageData;
}