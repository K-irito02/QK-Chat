@echo off
chcp 65001 >nul
echo ========================================
echo QK Chat Server - 安全数据库设置
echo ========================================
echo.

echo 此脚本将安全地创建QK Chat Server的数据库和用户
echo.

REM 检查MySQL是否安装
echo 正在检查MySQL安装...
mysql --version >nul 2>&1
if %errorlevel% neq 0 (
    echo 错误：MySQL未安装或不在PATH中
    echo 请安装MySQL并将其bin目录添加到系统PATH
    pause
    exit /b 1
)

echo MySQL已找到。
echo.

echo 正在检查数据库是否已存在...
mysql -u root -p -e "USE qkchat;" 2>nul
if %errorlevel% equ 0 (
    echo 数据库 'qkchat' 已存在。
    echo.
    echo 您想要：
    echo 1. 跳过数据库创建，仅创建表（如果缺失）
    echo 2. 删除并重新创建整个数据库
    echo 3. 退出
    echo.
    set /p choice="请输入您的选择 (1-3): "
    
    if "%choice%"=="1" (
        echo 跳过数据库创建...
        goto create_tables
    ) else if "%choice%"=="2" (
        echo 删除现有数据库...
        mysql -u root -p -e "DROP DATABASE IF EXISTS qkchat;"
        goto create_database
    ) else (
        echo 退出...
        pause
        exit /b 0
    )
) else (
    echo 数据库 'qkchat' 不存在。正在创建...
    goto create_database
)

:create_database
echo.
echo 正在创建数据库和用户...
echo 请在提示时输入您的MySQL root密码：
echo.

REM 创建数据库和用户
mysql -u root -p -e "CREATE DATABASE IF NOT EXISTS qkchat CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;"
mysql -u root -p -e "CREATE USER IF NOT EXISTS 'qkchat_user'@'localhost' IDENTIFIED BY '3143285505';"
mysql -u root -p -e "GRANT ALL PRIVILEGES ON qkchat.* TO 'qkchat_user'@'localhost';"
mysql -u root -p -e "FLUSH PRIVILEGES;"

if %errorlevel% neq 0 (
    echo.
    echo 错误：创建数据库或用户失败
    echo 请检查您的MySQL root密码并重试
    pause
    exit /b 1
)

echo 数据库和用户创建成功！

:create_tables
echo.
echo 正在创建/更新表...
echo.

REM 创建表结构（使用--force选项忽略错误）
mysql -u root -p qkchat --force < ../data/mysql_init.sql

if %errorlevel% neq 0 (
    echo.
    echo 警告：表创建过程中出现了一些错误
    echo 这可能是由于现有表或索引导致的
    echo 数据库应该仍然可以正常工作
    echo.
) else (
    echo 表创建成功！
)

echo.
echo ========================================
echo 数据库设置完成！
echo ========================================
echo.
echo 数据库：qkchat
echo 用户：qkchat_user
echo 密码：3143285505
echo.
echo 您现在可以启动QK Chat Server了。
echo.
pause