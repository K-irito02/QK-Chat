好的！我来为您提供Windows环境下的详细配置过程。

## �� Windows环境配置指南

### 1. 🗄️ MySQL配置（已安装）

**验证MySQL安装**：
```cmd
# 打开命令提示符，检查MySQL服务
sc query mysql80
# 或者
sc query mysql

# 启动MySQL服务（如果未启动）
net start mysql80
```

**创建数据库和用户**：
```cmd
# 以管理员身份打开命令提示符
cd /d "C:\Program Files\MySQL\MySQL Server 8.0\bin"

# 登录MySQL
mysql -u root -p

# 在MySQL中执行以下命令
```

```sql
-- 创建数据库
CREATE DATABASE IF NOT EXISTS qkchat CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- 创建用户
CREATE USER 'qkchat_user'@'localhost' IDENTIFIED BY 'QKchat2025!';
GRANT ALL PRIVILEGES ON qkchat.* TO 'qkchat_user'@'localhost';
FLUSH PRIVILEGES;

-- 使用数据库
USE qkchat;

-- 执行初始化脚本
SOURCE C:\path\to\your\project\server\data\mysql_init.sql;
```

### 2. �� Redis配置（已安装）

**验证Redis服务**：
```cmd
# 检查Redis服务状态
sc query redis

# 启动Redis服务（如果未启动）
net start redis

# 测试Redis连接
redis-cli ping
```

**配置Redis密码**：
```cmd
# 编辑Redis配置文件
notepad C:\Program Files\Redis\redis.windows.conf
```

在配置文件中添加：
```ini
# 设置密码
requirepass QKchat2025!

# 内存配置
maxmemory 256mb
maxmemory-policy allkeys-lru

# 持久化配置
save 900 1
save 300 10
save 60 10000
```

**重启Redis服务**：
```cmd
net stop redis
net start redis
```

### 3. �� SSL证书配置

**生成自签名证书**：
```cmd
# 打开PowerShell或命令提示符
cd C:\path\to\your\project\server

# 创建证书目录
mkdir certs

# 生成证书（使用OpenSSL）
openssl req -x509 -newkey rsa:4096 -keyout certs\server.key -out certs\server.crt -days 365 -nodes -subj "/C=CN/ST=Beijing/L=Beijing/O=QK Chat/OU=IT/CN=localhost"
```

**如果没有OpenSSL，下载并安装**：
1. 下载 [OpenSSL for Windows](https://slproweb.com/products/Win32OpenSSL.html)
2. 安装后添加到PATH环境变量
3. 重新打开命令提示符执行上述命令

### 4. 🌐 防火墙配置

**开放必要端口**：
```cmd
# 以管理员身份运行命令提示符

# 开放聊天服务器端口
netsh advfirewall firewall add rule name="QK Chat SSL" dir=in action=allow protocol=TCP localport=8443

# 开放管理界面端口
netsh advfirewall firewall add rule name="QK Chat Admin" dir=in action=allow protocol=TCP localport=8080

# 开放MySQL端口（如果需要远程访问）
netsh advfirewall firewall add rule name="MySQL" dir=in action=allow protocol=TCP localport=3306

# 开放Redis端口（如果需要远程访问）
netsh advfirewall firewall add rule name="Redis" dir=in action=allow protocol=TCP localport=6379
```

### 5. 📁 创建目录结构

```cmd
# 创建项目目录结构
cd C:\path\to\your\project

# 创建服务器目录
mkdir server\uploads
mkdir server\avatars
mkdir server\temp
mkdir server\logs

# 创建客户端目录
mkdir client\logs
mkdir client\cache
```

### 6. ⚙️ 更新配置文件

**服务器配置** (`server/config/dev.conf`)：
```ini
[Server]
host=localhost
port=8443
admin_port=8080
max_connections=1000
thread_pool_size=16

[Database]
type=mysql
host=localhost
port=3306
database=qkchat
username=qkchat_user
password=QKchat2024!
pool_size=10

[Security]
ssl_enabled=true
cert_file=certs/server.crt
key_file=certs/server.key
admin_username=admin
admin_password=QKchat2024!
session_timeout=1800

[Redis]
host=localhost
port=6379
password=QKchat2024!
database=0

[Logging]
level=info
file=logs/server.log
max_size=100MB
backup_count=5

[Email]
smtp_server=smtp.gmail.com
smtp_port=587
smtp_username=your-email@gmail.com
smtp_password=your-app-password
smtp_encryption=tls
from_email=noreply@qkchat.com
from_name=QK Chat
```

**客户端配置** (`client/config/dev.ini`)：
```ini
[Network]
server_host=localhost
server_port=8443
file_transfer_port=8444
connection_timeout=10000
reconnect_interval=5000
heartbeat_interval=30000

[UI]
theme=light
primary_color=#2196F3
accent_color=#FF4081
language=zh_CN
window_width=400
window_height=600

[Database]
cache_path=logs
message_limit=1000
max_cache_size=100MB

[Security]
remember_password=false
auto_login=false
encrypt_local_data=true

[Logging]
level=info
file=logs/client.log
max_size=50MB
backup_count=3
```

### 7. �� 构建和运行

**构建服务器**：
```cmd
cd C:\path\to\your\project\server
mkdir build
cd build

# 配置CMake（使用Qt Creator或命令行）
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:\Qt\6.5.3\mingw_64"

# 构建
mingw32-make

# 或者使用Qt Creator构建
```

**构建客户端**：
```cmd
cd C:\path\to\your\project\client
mkdir build
cd build

# 配置CMake
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="C:\Qt\6.5.3\mingw_64"

# 构建
mingw32-make
```

### 8. 🚀 运行服务

**启动服务器**：
```cmd
# 方法1：直接运行
cd C:\path\to\your\project\server\build
QKChatServer.exe

# 方法2：使用Qt Creator运行
```

**启动客户端**：
```cmd
# 方法1：直接运行
cd C:\path\to\your\project\client\build
QKChatClient.exe

# 方法2：使用Qt Creator运行
```

### 9. 📊 验证配置

**检查MySQL连接**：
```cmd
mysql -u qkchat_user -p
# 输入密码：QKchat2024!
# 执行：USE qkchat; SHOW TABLES;
```

**检查Redis连接**：
```cmd
redis-cli
# 执行：AUTH QKchat2024!
# 执行：PING
```

**检查SSL证书**：
```cmd
# 验证证书文件存在
dir C:\path\to\your\project\server\certs\server.crt
dir C:\path\to\your\project\server\certs\server.key
```

### 10. 🐛 故障排除

**常见问题解决**：

**Q: MySQL连接失败**
```cmd
# 检查MySQL服务
sc query mysql80

# 重启MySQL服务
net stop mysql80
net start mysql80

# 检查用户权限
mysql -u root -p
SELECT User, Host FROM mysql.user WHERE User='qkchat_user';
```

**Q: Redis连接失败**
```cmd
# 检查Redis服务
sc query redis

# 重启Redis服务
net stop redis
net start redis

# 测试连接
redis-cli -a QKchat2024! ping
```

**Q: SSL证书错误**
```cmd
# 重新生成证书
openssl req -x509 -newkey rsa:4096 -keyout certs\server.key -out certs\server.crt -days 365 -nodes -subj "/C=CN/ST=Beijing/L=Beijing/O=QK Chat/OU=IT/CN=localhost"
```

**Q: 防火墙阻止连接**
```cmd
# 临时关闭防火墙测试
netsh advfirewall set allprofiles state off

# 测试后重新启用
netsh advfirewall set allprofiles state on
```

### 11. �� Windows配置检查清单

- [ ] MySQL服务运行正常
- [ ] 数据库qkchat创建成功
- [ ] 用户qkchat_user创建并授权
- [ ] Redis服务运行正常
- [ ] Redis密码配置正确
- [ ] SSL证书生成成功
- [ ] 防火墙端口开放
- [ ] 项目目录结构创建
- [ ] 配置文件更新
- [ ] 服务器构建成功
- [ ] 客户端构建成功
- [ ] 服务器启动正常
- [ ] 客户端连接成功

### 12. �� 快速测试

**创建测试脚本** `test_config.bat`：
```batch
@echo off
echo 测试 QK Chat 配置...

echo 1. 检查MySQL服务...
sc query mysql80 | find "RUNNING"
if errorlevel 1 (
    echo MySQL服务未运行！
    pause
    exit /b 1
)

echo 2. 检查Redis服务...
sc query redis | find "RUNNING"
if errorlevel 1 (
    echo Redis服务未运行！
    pause
    exit /b 1
)

echo 3. 测试MySQL连接...
mysql -u qkchat_user -pQKchat2024! -e "USE qkchat; SHOW TABLES;" 2>nul
if errorlevel 1 (
    echo MySQL连接失败！
    pause
    exit /b 1
)

echo 4. 测试Redis连接...
redis-cli -a QKchat2024! ping 2>nul
if errorlevel 1 (
    echo Redis连接失败！
    pause
    exit /b 1
)

echo 5. 检查SSL证书...
if not exist "server\certs\server.crt" (
    echo SSL证书不存在！
    pause
    exit /b 1
)

echo 所有配置检查通过！
pause
```

按照这个Windows配置指南，您应该能够成功配置和运行QK Chat系统。如果在任何步骤遇到问题，请告诉我具体的错误信息！