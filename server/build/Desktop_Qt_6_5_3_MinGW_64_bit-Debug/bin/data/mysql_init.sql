-- QK Chat Server MySQL Database Schema
-- 初始化脚本 v1.0

-- 创建数据库
CREATE DATABASE IF NOT EXISTS qkchat CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE qkchat;

-- 用户表
CREATE TABLE IF NOT EXISTS users (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) NOT NULL UNIQUE COMMENT '用户名',
    email VARCHAR(100) NOT NULL UNIQUE COMMENT '邮箱',
    password_hash VARCHAR(64) NOT NULL COMMENT 'SHA-256密码哈希',
    avatar_url VARCHAR(255) DEFAULT NULL COMMENT '头像URL',
    display_name VARCHAR(100) DEFAULT NULL COMMENT '显示名称',
    status ENUM('active', 'disabled', 'pending') DEFAULT 'active' COMMENT '用户状态',
    last_online TIMESTAMP NULL DEFAULT NULL COMMENT '最后在线时间',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    
    INDEX idx_username (username),
    INDEX idx_email (email),
    INDEX idx_status (status),
    INDEX idx_last_online (last_online)
) ENGINE=InnoDB COMMENT='用户信息表';

-- 用户会话表
CREATE TABLE IF NOT EXISTS user_sessions (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL,
    session_token VARCHAR(128) NOT NULL UNIQUE COMMENT '会话令牌',
    device_info VARCHAR(500) DEFAULT NULL COMMENT '设备信息',
    ip_address VARCHAR(45) DEFAULT NULL COMMENT 'IP地址',
    expires_at TIMESTAMP NOT NULL COMMENT '过期时间',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    last_activity TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '最后活动时间',
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_user_id (user_id),
    INDEX idx_session_token (session_token),
    INDEX idx_expires_at (expires_at)
) ENGINE=InnoDB COMMENT='用户会话表';

-- 消息表
CREATE TABLE IF NOT EXISTS messages (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    message_id VARCHAR(36) NOT NULL UNIQUE COMMENT '消息唯一标识',
    sender_id BIGINT UNSIGNED NOT NULL COMMENT '发送者ID',
    receiver_id BIGINT UNSIGNED NOT NULL COMMENT '接收者ID',
    message_type ENUM('text', 'image', 'file', 'audio', 'video') DEFAULT 'text' COMMENT '消息类型',
    content TEXT NOT NULL COMMENT '消息内容',
    file_url VARCHAR(255) DEFAULT NULL COMMENT '文件URL（如果是文件消息）',
    file_size BIGINT DEFAULT NULL COMMENT '文件大小（字节）',
    delivery_status ENUM('sent', 'delivered', 'read') DEFAULT 'sent' COMMENT '投递状态',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    delivered_at TIMESTAMP NULL DEFAULT NULL COMMENT '投递时间',
    read_at TIMESTAMP NULL DEFAULT NULL COMMENT '已读时间',
    
    FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (receiver_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_message_id (message_id),
    INDEX idx_sender_id (sender_id),
    INDEX idx_receiver_id (receiver_id),
    INDEX idx_created_at (created_at),
    INDEX idx_delivery_status (delivery_status)
) ENGINE=InnoDB COMMENT='消息表';

-- 好友关系表
CREATE TABLE IF NOT EXISTS friendships (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT UNSIGNED NOT NULL COMMENT '用户ID',
    friend_id BIGINT UNSIGNED NOT NULL COMMENT '好友ID',
    status ENUM('pending', 'accepted', 'blocked') DEFAULT 'pending' COMMENT '关系状态',
    requested_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '请求时间',
    accepted_at TIMESTAMP NULL DEFAULT NULL COMMENT '接受时间',
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (friend_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY uk_friendship (user_id, friend_id),
    INDEX idx_user_id (user_id),
    INDEX idx_friend_id (friend_id),
    INDEX idx_status (status)
) ENGINE=InnoDB COMMENT='好友关系表';

-- 群组表
CREATE TABLE IF NOT EXISTS groups (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    group_name VARCHAR(100) NOT NULL COMMENT '群组名称',
    description TEXT DEFAULT NULL COMMENT '群组描述',
    avatar_url VARCHAR(255) DEFAULT NULL COMMENT '群组头像',
    owner_id BIGINT UNSIGNED NOT NULL COMMENT '群主ID',
    max_members INT DEFAULT 500 COMMENT '最大成员数',
    is_public BOOLEAN DEFAULT FALSE COMMENT '是否公开群组',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    
    FOREIGN KEY (owner_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_owner_id (owner_id),
    INDEX idx_is_public (is_public),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB COMMENT='群组表';

-- 群组成员表
CREATE TABLE IF NOT EXISTS group_members (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    group_id BIGINT UNSIGNED NOT NULL,
    user_id BIGINT UNSIGNED NOT NULL,
    role ENUM('owner', 'admin', 'member') DEFAULT 'member' COMMENT '成员角色',
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '加入时间',
    
    FOREIGN KEY (group_id) REFERENCES groups(id) ON DELETE CASCADE,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    UNIQUE KEY uk_group_member (group_id, user_id),
    INDEX idx_group_id (group_id),
    INDEX idx_user_id (user_id),
    INDEX idx_role (role)
) ENGINE=InnoDB COMMENT='群组成员表';

-- 群组消息表
CREATE TABLE IF NOT EXISTS group_messages (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    message_id VARCHAR(36) NOT NULL UNIQUE COMMENT '消息唯一标识',
    group_id BIGINT UNSIGNED NOT NULL COMMENT '群组ID',
    sender_id BIGINT UNSIGNED NOT NULL COMMENT '发送者ID',
    message_type ENUM('text', 'image', 'file', 'audio', 'video', 'system') DEFAULT 'text' COMMENT '消息类型',
    content TEXT NOT NULL COMMENT '消息内容',
    file_url VARCHAR(255) DEFAULT NULL COMMENT '文件URL',
    file_size BIGINT DEFAULT NULL COMMENT '文件大小',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    
    FOREIGN KEY (group_id) REFERENCES groups(id) ON DELETE CASCADE,
    FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_message_id (message_id),
    INDEX idx_group_id (group_id),
    INDEX idx_sender_id (sender_id),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB COMMENT='群组消息表';

-- 文件传输记录表
CREATE TABLE IF NOT EXISTS file_transfers (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    transfer_id VARCHAR(36) NOT NULL UNIQUE COMMENT '传输唯一标识',
    sender_id BIGINT UNSIGNED NOT NULL COMMENT '发送者ID',
    receiver_id BIGINT UNSIGNED NOT NULL COMMENT '接收者ID',
    file_name VARCHAR(255) NOT NULL COMMENT '文件名',
    file_size BIGINT NOT NULL COMMENT '文件大小',
    file_path VARCHAR(500) NOT NULL COMMENT '文件存储路径',
    file_hash VARCHAR(64) DEFAULT NULL COMMENT '文件MD5哈希',
    status ENUM('uploading', 'completed', 'failed', 'cancelled') DEFAULT 'uploading' COMMENT '传输状态',
    progress INT DEFAULT 0 COMMENT '传输进度(0-100)',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    completed_at TIMESTAMP NULL DEFAULT NULL COMMENT '完成时间',
    
    FOREIGN KEY (sender_id) REFERENCES users(id) ON DELETE CASCADE,
    FOREIGN KEY (receiver_id) REFERENCES users(id) ON DELETE CASCADE,
    INDEX idx_transfer_id (transfer_id),
    INDEX idx_sender_id (sender_id),
    INDEX idx_receiver_id (receiver_id),
    INDEX idx_status (status),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB COMMENT='文件传输记录表';

-- 系统日志表
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
    
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE SET NULL,
    INDEX idx_log_level (log_level),
    INDEX idx_module (module),
    INDEX idx_user_id (user_id),
    INDEX idx_created_at (created_at)
) ENGINE=InnoDB COMMENT='系统日志表';

-- 服务器统计表
CREATE TABLE IF NOT EXISTS server_stats (
    id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
    stat_date DATE NOT NULL COMMENT '统计日期',
    online_users INT DEFAULT 0 COMMENT '在线用户数',
    new_registrations INT DEFAULT 0 COMMENT '新注册用户数',
    messages_sent INT DEFAULT 0 COMMENT '发送消息数',
    files_transferred INT DEFAULT 0 COMMENT '文件传输数',
    total_users INT DEFAULT 0 COMMENT '总用户数',
    active_users INT DEFAULT 0 COMMENT '活跃用户数',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP COMMENT '创建时间',
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT '更新时间',
    
    UNIQUE KEY uk_stat_date (stat_date),
    INDEX idx_stat_date (stat_date)
) ENGINE=InnoDB COMMENT='服务器统计表';

-- 创建默认管理员用户（用于测试）
INSERT INTO users (username, email, password_hash, display_name, status) VALUES 
('admin', 'admin@qkchat.com', SHA2('QKchat2024!', 256), '系统管理员', 'active')
ON DUPLICATE KEY UPDATE username=username;

-- 创建索引优化查询性能
CREATE INDEX idx_users_status_last_online ON users(status, last_online);
CREATE INDEX idx_messages_sender_receiver_created ON messages(sender_id, receiver_id, created_at);
CREATE INDEX idx_user_sessions_user_expires ON user_sessions(user_id, expires_at);

-- 创建视图简化常用查询
CREATE OR REPLACE VIEW active_users AS
SELECT 
    id, username, email, display_name, avatar_url, last_online
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

-- 存储过程：清理过期会话
DELIMITER //
CREATE PROCEDURE CleanExpiredSessions()
BEGIN
    DELETE FROM user_sessions WHERE expires_at < NOW();
    SELECT ROW_COUNT() as deleted_sessions;
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
        MAX(created_at) as last_message_time
    FROM messages 
    WHERE sender_id = user_id;
END //
DELIMITER ;

-- 触发器：更新用户最后在线时间
DELIMITER //
CREATE TRIGGER update_user_last_online 
AFTER UPDATE ON user_sessions
FOR EACH ROW
BEGIN
    IF NEW.last_activity != OLD.last_activity THEN
        UPDATE users SET last_online = NEW.last_activity WHERE id = NEW.user_id;
    END IF;
END //
DELIMITER ;

-- 数据库初始化完成
SELECT 'QK Chat Database Schema Initialized Successfully' as Status; 