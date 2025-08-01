#ifndef GROUPMANAGER_H
#define GROUPMANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QDateTime>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(groupManager)

class Database;

class GroupManager : public QObject
{
    Q_OBJECT

public:
    // 群组成员角色
    enum MemberRole {
        Member = 0,     // 普通成员
        Admin = 1,      // 管理员
        Owner = 2       // 群主
    };
    Q_ENUM(MemberRole)

    // 群组权限
    enum Permission {
        SendMessage = 0x01,         // 发送消息
        InviteMembers = 0x02,       // 邀请成员
        RemoveMembers = 0x04,       // 移除成员
        ManageAdmins = 0x08,        // 管理管理员
        EditGroupInfo = 0x10,       // 编辑群组信息
        DeleteGroup = 0x20,         // 删除群组
        ManagePermissions = 0x40    // 管理权限
    };
    Q_DECLARE_FLAGS(Permissions, Permission)
    Q_FLAG(Permissions)

    // 群组设置
    struct GroupSettings {
        bool allowMemberInvite = true;      // 允许成员邀请
        bool requireApproval = false;       // 需要管理员审批
        bool muteAll = false;               // 全员禁言
        int maxMembers = 500;               // 最大成员数
        QString joinCode;                   // 入群码
        QDateTime joinCodeExpiry;           // 入群码过期时间
    };

    explicit GroupManager(Database *database, QObject *parent = nullptr);
    ~GroupManager();

    // 群组管理
    qint64 createGroup(qint64 creatorId, const QString &groupName, const QString &description = QString(), const QString &avatarUrl = QString());
    bool deleteGroup(qint64 groupId, qint64 operatorId);
    bool updateGroupInfo(qint64 groupId, qint64 operatorId, const QVariantMap &info);
    QVariantMap getGroupInfo(qint64 groupId);
    
    // 成员管理
    bool addMember(qint64 groupId, qint64 userId, qint64 operatorId, MemberRole role = Member);
    bool removeMember(qint64 groupId, qint64 userId, qint64 operatorId);
    bool updateMemberRole(qint64 groupId, qint64 userId, qint64 operatorId, MemberRole newRole);
    QList<QVariantMap> getGroupMembers(qint64 groupId);
    QList<QVariantMap> getUserGroups(qint64 userId);
    
    // 权限检查
    bool hasPermission(qint64 groupId, qint64 userId, Permission permission);
    Permissions getUserPermissions(qint64 groupId, qint64 userId);
    MemberRole getUserRole(qint64 groupId, qint64 userId);
    
    // 群组设置
    bool updateGroupSettings(qint64 groupId, qint64 operatorId, const GroupSettings &settings);
    GroupSettings getGroupSettings(qint64 groupId);
    
    // 入群申请
    bool requestJoinGroup(qint64 groupId, qint64 userId, const QString &message = QString());
    bool approveJoinRequest(qint64 requestId, qint64 operatorId, bool approved);
    QList<QVariantMap> getPendingRequests(qint64 groupId);
    
    // 入群码
    QString generateJoinCode(qint64 groupId, qint64 operatorId, int validHours = 24);
    bool joinGroupByCode(const QString &joinCode, qint64 userId);
    bool revokeJoinCode(qint64 groupId, qint64 operatorId);
    
    // 禁言管理
    bool muteUser(qint64 groupId, qint64 userId, qint64 operatorId, int minutes);
    bool unmuteUser(qint64 groupId, qint64 userId, qint64 operatorId);
    bool isUserMuted(qint64 groupId, qint64 userId);
    
    // 消息管理
    bool canSendMessage(qint64 groupId, qint64 userId);
    bool deleteMessage(qint64 messageId, qint64 operatorId);

signals:
    void groupCreated(qint64 groupId, qint64 creatorId);
    void groupDeleted(qint64 groupId);
    void memberAdded(qint64 groupId, qint64 userId, MemberRole role);
    void memberRemoved(qint64 groupId, qint64 userId);
    void memberRoleChanged(qint64 groupId, qint64 userId, MemberRole oldRole, MemberRole newRole);
    void groupSettingsChanged(qint64 groupId);
    void joinRequestReceived(qint64 groupId, qint64 userId, const QString &message);
    void userMuted(qint64 groupId, qint64 userId, int minutes);
    void userUnmuted(qint64 groupId, qint64 userId);

private:
    Database *m_database;
    
    // 权限映射
    static Permissions getRolePermissions(MemberRole role);
    bool isValidRole(MemberRole role);
    bool canManageRole(MemberRole operatorRole, MemberRole targetRole);
    
    // 数据验证
    bool validateGroupName(const QString &name);
    bool validateGroupId(qint64 groupId);
    bool validateUserId(qint64 userId);
    
    // 日志记录
    void logGroupAction(qint64 groupId, qint64 operatorId, const QString &action, const QVariantMap &details = QVariantMap());
};

Q_DECLARE_OPERATORS_FOR_FLAGS(GroupManager::Permissions)

#endif // GROUPMANAGER_H 