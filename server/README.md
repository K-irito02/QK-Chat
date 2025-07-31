# QK Chat 服务器端

一个基于 Qt 6 和 C++ 开发的高性能聊天服务器，提供完整的即时通讯后端服务和管理界面。

## 📋 项目概述

QK Chat 服务器端是一个功能完整的即时通讯后端系统，采用现代化的微服务架构设计。支持用户认证、实时消息路由、会话管理、文件传输等功能，并提供友好的管理界面进行服务器监控和用户管理。

## ✨ 主要特性

### 🔐 安全认证系统
- **SSL/TLS加密**：TLS 1.3加密通信保障数据安全
- **用户认证**：支持用户名/邮箱 + 密码登录
- **会话管理**：基于令牌的会话验证机制
- **管理员认证**：独立的管理员登录系统，支持账户锁定

### 💬 实时通信引擎
- **高并发支持**：基于线程池的并发连接处理
- **消息路由**：智能消息转发和投递机制
- **在线状态**：实时用户在线状态跟踪
- **心跳检测**：连接状态监控和自动清理

### 🛠️ 管理界面
- **可视化监控**：实时服务器状态和性能监控
- **用户管理**：用户信息查看、编辑、删除
- **系统配置**：服务器参数动态配置
- **日志查看**：系统日志实时查看和过滤

### 🗄️ 数据管理
- **MySQL数据库**：用户数据、消息、会话存储
- **Redis缓存**：验证码、热点数据缓存
- **数据备份**：自动数据备份和恢复机制
- **性能优化**：数据库索引和查询优化

### 🔧 系统特性
- **跨平台支持**：Windows、Linux、macOS
- **模块化设计**：可扩展的插件架构
- **配置管理**：灵活的配置文件系统
- **日志系统**：结构化日志记录和分析

## 🏗️ 技术架构

### 核心技术栈
- **Qt 6.5+**：跨平台应用框架
- **C++17**：高性能业务逻辑
- **MySQL**：关系型数据库
- **Redis**：内存缓存数据库
- **OpenSSL**：SSL/TLS加密通信
- **QSslServer**：SSL服务器实现

### 项目结构
```
server/
├── CMakeLists.txt          # CMake构建配置
├── src/                    # 源代码目录
│   ├── main.cpp           # 应用程序入口
│   ├── core/              # 核心服务层
│   │   ├── ChatServer.h
│   │   ├── ChatServer.cpp
│   │   ├── SessionManager.h
│   │   └── SessionManager.cpp
│   ├── admin/             # 管理界面层
│   │   ├── AdminWindow.h
│   │   ├── AdminWindow.cpp
│   │   ├── LoginDialog.h
│   │   ├── LoginDialog.cpp
│   │   ├── LoginDialog.ui
│   │   ├── DashboardWidget.h
│   │   ├── UserManagerWidget.h
│   │   ├── SystemConfigWidget.h
│   │   ├── LogViewerWidget.h
│   │   └── MonitorWidget.h
│   ├── database/          # 数据库层
│   │   ├── Database.h
│   │   └── Database.cpp
│   ├── network/           # 网络通信层
│   │   ├── ProtocolParser.h
│   │   └── ProtocolParser.cpp
│   ├── utils/             # 工具类
│   │   ├── AdminAuth.h
│   │   └── AdminAuth.cpp
│   └── config/            # 配置管理
│       ├── ServerConfig.h
│       └── ServerConfig.cpp
├── data/                  # 数据文件
│   └── mysql_init.sql    # MySQL数据库初始化脚本
├── config/               # 配置文件
│   └── dev.conf         # 开发环境配置
├── certs/               # SSL证书目录
│   ├── server.crt      # 服务器证书
│   └── server.key      # 服务器私钥
└── resources/           # 资源文件
    ├── qml/            # QML管理界面组件
    └── icons/          # 管理界面图标
```

## 🚀 快速开始

### 环境要求
- **Qt 6.5+**：Qt Core, Widgets, Network, Sql, Qml, Quick
- **CMake 3.16+**：构建系统
- **C++17**：编译器支持
- **MySQL 8.0+**：数据库服务器
- **Redis 6.0+**：缓存服务器
- **OpenSSL**：SSL/TLS支持

### 开发环境

#### Qt Creator
1. Qt Creator 16.0.2
2. Qt Widgets Application
3. MySQL 8.0
4. Redis 6.0
5. Qt_6_5_3_MinGW_64_bit

### 安装步骤

1. **安装依赖**
```bash
# Ubuntu/Debian
sudo apt-get install mysql-server redis-server libssl-dev

# CentOS/RHEL
sudo yum install mysql-server redis openssl-devel

# Windows
# 下载并安装 MySQL 和 Redis
```

2. **配置数据库**
```bash
# 启动MySQL服务
sudo systemctl start mysql

# 创建数据库
mysql -u root -p < data/mysql_init.sql
```

3. **配置Redis**
```bash
# 启动Redis服务
sudo systemctl start redis

# 测试连接
redis-cli ping
```

4. **构建项目**
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

5. **运行服务器**
```bash
./QKChatServer
```

## 📊 系统配置

### 服务器配置 (`config/dev.conf`)
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
password=qkchat_pass
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
password=
database=0

[Logging]
level=info
file=logs/server.log
max_size=100MB
backup_count=5
```

### SSL证书配置
```bash
# 生成自签名证书（开发环境）
openssl req -x509 -newkey rsa:4096 -keyout certs/server.key -out certs/server.crt -days 365 -nodes
```

## 🔧 管理界面使用

### 管理员登录
1. 启动服务器后，自动显示登录对话框
2. 输入管理员账号密码（默认：admin/QKchat2024!）
3. 登录成功后进入主管理界面

### 功能模块

#### 📊 仪表板
- 实时服务器状态监控
- 在线用户统计
- 系统资源使用情况
- 消息流量统计

#### 👥 用户管理
- 用户列表查看
- 用户信息编辑
- 用户状态管理
- 用户搜索和过滤

#### ⚙️ 系统配置
- 服务器参数配置
- 数据库连接设置
- 安全策略配置
- 日志级别调整

#### 📋 日志查看
- 系统日志实时查看
- 日志级别过滤
- 日志搜索功能
- 日志导出功能

#### 📈 实时监控
- 连接数监控
- 消息流量监控
- 系统性能监控
- 异常告警功能

## 🔒 安全特性

### 通信安全
- **SSL/TLS 1.3**：端到端加密通信
- **证书验证**：服务器证书验证
- **会话管理**：基于令牌的安全会话

### 数据安全
- **密码哈希**：SHA-256密码存储
- **数据加密**：敏感数据加密存储
- **访问控制**：基于角色的权限管理

### 系统安全
- **账户锁定**：管理员账户保护
- **日志审计**：完整的操作日志记录
- **防火墙**：网络访问控制

## 📊 性能优化

### 数据库优化
- 连接池管理
- 查询优化和索引
- 读写分离
- 数据分片

### 网络优化
- 连接复用
- 消息压缩
- 负载均衡
- 缓存策略

### 内存优化
- 对象池管理
- 内存泄漏检测
- 垃圾回收
- 缓存清理

## 🐛 故障排除

### 调试模式
```bash
# 启用详细日志
export QT_LOGGING_RULES="qkchat.*=true"
./QKChatServer

# 启用SSL调试
export QT_LOGGING_RULES="qkchat.*=true"
export QT_LOGGING_RULES="qt.network.ssl.*=true"
./QKChatServer
```

### 日志分析
```bash
# 查看实时日志
tail -f logs/server.log

# 搜索错误日志
grep "ERROR" logs/server.log

# 分析连接日志
grep "client connected" logs/server.log
```

## 📈 监控和维护

### 系统监控
- **CPU使用率**：实时CPU监控
- **内存使用**：内存占用统计
- **网络流量**：网络I/O监控
- **磁盘使用**：存储空间监控

### 性能指标
- **并发连接数**：当前活跃连接数
- **消息吞吐量**：每秒处理消息数
- **响应时间**：平均响应时间
- **错误率**：系统错误统计

### 维护任务
- **日志轮转**：自动日志文件轮转
- **数据备份**：定期数据库备份
- **证书更新**：SSL证书到期提醒
- **系统更新**：安全补丁更新

## 🔧 开发指南

### 添加新功能
1. **业务逻辑**：在 `src/core/` 中添加核心服务
2. **管理界面**：在 `src/admin/` 中添加管理组件
3. **数据库操作**：在 `src/database/` 中扩展数据访问
4. **网络协议**：在 `src/network/` 中扩展协议支持

### 代码规范
- **命名规范**：遵循Qt风格（PascalCase类名，camelCase方法名）
- **内存管理**：使用Qt对象树和智能指针
- **错误处理**：使用Qt的异常处理机制
- **日志记录**：使用QLoggingCategory进行结构化日志

**版本**：1.0.0  
**最后更新**：2025年07月31日  
**Qt版本**：6.5+ 