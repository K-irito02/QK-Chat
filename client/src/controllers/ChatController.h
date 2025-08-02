#ifndef CHATCONTROLLER_H
#define CHATCONTROLLER_H

#include <QObject>
#include <QQmlEngine>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonDocument>
#include <QVariantMap>
#include <QVariantList>
#include <QMap>

class NetworkClient;
class LocalDatabase;
class UserModel;
class ThreadPool;

/**
 * @brief 聊天控制器类
 * 
 * 负责处理聊天相关的业务逻辑，包括：
 * - 消息收发
 * - 联系人管理
 * - 群组管理
 * - 文件传输
 * - 在线状态管理
 */
class ChatController : public QObject
{
    Q_OBJECT
    
    Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStatusChanged)
    Q_PROPERTY(QString connectionStatus READ connectionStatus NOTIFY connectionStatusChanged)
    Q_PROPERTY(QVariantList recentChats READ recentChats NOTIFY recentChatsChanged)
    Q_PROPERTY(QVariantList contacts READ contacts NOTIFY contactsChanged)
    Q_PROPERTY(QVariantList groups READ groups NOTIFY groupsChanged)
    
public:
    explicit ChatController(QObject *parent = nullptr);
    ~ChatController();
    
    // 属性访问器
    bool isConnected() const;
    QString connectionStatus() const;
    QVariantList recentChats() const;
    QVariantList contacts() const;
    QVariantList groups() const;
    
    // 初始化和连接管理
    Q_INVOKABLE void initialize();
    Q_INVOKABLE void connectToServer();
    Q_INVOKABLE void disconnectFromServer();
    
    // 消息管理
    Q_INVOKABLE void sendMessage(qint64 receiverId, const QString &message, int messageType = 0);
    Q_INVOKABLE void sendGroupMessage(qint64 groupId, const QString &message, int messageType = 0);
    Q_INVOKABLE void markMessageAsRead(const QString &messageId);
    Q_INVOKABLE QVariantList getMessageHistory(qint64 userId, int limit = 50, int offset = 0);
    Q_INVOKABLE QVariantList getGroupMessageHistory(qint64 groupId, int limit = 50, int offset = 0);
    
    // 联系人管理
    Q_INVOKABLE void addContact(const QString &username, const QString &remark = "", const QString &group = "好友");
    Q_INVOKABLE void removeContact(qint64 contactId);
    Q_INVOKABLE void updateContact(qint64 contactId, const QString &remark, const QString &group);
    Q_INVOKABLE QVariantMap getContactInfo(qint64 contactId);
    Q_INVOKABLE void refreshContacts();
    
    // 群组管理
    Q_INVOKABLE void createGroup(const QString &groupName, const QString &description = "", const QString &avatarUrl = "");
    Q_INVOKABLE void joinGroup(qint64 groupId);
    Q_INVOKABLE void leaveGroup(qint64 groupId);
    Q_INVOKABLE void inviteToGroup(qint64 groupId, qint64 userId);
    Q_INVOKABLE void removeFromGroup(qint64 groupId, qint64 userId);
    Q_INVOKABLE void updateGroupMemberRole(qint64 groupId, qint64 userId, const QString &role);
    Q_INVOKABLE QVariantMap getGroupInfo(qint64 groupId);
    Q_INVOKABLE QVariantList getGroupMembers(qint64 groupId);
    Q_INVOKABLE void refreshGroups();
    
    // 文件传输
    Q_INVOKABLE void sendFile(qint64 receiverId, const QString &filePath);
    Q_INVOKABLE void sendFileToGroup(qint64 groupId, const QString &filePath);
    Q_INVOKABLE void downloadFile(const QString &fileUrl, const QString &savePath);
    
    // 在线状态管理
    Q_INVOKABLE void updateOnlineStatus(const QString &status = "online");
    Q_INVOKABLE QVariantList getOnlineContacts();
    
    // 实用函数
    Q_INVOKABLE QString formatTime(const QDateTime &dateTime);
    Q_INVOKABLE bool isToday(const QDateTime &dateTime);
    Q_INVOKABLE QString generateMessageId();
    
public slots:
    void setUserModel(UserModel *userModel);
    void setNetworkClient(NetworkClient *networkClient);
    void setLocalDatabase(LocalDatabase *localDatabase);
    void setThreadPool(ThreadPool *threadPool);
    
signals:
    // 连接状态信号
    void connectionStatusChanged();
    void connected();
    void disconnected();
    void connectionError(const QString &error);
    
    // 消息相关信号
    void messageReceived(qint64 senderId, const QString &message, int messageType, const QDateTime &timestamp);
    void groupMessageReceived(qint64 groupId, qint64 senderId, const QString &message, int messageType, const QDateTime &timestamp);
    void messageSent(const QString &messageId);
    void messageDelivered(const QString &messageId);
    void messageRead(const QString &messageId);
    void messageFailed(const QString &messageId, const QString &error);
    
    // 联系人相关信号
    void contactsChanged();
    void contactAdded(const QVariantMap &contact);
    void contactRemoved(qint64 contactId);
    void contactUpdated(const QVariantMap &contact);
    void contactOnlineStatusChanged(qint64 contactId, bool isOnline);
    
    // 群组相关信号
    void groupsChanged();
    void groupCreated(const QVariantMap &group);
    void groupJoined(qint64 groupId);
    void groupLeft(qint64 groupId);
    void groupMemberAdded(qint64 groupId, const QVariantMap &member);
    void groupMemberRemoved(qint64 groupId, qint64 userId);
    void groupMemberRoleChanged(qint64 groupId, qint64 userId, const QString &role);
    
    // 文件传输信号
    void fileUploadProgress(const QString &fileId, int progress);
    void fileUploadCompleted(const QString &fileId, const QString &fileUrl);
    void fileUploadFailed(const QString &fileId, const QString &error);
    void fileDownloadProgress(const QString &fileId, int progress);
    void fileDownloadCompleted(const QString &fileId, const QString &filePath);
    void fileDownloadFailed(const QString &fileId, const QString &error);
    
    // 其他信号
    void recentChatsChanged();
    void messageHistoryUpdated(qint64 chatId, bool isGroup);
    
    // 通知信号
    void error(const QString &message);
    void warning(const QString &message);
    void info(const QString &message);
    
private slots:
    void onNetworkMessageReceived(const QString &messageType, const QVariantMap &data);
    void onNetworkConnectionChanged(bool connected);
    void onNetworkError(const QString &error);
    
    // 定时器槽函数
    void onHeartbeatTimer();
    void onReconnectTimer();
    void onStatusUpdateTimer();
    
private:
    void setupConnections();
    void loadRecentChats();
    void loadContacts();
    void loadGroups();
    void saveMessageToLocal(const QVariantMap &message);
    void updateRecentChat(const QVariantMap &message);
    void processIncomingMessage(const QVariantMap &messageData);
    void processGroupMessage(const QVariantMap &messageData);
    void processContactUpdate(const QVariantMap &contactData);
    void processGroupUpdate(const QVariantMap &groupData);
    void startHeartbeat();
    void stopHeartbeat();
    QVariantMap createMessageObject(qint64 receiverId, const QString &content, int messageType, bool isGroup = false, qint64 groupId = 0);
    QVariantMap contactToVariant(const QVariantMap &contactData);
    QVariantMap groupToVariant(const QVariantMap &groupData);
    QVariantMap messageToVariant(const QVariantMap &messageData);
    
    // 成员变量
    UserModel *m_userModel;
    NetworkClient *m_networkClient;
    LocalDatabase *m_localDatabase;
    ThreadPool *m_threadPool;
    QNetworkAccessManager *m_networkManager;
    
    bool m_isConnected;
    QString m_connectionStatus;
    int m_retryCount;
    
    QVariantList m_recentChats;
    QVariantList m_contacts;
    QVariantList m_groups;
    
    QTimer *m_heartbeatTimer;
    QTimer *m_reconnectTimer;
    QTimer *m_statusUpdateTimer;
    
    // 文件传输映射
    QMap<QString, QString> m_uploadingFiles;  // fileId -> filePath
    QMap<QString, QString> m_downloadingFiles; // fileId -> savePath
    
    // 消息缓存
    QMap<QString, QVariantMap> m_pendingMessages; // messageId -> message
    QMap<qint64, QVariantList> m_messageCache; // chatId -> messages
    
    static const int MAX_RETRY_COUNT = 3;
    static const int HEARTBEAT_INTERVAL = 30000; // 30秒
    static const int RECONNECT_INTERVAL = 5000;  // 5秒
    static const int STATUS_UPDATE_INTERVAL = 60000; // 1分钟
};

#endif // CHATCONTROLLER_H