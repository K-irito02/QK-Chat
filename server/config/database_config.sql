-- 根据配置清单-0.1V.md创建数据库和用户
-- MySQL 8.0+ 配置脚本

-- 设置字符集
SET NAMES utf8mb4;
SET CHARACTER SET utf8mb4;

-- 创建数据库
CREATE DATABASE IF NOT EXISTS qkchat 
CHARACTER SET utf8mb4 
COLLATE utf8mb4_unicode_ci;

-- 创建用户
CREATE USER IF NOT EXISTS 'qkchat_user'@'localhost' 
IDENTIFIED BY '3143285505';

-- 授权
GRANT ALL PRIVILEGES ON qkchat.* TO 'qkchat_user'@'localhost';

-- 刷新权限
FLUSH PRIVILEGES;

-- 使用数据库
USE qkchat;

-- 显示创建结果
SELECT '数据库和用户创建完成' AS status;
SELECT '数据库: qkchat' AS info;
SELECT '用户: qkchat_user@localhost' AS info;
SELECT '密码: 3143285505' AS info;