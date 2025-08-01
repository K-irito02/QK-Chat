#include "GroupManager.h"
#include "../database/Database.h"
#include <QUuid>
#include <QCryptographicHash>
#include <QLoggingCategory>
#include <QDebug>

Q_LOGGING_CATEGORY(groupManager, "qkchat.server.groupmanager")

GroupManager::GroupManager(Database *database, QObject *parent)
    : QObject(parent)
    , m_database(database)
{
    qCInfo(groupManager) << "GroupManager created";
}

GroupManager::~GroupManager()
{
    qCInfo(groupManager) << "GroupManager destroyed";
}

// 群组管理
qint64 GroupManager::createGroup(qint64 creatorId, const QString &groupName, const QString &description, const QString &avatarUrl)
{
    if (!validateUserId(creatorId) || !validateGroupName(groupName)) {
        return -1;
    }
    
    // 创建群组
    qint64 groupId = m_database->createGroup(groupName, description, creatorId, avatarUrl);
    if (groupId <= 0) {
        qCWarning(groupManager) << "Failed to create group in database";
        return -1;
    }
    
    // 添加创建者为群主
    if (!m_database->addGroupMember(groupId, creatorId, "owner")) {
        qCWarning(groupManager) << "Failed to add creator as owner";
        m_database->deleteGroup(groupId);
        return -1;
    }
    
    // 设置默认群组设置
    GroupSettings defaultSettings;
    updateGroupSettings(groupId, creatorId, defaultSettings);
    
    // 记录日志
    logGroupAction(groupId, creatorId, "create_group", QVariantMap{
        {"group_name", groupName},
        {"description", description}
    });
    
    emit groupCreated(groupId, creatorId);
    qCInfo(groupManager) << "Group created:" << groupId << "by user:" << creatorId;
    
    return groupId;
}

bool GroupManager::deleteGroup(qint64 groupId, qint64 operatorId)
{
    if (!validateGroupId(groupId) || !validateUserId(operatorId)) {
        return false;
    }
    
    // 检查权限（只有群主可以删除群组）
    if (!hasPermission(groupId, operatorId, DeleteGroup)) {
        qCWarning(groupManager) << "User" << operatorId << "has no permission to delete group" << groupId;
        return false;
    }
    
    // 删除群组
    if (!m_database->deleteGroup(groupId)) {
        qCWarning(groupManager) << "Failed to delete group from database";
        return false;
    }
    
    // 记录日志
    logGroupAction(groupId, operatorId, "delete_group");
    
    emit groupDeleted(groupId);
    qCInfo(groupManager) << "Group deleted:" << groupId << "by user:" << operatorId;
    
    return true;
}

bool GroupManager::updateGroupInfo(qint64 groupId, qint64 operatorId, const QVariantMap &info)
{
    if (!validateGroupId(groupId) || !validateUserId(operatorId)) {
        return false;
    }
    
    // 检查权限
    if (!hasPermission(groupId, operatorId, EditGroupInfo)) {
        qCWarning(groupManager) << "User" << operatorId << "has no permission to edit group" << groupId;
        return false;
    }
    
    // 验证群组名称（如果包含）
    if (info.contains("name") && !validateGroupName(info["name"].toString())) {
        return false;
    }
    
    // 更新群组信息
    if (!m_database->updateGroupInfo(groupId, info)) {
        qCWarning(groupManager) << "Failed to update group info in database";
        return false;
    }
    
    // 记录日志
    logGroupAction(groupId, operatorId, "update_group_info", info);
    
    qCInfo(groupManager) << "Group info updated:" << groupId << "by user:" << operatorId;
    return true;
}

QVariantMap GroupManager::getGroupInfo(qint64 groupId)
{
    if (!validateGroupId(groupId)) {
        return QVariantMap();
    }
    
    Database::GroupInfo groupInfo = m_database->getGroupById(groupId);
    if (groupInfo.id <= 0) {
        return QVariantMap();
    }
    
    QVariantMap result;
    result["id"] = groupInfo.id;
    result["name"] = groupInfo.name;
    result["description"] = groupInfo.description;
    result["avatarUrl"] = groupInfo.avatarUrl;
    result["creatorId"] = groupInfo.creatorId;
    result["memberCount"] = groupInfo.memberCount;
    result["createdAt"] = groupInfo.createdAt;
    result["updatedAt"] = groupInfo.updatedAt;
    
    return result;
}

// 成员管理
bool GroupManager::addMember(qint64 groupId, qint64 userId, qint64 operatorId, MemberRole role)
{
    if (!validateGroupId(groupId) || !validateUserId(userId) || !validateUserId(operatorId)) {
        return false;
    }
    
    // 检查权限
    if (!hasPermission(groupId, operatorId, InviteMembers)) {
        qCWarning(groupManager) << "User" << operatorId << "has no permission to invite members to group" << groupId;
        return false;
    }
    
    // 检查是否可以分配该角色
    MemberRole operatorRole = getUserRole(groupId, operatorId);
    if (!canManageRole(operatorRole, role)) {
        qCWarning(groupManager) << "User" << operatorId << "cannot assign role" << role << "in group" << groupId;
        return false;
    }
    
    // 检查群组设置
    GroupSettings settings = getGroupSettings(groupId);
    Database::GroupInfo groupInfo = m_database->getGroupById(groupId);
    
    if (groupInfo.memberCount >= settings.maxMembers) {
        qCWarning(groupManager) << "Group" << groupId << "has reached maximum member limit";
        return false;
    }
    
    // 添加成员
    QString roleString;
    switch (role) {
        case Owner: roleString = "owner"; break;
        case Admin: roleString = "admin"; break;
        case Member: 
        default: roleString = "member"; break;
    }
    
    if (!m_database->addGroupMember(groupId, userId, roleString)) {
        qCWarning(groupManager) << "Failed to add member to group in database";
        return false;
    }
    
    // 记录日志
    logGroupAction(groupId, operatorId, "add_member", QVariantMap{
        {"user_id", userId},
        {"role", static_cast<int>(role)}
    });
    
    emit memberAdded(groupId, userId, role);
    qCInfo(groupManager) << "Member added to group:" << userId << "to" << groupId << "with role" << role;
    
    return true;
}

bool GroupManager::removeMember(qint64 groupId, qint64 userId, qint64 operatorId)
{
    if (!validateGroupId(groupId) || !validateUserId(userId) || !validateUserId(operatorId)) {
        return false;
    }
    
    // 不能移除自己（应该使用退群功能）
    if (userId == operatorId) {
        return false;
    }
    
    // 检查权限
    if (!hasPermission(groupId, operatorId, RemoveMembers)) {
        qCWarning(groupManager) << "User" << operatorId << "has no permission to remove members from group" << groupId;
        return false;
    }
    
    // 检查角色层级
    MemberRole operatorRole = getUserRole(groupId, operatorId);
    MemberRole targetRole = getUserRole(groupId, userId);
    
    if (!canManageRole(operatorRole, targetRole)) {
        qCWarning(groupManager) << "User" << operatorId << "cannot remove user" << userId << "from group" << groupId;
        return false;
    }
    
    // 移除成员
    if (!m_database->removeGroupMember(groupId, userId)) {
        qCWarning(groupManager) << "Failed to remove member from group in database";
        return false;
    }
    
    // 记录日志
    logGroupAction(groupId, operatorId, "remove_member", QVariantMap{
        {"user_id", userId}
    });
    
    emit memberRemoved(groupId, userId);
    qCInfo(groupManager) << "Member removed from group:" << userId << "from" << groupId;
    
    return true;
}

bool GroupManager::updateMemberRole(qint64 groupId, qint64 userId, qint64 operatorId, MemberRole newRole)
{
    if (!validateGroupId(groupId) || !validateUserId(userId) || !validateUserId(operatorId)) {
        return false;
    }
    
    // 检查权限
    if (!hasPermission(groupId, operatorId, ManageAdmins)) {
        qCWarning(groupManager) << "User" << operatorId << "has no permission to manage roles in group" << groupId;
        return false;
    }
    
    // 获取当前角色
    MemberRole currentRole = getUserRole(groupId, userId);
    if (currentRole == newRole) {
        return true; // 角色没有变化
    }
    
    // 检查角色层级
    MemberRole operatorRole = getUserRole(groupId, operatorId);
    if (!canManageRole(operatorRole, currentRole) || !canManageRole(operatorRole, newRole)) {
        qCWarning(groupManager) << "User" << operatorId << "cannot change role from" << currentRole << "to" << newRole;
        return false;
    }
    
    // 更新角色
    QString newRoleString;
    switch (newRole) {
        case Owner: newRoleString = "owner"; break;
        case Admin: newRoleString = "admin"; break;
        case Member: 
        default: newRoleString = "member"; break;
    }
    
    if (!m_database->updateGroupMemberRole(groupId, userId, newRoleString)) {
        qCWarning(groupManager) << "Failed to update member role in database";
        return false;
    }
    
    // 记录日志
    logGroupAction(groupId, operatorId, "update_member_role", QVariantMap{
        {"user_id", userId},
        {"old_role", static_cast<int>(currentRole)},
        {"new_role", static_cast<int>(newRole)}
    });
    
    emit memberRoleChanged(groupId, userId, currentRole, newRole);
    qCInfo(groupManager) << "Member role updated:" << userId << "in group" << groupId << "from" << currentRole << "to" << newRole;
    
    return true;
}

QList<QVariantMap> GroupManager::getGroupMembers(qint64 groupId)
{
    if (!validateGroupId(groupId)) {
        return QList<QVariantMap>();
    }
    
    QList<Database::GroupMemberInfo> members = m_database->getGroupMembers(groupId);
    QList<QVariantMap> result;
    
    for (const auto &member : members) {
        QVariantMap memberMap;
        memberMap["userId"] = member.userId;
        memberMap["username"] = member.username;
        memberMap["displayName"] = member.displayName;
        memberMap["avatarUrl"] = member.avatarUrl;
        memberMap["role"] = member.role;
        memberMap["joinedAt"] = member.joinedAt;
        memberMap["isOnline"] = member.isOnline;
        result.append(memberMap);
    }
    
    return result;
}

QList<QVariantMap> GroupManager::getUserGroups(qint64 userId)
{
    if (!validateUserId(userId)) {
        return QList<QVariantMap>();
    }
    
    QList<Database::GroupInfo> groups = m_database->getUserGroups(userId);
    QList<QVariantMap> result;
    
    for (const auto &group : groups) {
        QVariantMap groupMap;
        groupMap["id"] = group.id;
        groupMap["name"] = group.name;
        groupMap["description"] = group.description;
        groupMap["avatarUrl"] = group.avatarUrl;
        groupMap["memberCount"] = group.memberCount;
        groupMap["createdAt"] = group.createdAt;
        result.append(groupMap);
    }
    
    return result;
}

// 权限检查
bool GroupManager::hasPermission(qint64 groupId, qint64 userId, Permission permission)
{
    Permissions userPermissions = getUserPermissions(groupId, userId);
    return userPermissions.testFlag(permission);
}

GroupManager::Permissions GroupManager::getUserPermissions(qint64 groupId, qint64 userId)
{
    MemberRole role = getUserRole(groupId, userId);
    return getRolePermissions(role);
}

GroupManager::MemberRole GroupManager::getUserRole(qint64 groupId, qint64 userId)
{
    if (!validateGroupId(groupId) || !validateUserId(userId)) {
        return Member; // 默认返回最低权限
    }
    
    QList<Database::GroupMemberInfo> members = m_database->getGroupMembers(groupId);
    for (const auto &member : members) {
        if (member.userId == userId) {
            QString roleStr = member.role;
            if (roleStr == "owner") {
                return Owner;
            } else if (roleStr == "admin") {
                return Admin;
            } else {
                return Member;
            }
        }
    }
    
    return Member; // 不在群组中，返回最低权限
}

// 群组设置
bool GroupManager::updateGroupSettings(qint64 groupId, qint64 operatorId, const GroupSettings &settings)
{
    Q_UNUSED(settings) // TODO: 实现群组设置保存
    
    if (!validateGroupId(groupId) || !validateUserId(operatorId)) {
        return false;
    }
    
    // 检查权限
    if (!hasPermission(groupId, operatorId, ManagePermissions)) {
        qCWarning(groupManager) << "User" << operatorId << "has no permission to manage settings in group" << groupId;
        return false;
    }
    
    // TODO: 保存群组设置到数据库
    // 这里需要扩展数据库表来存储群组设置
    
    // 记录日志
    logGroupAction(groupId, operatorId, "update_group_settings");
    
    emit groupSettingsChanged(groupId);
    qCInfo(groupManager) << "Group settings updated:" << groupId;
    
    return true;
}

GroupManager::GroupSettings GroupManager::getGroupSettings(qint64 groupId)
{
    GroupSettings settings; // 返回默认设置
    
    if (!validateGroupId(groupId)) {
        return settings;
    }
    
    // TODO: 从数据库加载群组设置
    
    return settings;
}

// 入群申请
bool GroupManager::requestJoinGroup(qint64 groupId, qint64 userId, const QString &message)
{
    if (!validateGroupId(groupId) || !validateUserId(userId)) {
        return false;
    }
    
    // 检查是否已经是成员
    MemberRole currentRole = getUserRole(groupId, userId);
    if (currentRole != Member || m_database->getGroupMembers(groupId).size() > 0) {
        // 如果用户已经在群组中，不需要申请
        for (const auto &member : m_database->getGroupMembers(groupId)) {
            if (member.userId == userId) {
                return false; // 已经是成员
            }
        }
    }
    
    // TODO: 实现入群申请逻辑
    // 需要新的数据库表来存储申请记录
    
    emit joinRequestReceived(groupId, userId, message);
    qCInfo(groupManager) << "Join request received for group" << groupId << "from user" << userId;
    
    return true;
}

bool GroupManager::approveJoinRequest(qint64 requestId, qint64 operatorId, bool approved)
{
    // TODO: 实现申请审批逻辑
    Q_UNUSED(requestId)
    Q_UNUSED(operatorId)
    Q_UNUSED(approved)
    
    return true;
}

QList<QVariantMap> GroupManager::getPendingRequests(qint64 groupId)
{
    // TODO: 实现获取待审批申请列表
    Q_UNUSED(groupId)
    
    return QList<QVariantMap>();
}

// 入群码
QString GroupManager::generateJoinCode(qint64 groupId, qint64 operatorId, int validHours)
{
    if (!validateGroupId(groupId) || !validateUserId(operatorId)) {
        return QString();
    }
    
    // 检查权限
    if (!hasPermission(groupId, operatorId, ManagePermissions)) {
        qCWarning(groupManager) << "User" << operatorId << "has no permission to generate join code for group" << groupId;
        return QString();
    }
    
    // 生成入群码
    QString code = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8).toUpper();
    QDateTime expiry = QDateTime::currentDateTime().addSecs(validHours * 3600);
    
    // TODO: 保存入群码到数据库
    
    // 记录日志
    logGroupAction(groupId, operatorId, "generate_join_code", QVariantMap{
        {"join_code", code},
        {"valid_hours", validHours}
    });
    
    qCInfo(groupManager) << "Join code generated for group" << groupId << ":" << code;
    return code;
}

bool GroupManager::joinGroupByCode(const QString &joinCode, qint64 userId)
{
    if (joinCode.isEmpty() || !validateUserId(userId)) {
        return false;
    }
    
    // TODO: 实现通过入群码加入群组
    
    qCInfo(groupManager) << "User" << userId << "attempting to join group with code:" << joinCode;
    return true;
}

bool GroupManager::revokeJoinCode(qint64 groupId, qint64 operatorId)
{
    if (!validateGroupId(groupId) || !validateUserId(operatorId)) {
        return false;
    }
    
    // 检查权限
    if (!hasPermission(groupId, operatorId, ManagePermissions)) {
        return false;
    }
    
    // TODO: 撤销入群码
    
    logGroupAction(groupId, operatorId, "revoke_join_code");
    return true;
}

// 禁言管理
bool GroupManager::muteUser(qint64 groupId, qint64 userId, qint64 operatorId, int minutes)
{
    if (!validateGroupId(groupId) || !validateUserId(userId) || !validateUserId(operatorId)) {
        return false;
    }
    
    // 检查权限
    if (!hasPermission(groupId, operatorId, RemoveMembers)) {
        return false;
    }
    
    // 检查角色层级
    MemberRole operatorRole = getUserRole(groupId, operatorId);
    MemberRole targetRole = getUserRole(groupId, userId);
    
    if (!canManageRole(operatorRole, targetRole)) {
        return false;
    }
    
    // TODO: 实现禁言功能
    
    logGroupAction(groupId, operatorId, "mute_user", QVariantMap{
        {"user_id", userId},
        {"minutes", minutes}
    });
    
    emit userMuted(groupId, userId, minutes);
    return true;
}

bool GroupManager::unmuteUser(qint64 groupId, qint64 userId, qint64 operatorId)
{
    if (!validateGroupId(groupId) || !validateUserId(userId) || !validateUserId(operatorId)) {
        return false;
    }
    
    // 检查权限
    if (!hasPermission(groupId, operatorId, RemoveMembers)) {
        return false;
    }
    
    // TODO: 实现取消禁言功能
    
    logGroupAction(groupId, operatorId, "unmute_user", QVariantMap{
        {"user_id", userId}
    });
    
    emit userUnmuted(groupId, userId);
    return true;
}

bool GroupManager::isUserMuted(qint64 groupId, qint64 userId)
{
    // TODO: 检查用户是否被禁言
    Q_UNUSED(groupId)
    Q_UNUSED(userId)
    
    return false;
}

// 消息管理
bool GroupManager::canSendMessage(qint64 groupId, qint64 userId)
{
    if (!hasPermission(groupId, userId, SendMessage)) {
        return false;
    }
    
    if (isUserMuted(groupId, userId)) {
        return false;
    }
    
    GroupSettings settings = getGroupSettings(groupId);
    if (settings.muteAll && getUserRole(groupId, userId) == Member) {
        return false;
    }
    
    return true;
}

bool GroupManager::deleteMessage(qint64 messageId, qint64 operatorId)
{
    // TODO: 实现删除消息功能
    Q_UNUSED(messageId)
    Q_UNUSED(operatorId)
    
    return true;
}

// 私有方法
GroupManager::Permissions GroupManager::getRolePermissions(MemberRole role)
{
    switch (role) {
    case Owner:
        return SendMessage | InviteMembers | RemoveMembers | ManageAdmins | 
               EditGroupInfo | DeleteGroup | ManagePermissions;
    case Admin:
        return SendMessage | InviteMembers | RemoveMembers | EditGroupInfo;
    case Member:
    default:
        return SendMessage;
    }
}

bool GroupManager::isValidRole(MemberRole role)
{
    return role >= Member && role <= Owner;
}

bool GroupManager::canManageRole(MemberRole operatorRole, MemberRole targetRole)
{
    // 群主可以管理所有角色
    if (operatorRole == Owner) {
        return true;
    }
    
    // 管理员可以管理普通成员
    if (operatorRole == Admin && targetRole == Member) {
        return true;
    }
    
    return false;
}

bool GroupManager::validateGroupName(const QString &name)
{
    return !name.trimmed().isEmpty() && name.length() <= 50;
}

bool GroupManager::validateGroupId(qint64 groupId)
{
    return groupId > 0;
}

bool GroupManager::validateUserId(qint64 userId)
{
    return userId > 0;
}

void GroupManager::logGroupAction(qint64 groupId, qint64 operatorId, const QString &action, const QVariantMap &details)
{
    if (m_database) {
        QVariantMap logData = details;
        logData["group_id"] = groupId;
        logData["action"] = action;
        
        m_database->logEvent(Database::Info, "group_management", 
                           QString("Group action: %1").arg(action), 
                           operatorId, "", "", logData);
    }
}