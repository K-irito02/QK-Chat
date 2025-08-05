-- QK Chat Server MySQL Database Schema
-- 优化版本 v1.1 - 高性能、可扩展、安全

-- 设置字符集
SET NAMES utf8mb4;
SET CHARACTER SET utf8mb4;

-- 创建数据库
CREATE DATABASE IF NOT EXISTS qkchat CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE qkchat;

-- 设置SQL模式
SET sql_mode = 'STRICT_TRANS_TABLES,NO_ZERO_DATE,NO_ZERO_IN_DATE,ERROR_FOR_DIVISION_BY_ZERO';

-- 用户表
CREATE TABLE IF NOT EXISTS users (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE COMMENT '用户名',
    email VARCHAR(100) NOT NULL UNIQUE COMMENT '邮箱',
    password_hash VARCHAR(255) NOT NULL COMMENT '密码哈希',
    salt VARCHAR(64) NOT NULL COMMENT '盐值',
    display_name VARCHAR(50) DEFAULT NULL COMMENT '显示名称',
    avatar_url VARCHAR(512) DEFAULT NULL COMMENT '头像URL',
    bio TEXT DEFAULT NULL COMMENT '个人简介',
    status ENUM('active', 'inactive', 'banned', 'deleted') DEFAULT 'inactive' COMMENT '账户状态',
    email_verified BOOLEAN DEFAULT FALSE COMMENT '邮箱是否已验证',
    verification_token VARCHAR(64) DEFAULT NULL COMMENT '邮箱验证令牌',
    last_online TIMESTAMP NULL DEFAULT CURRENT_TIMESTAMP COMMENT '最后在线时间',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    
    UNIQUE INDEX idx_username (username),
    UNIQUE INDEX idx_email (email),
    INDEX idx_status (status),
    INDEX idx_last_online (last_online)
) ENGINE=InnoDB COMMENT='用户表';

-- 用户会话表
CREATE TABLE IF NOT EXISTS user_sessions (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL,
    session_token VARCHAR(128) NOT NULL UNIQUE COMMENT '会话令牌',
    refresh_token VARCHAR(128) DEFAULT NULL COMMENT '刷新令牌',
    device_info VARCHAR(500) DEFAULT NULL COMMENT '设备信息',
    ip_address VARCHAR(45) DEFAULT NULL COMMENT 'IP地址',
    user_agent TEXT DEFAULT NULL COMMENT '用户代理',
    expires_at TIMESTAMP NOT NULL COMMENT '过期时间',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后活动时间',
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_session_token (session_token),
    INDEX idx_expires_at (expires_at),
    INDEX idx_user_expires (user_id, expires_at)
) ENGINE=InnoDB COMMENT='用户会话表';

-- 消息表（优化版本，去掉分区表）
CREATE TABLE IF NOT EXISTS messages (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    message_id VARCHAR(36) NOT NULL UNIQUE COMMENT '消息唯一标识',
    sender_id BIGINT UNSIGNED NOT NULL COMMENT '发送者ID',
    receiver_id BIGINT UNSIGNED NOT NULL COMMENT '接收者ID',
    message_type ENUM('text', 'image', 'file', 'audio', 'video', 'system', 'location', 'contact') DEFAULT 'text' COMMENT '消息类型',
    content TEXT NOT NULL COMMENT '消息内容',
    encrypted_content BLOB DEFAULT NULL COMMENT '加密消息内容',
    file_url VARCHAR(512) DEFAULT NULL COMMENT '文件URL',
    file_size BIGINT DEFAULT NULL COMMENT '文件大小',
    file_hash VARCHAR(64) DEFAULT NULL COMMENT '文件哈希值',
    is_encrypted BOOLEAN DEFAULT FALSE COMMENT '是否加密',
    encryption_type ENUM('none', 'aes256', 'rsa2048', 'ecc_p256') DEFAULT 'none' COMMENT '加密类型',
    delivery_status ENUM('sent', 'delivered', 'read', 'failed') DEFAULT 'sent' COMMENT '投递状态',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    
    INDEX idx_message_id (message_id),
    INDEX idx_sender_id (sender_id),
    INDEX idx_receiver_id (receiver_id),
    INDEX idx_created_at (created_at),
    INDEX idx_delivery_status (delivery_status),
    INDEX idx_sender_receiver_created (sender_id, receiver_id, created_at)
) ENGINE=InnoDB COMMENT='消息表';

-- 消息已读状态表（优化查询性能）
CREATE TABLE IF NOT EXISTS message_read_status (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    message_id BIGINT UNSIGNED NOT NULL COMMENT '消息ID',
    user_id BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    read_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '已读时间',
    
    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY uk_message_user (message_id, user_id),
    INDEX idx_user_read_at (user_id, read_at),
    INDEX idx_message_id (message_id)
) ENGINE=InnoDB COMMENT='消息已读状态表';

-- 好友关系表
CREATE TABLE IF NOT EXISTS friendships (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    friend_id BIGINT UNSIGNED NOT NULL COMMENT '好友ID',
    status ENUM('pending', 'accepted', 'blocked', 'deleted') DEFAULT 'pending' COMMENT '关系状态',
    requested_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '请求时间',
    accepted_at TIMESTAMP NULL DEFAULT NULL COMMENT '接受时间',
    blocked_at TIMESTAMP NULL DEFAULT NULL COMMENT '屏蔽时间',
    note VARCHAR(255) DEFAULT NULL COMMENT '备注',
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (friend_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY uk_friendship (user_id, friend_id),
    INDEX idx_user_id (user_id),
    INDEX idx_friend_id (friend_id),
    INDEX idx_status (status),
    INDEX idx_user_status (user_id, status)
) ENGINE=InnoDB COMMENT='好友关系表';

-- 群组表
CREATE TABLE IF NOT EXISTS chat_groups (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    group_name VARCHAR(100) NOT NULL COMMENT '群组名称',
    description TEXT DEFAULT NULL COMMENT '群组描述',
    avatar_url VARCHAR(255) DEFAULT NULL COMMENT '群组头像',
    owner_id BIGINT UNSIGNED NOT NULL COMMENT '群主ID',
    max_members INT DEFAULT 500 COMMENT '最大成员数',
    current_members INT DEFAULT 0 COMMENT '当前成员数',
    is_public BOOLEAN DEFAULT FALSE COMMENT '是否公开群组',
    is_encrypted BOOLEAN DEFAULT FALSE COMMENT '是否加密群组',
    encryption_key_id VARCHAR(64) DEFAULT NULL COMMENT '群组加密密钥ID',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    
    FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_owner_id (owner_id),
    INDEX idx_is_public (is_public),
    INDEX idx_created_at (created_at),
    INDEX idx_is_encrypted (is_encrypted)
) ENGINE=InnoDB COMMENT='群组表';

-- 群组成员表
CREATE TABLE IF NOT EXISTS group_members (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    group_id BIGINT UNSIGNED NOT NULL,
    user_id BIGINT UNSIGNED NOT NULL,
    role ENUM('owner', 'admin', 'member') DEFAULT 'member' COMMENT '成员角色',
    nickname VARCHAR(50) DEFAULT NULL COMMENT '群内昵称',
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '加入时间',
    last_message_at TIMESTAMP NULL DEFAULT NULL COMMENT '最后发言时间',
    
    FOREIGN KEY (group_id) REFERENCES chat_groups(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY uk_group_member (group_id, user_id),
    INDEX idx_group_id (group_id),
    INDEX idx_user_id (user_id),
    INDEX idx_role (role),
    INDEX idx_group_role (group_id, role)
) ENGINE=InnoDB COMMENT='群组成员表';

-- 群组消息表（优化版本，去掉分区表）
CREATE TABLE IF NOT EXISTS group_messages (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    message_id VARCHAR(36) NOT NULL UNIQUE COMMENT '消息唯一标识',
    group_id BIGINT UNSIGNED NOT NULL COMMENT '群组ID',
    sender_id BIGINT UNSIGNED NOT NULL COMMENT '发送者ID',
    message_type ENUM('text', 'image', 'file', 'audio', 'video', 'system', 'location', 'contact') DEFAULT 'text' COMMENT '消息类型',
    content TEXT NOT NULL COMMENT '消息内容',
    encrypted_content BLOB DEFAULT NULL COMMENT '加密消息内容',
    file_url VARCHAR(512) DEFAULT NULL COMMENT '文件URL',
    file_size BIGINT DEFAULT NULL COMMENT '文件大小',
    file_hash VARCHAR(64) DEFAULT NULL COMMENT '文件哈希值',
    is_encrypted BOOLEAN DEFAULT FALSE COMMENT '是否加密',
    encryption_type ENUM('none', 'aes256', 'rsa2048', 'ecc_p256') DEFAULT 'none' COMMENT '加密类型',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    
    INDEX idx_message_id (message_id),
    INDEX idx_group_id (group_id),
    INDEX idx_sender_id (sender_id),
    INDEX idx_created_at (created_at),
    INDEX idx_group_created (group_id, created_at)
) ENGINE=InnoDB COMMENT='群组消息表';

-- 群组消息已读状态表
CREATE TABLE IF NOT EXISTS group_message_read_status (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    message_id BIGINT UNSIGNED NOT NULL COMMENT '消息ID',
    user_id BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    read_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '已读时间',
    
    FOREIGN KEY (message_id) REFERENCES group_messages(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY uk_group_message_user (message_id, user_id),
    INDEX idx_user_read_at (user_id, read_at),
    INDEX idx_message_id (message_id)
) ENGINE=InnoDB COMMENT='群组消息已读状态表';

-- 加密密钥表
CREATE TABLE IF NOT EXISTS encryption_keys (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    key_id VARCHAR(64) NOT NULL UNIQUE COMMENT '密钥ID',
    user_id BIGINT UNSIGNED DEFAULT NULL COMMENT '用户ID（用户密钥）',
    group_id BIGINT UNSIGNED DEFAULT NULL COMMENT '群组ID（群组密钥）',
    key_type ENUM('user', 'group', 'session') NOT NULL COMMENT '密钥类型',
    encryption_type ENUM('aes256', 'rsa2048', 'ecc_p256') NOT NULL COMMENT '加密类型',
    public_key BLOB NOT NULL COMMENT '公钥',
    encrypted_private_key BLOB DEFAULT NULL COMMENT '加密的私钥',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    expires_at TIMESTAMP NULL DEFAULT NULL COMMENT '过期时间',
    is_active BOOLEAN DEFAULT TRUE COMMENT '是否激活',
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (group_id) REFERENCES chat_groups(id) ON DELETE CASCADE,
    INDEX idx_key_id (key_id),
    INDEX idx_user_id (user_id),
    INDEX idx_group_id (group_id),
    INDEX idx_key_type (key_type),
    INDEX idx_encryption_type (encryption_type),
    INDEX idx_expires_at (expires_at)
) ENGINE=InnoDB COMMENT='加密密钥表';

-- 文件传输记录表
CREATE TABLE IF NOT EXISTS file_transfers (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    transfer_id VARCHAR(36) NOT NULL UNIQUE COMMENT '传输唯一标识',
    sender_id BIGINT UNSIGNED NOT NULL COMMENT '发送者ID',
    receiver_id BIGINT UNSIGNED DEFAULT NULL COMMENT '接收者ID（私聊）',
    group_id BIGINT UNSIGNED DEFAULT NULL COMMENT '群组ID（群聊）',
    file_name VARCHAR(255) NOT NULL COMMENT '文件名',
    file_size BIGINT NOT NULL COMMENT '文件大小',
    file_path VARCHAR(500) NOT NULL COMMENT '文件存储路径',
    file_hash VARCHAR(64) DEFAULT NULL COMMENT '文件MD5哈希',
    mime_type VARCHAR(100) DEFAULT NULL COMMENT 'MIME类型',
    status ENUM('uploading', 'completed', 'failed', 'cancelled') DEFAULT 'uploading' COMMENT '传输状态',
    progress INT DEFAULT 0 COMMENT '传输进度(0-100)',
    is_encrypted BOOLEAN DEFAULT FALSE COMMENT '是否加密',
    encryption_key_id VARCHAR(64) DEFAULT NULL COMMENT '加密密钥ID',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    completed_at TIMESTAMP NULL DEFAULT NULL COMMENT '完成时间',
    
    FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (receiver_id) REFERENCES users(id) ON DELETE SET NULL,
    FOREIGN KEY (group_id) REFERENCES chat_groups(id) ON DELETE CASCADE,
    INDEX idx_transfer_id (transfer_id),
    INDEX idx_sender_id (sender_id),
    INDEX idx_receiver_id (receiver_id),
    INDEX idx_group_id (group_id),
    INDEX idx_status (status),
    INDEX idx_created_at (created_at),
    INDEX idx_file_hash (file_hash)
) ENGINE=InnoDB COMMENT='文件传输记录表';

-- 系统日志表（优化版本，去掉分区表）
CREATE TABLE IF NOT EXISTS system_logs (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    log_level ENUM('DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL') NOT NULL COMMENT '日志级别',
    module VARCHAR(50) NOT NULL COMMENT '模块名称',
    message TEXT NOT NULL COMMENT '日志消息',
    user_id BIGINT UNSIGNED DEFAULT NULL COMMENT '相关用户ID',
    ip_address VARCHAR(45) DEFAULT NULL COMMENT 'IP地址',
    user_agent TEXT DEFAULT NULL COMMENT '用户代理',
    extra_data JSON DEFAULT NULL COMMENT '额外数据(JSON格式)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    
    INDEX idx_log_level (log_level),
    INDEX idx_module (module),
    INDEX idx_user_id (user_id),
    INDEX idx_created_at (created_at),
    INDEX idx_level_created (log_level, created_at)
) ENGINE=InnoDB COMMENT='系统日志表';

-- 服务器统计表
CREATE TABLE IF NOT EXISTS server_stats (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    stat_date DATE NOT NULL COMMENT '统计日期',
    stat_hour TINYINT DEFAULT NULL COMMENT '统计小时（NULL表示全天）',
    online_users INT DEFAULT 0 COMMENT '在线用户数',
    new_registrations INT DEFAULT 0 COMMENT '新注册用户数',
    messages_sent INT DEFAULT 0 COMMENT '发送消息数',
    files_transferred INT DEFAULT 0 COMMENT '文件传输数',
    total_users INT DEFAULT 0 COMMENT '总用户数',
    active_users INT DEFAULT 0 COMMENT '活跃用户数',
    peak_online_users INT DEFAULT 0 COMMENT '峰值在线用户数',
    avg_response_time DECIMAL(10,3) DEFAULT 0 COMMENT '平均响应时间(ms)',
    error_count INT DEFAULT 0 COMMENT '错误数量',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    
    UNIQUE KEY uk_stat_date_hour (stat_date, stat_hour),
    INDEX idx_stat_date (stat_date),
    INDEX idx_stat_hour (stat_hour)
) ENGINE=InnoDB COMMENT='服务器统计表';

-- 用户活动统计表
CREATE TABLE IF NOT EXISTS user_activity_stats (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    stat_date DATE NOT NULL COMMENT '统计日期',
    login_count INT DEFAULT 0 COMMENT '登录次数',
    message_count INT DEFAULT 0 COMMENT '发送消息数',
    file_count INT DEFAULT 0 COMMENT '发送文件数',
    online_duration INT DEFAULT 0 COMMENT '在线时长(分钟)',
    last_activity TIMESTAMP NULL DEFAULT NULL COMMENT '最后活动时间',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY uk_user_date (user_id, stat_date),
    INDEX idx_user_id (user_id),
    INDEX idx_stat_date (stat_date),
    INDEX idx_last_activity (last_activity)
) ENGINE=InnoDB COMMENT='用户活动统计表';

-- 邮箱验证记录表
CREATE TABLE IF NOT EXISTS email_verifications (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    email VARCHAR(100) NOT NULL COMMENT '邮箱地址',
    verification_token VARCHAR(64) NOT NULL UNIQUE COMMENT '验证令牌',
    token_type ENUM('register', 'reset_password', 'change_email') DEFAULT 'register' COMMENT '令牌类型',
    expires_at TIMESTAMP NOT NULL COMMENT '过期时间',
    used BOOLEAN DEFAULT FALSE COMMENT '是否已使用',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    used_at TIMESTAMP NULL DEFAULT NULL COMMENT '使用时间',
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_verification_token (verification_token),
    INDEX idx_email (email),
    INDEX idx_expires_at (expires_at),
    INDEX idx_token_type (token_type)
) ENGINE=InnoDB COMMENT='邮箱验证记录表';

-- 邮箱验证码表
CREATE TABLE IF NOT EXISTS email_verification_codes (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    email VARCHAR(100) NOT NULL COMMENT '邮箱地址',
    verification_code VARCHAR(6) NOT NULL COMMENT '验证码',
    expires_at TIMESTAMP NOT NULL COMMENT '过期时间',
    used BOOLEAN DEFAULT FALSE COMMENT '是否已使用',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    used_at TIMESTAMP NULL DEFAULT NULL COMMENT '使用时间',
    
    INDEX idx_email (email),
    INDEX idx_verification_code (verification_code),
    INDEX idx_expires_at (expires_at),
    INDEX idx_email_code (email, verification_code)
) ENGINE=InnoDB COMMENT='邮箱验证码表';

-- 创建默认管理员用户（用于测试）
INSERT INTO users (username, email, password_hash, salt, display_name, status) VALUES 
('admin', 'admin@qkchat.com', SHA2(CONCAT('QKchat2024!', 'admin_salt'), 256), 'admin_salt', '系统管理员', 'active')
ON DUPLICATE KEY UPDATE username=username;

-- 注意：复合索引已在表定义中创建，这里不再重复创建
-- user_sessions表已有 idx_user_expires (user_id, expires_at)
-- friendships表已有 idx_user_status (user_id, status)  
-- group_members表已有相应的索引

-- 创建视图简化常用查询
CREATE OR REPLACE VIEW active_users AS
SELECT 
    id, username, email, display_name, avatar_url, last_online, status
FROM users 
WHERE status = 'active'
ORDER BY last_online DESC;

CREATE OR REPLACE VIEW recent_messages AS
SELECT 
    m.id, m.message_id, m.message_type, m.content, m.delivery_status, m.created_at,
    s.username as sender_username, s.display_name as sender_name,
    r.username as receiver_username, r.display_name as receiver_name
FROM messages m
JOIN users s ON m.sender_id = s.id
JOIN users r ON m.receiver_id = r.id
WHERE m.created_at >= DATE_SUB(NOW(), INTERVAL 7 DAY)
ORDER BY m.created_at DESC;

-- This view is problematic as it cannot be parameterized. 
-- It's better to implement this logic in the application code.
-- For demonstration, here is a corrected but still limited version.
CREATE OR REPLACE VIEW user_friends_example AS
SELECT 
    f.user_id as user_id,
    u.id as friend_id, u.username, u.display_name, u.avatar_url, u.last_online,
    f.status, f.requested_at, f.accepted_at
FROM friendships f
JOIN users u ON (f.friend_id = u.id)
WHERE f.status = 'accepted'
ORDER BY u.last_online DESC;

CREATE OR REPLACE VIEW group_info AS
SELECT 
    g.id, g.group_name, g.description, g.avatar_url, g.current_members, g.max_members,
    g.is_public, g.is_encrypted, g.created_at,
    u.username as owner_username, u.display_name as owner_name
FROM chat_groups g
JOIN users u ON g.owner_id = u.id;

-- 存储过程：清理过期会话
DELIMITER //
CREATE PROCEDURE CleanExpiredSessions()
BEGIN
    DECLARE deleted_count INT DEFAULT 0;
    DELETE FROM user_sessions WHERE expires_at < NOW();
    SET deleted_count = ROW_COUNT();
    SELECT deleted_count as deleted_sessions;
END //
DELIMITER ;

-- 存储过程：获取用户聊天统计
DELIMITER //
CREATE PROCEDURE GetUserChatStats(IN user_id BIGINT)
BEGIN
    SELECT 
        COUNT(*) as total_messages,
        COUNT(CASE WHEN delivery_status = 'read' THEN 1 END) as read_messages,
        COUNT(DISTINCT receiver_id) as chat_partners,
        MAX(created_at) as last_message_time,
        COUNT(CASE WHEN created_at >= DATE_SUB(NOW(), INTERVAL 24 HOUR) THEN 1 END) as messages_today
    FROM messages 
    WHERE sender_id = user_id;
END //
DELIMITER ;

-- 存储过程：更新群组成员数
DELIMITER //
CREATE PROCEDURE UpdateGroupMemberCount(IN p_group_id BIGINT)
BEGIN
    UPDATE chat_groups 
    SET current_members = (
        SELECT COUNT(*) 
        FROM group_members 
        WHERE group_id = p_group_id
    )
    WHERE id = p_group_id;
END //
DELIMITER ;

-- 存储过程：清理过期数据
DELIMITER //
CREATE PROCEDURE CleanupExpiredData()
BEGIN
    -- 清理过期会话
    DELETE FROM user_sessions WHERE expires_at < NOW();
    
    -- 清理过期密钥
    DELETE FROM encryption_keys WHERE expires_at < NOW() AND expires_at IS NOT NULL;
    
    -- 清理30天前的系统日志
    DELETE FROM system_logs WHERE created_at < DATE_SUB(NOW(), INTERVAL 30 DAY);
    
    -- 清理1年前的统计数据
    DELETE FROM server_stats WHERE stat_date < DATE_SUB(NOW(), INTERVAL 1 YEAR);
    
    SELECT 'Cleanup completed' as status;
END //
DELIMITER ;

-- 触发器功能可以在后续手动配置
-- 注意：触发器需要手动创建以确保数据一致性

-- 注意：事件调度器需要手动启用
-- SET GLOBAL event_scheduler = ON;
-- 事件调度器功能可以在后续手动配置

-- 数据库初始化完成
SELECT 'QK Chat Database Schema Optimized v1.1 Initialized Successfully' as Status;