# QKChat - 聊天应用

## 📖 项目简介

QKChat 是一个基于 Qt6 和 QML 开发的现代化端到端聊天应用。支持私聊、群聊、文件传输、语音消息等功能，采用MVC架构设计，确保可扩展性和可维护性。

## ✨ 主要特性

### 🔐 安全加密
- **端到端加密**：所有消息使用 AES-256 加密
- **密钥管理**：支持 RSA-2048 和 ECC P-256 密钥对
- **前向保密**：定期密钥轮换机制
- **数字签名**：消息完整性验证
- **安全传输**：SSL/TLS 加密通信

### 💬 聊天功能
- **私聊消息**：一对一加密聊天
- **群组聊天**：支持最多 500 人的群组
- **消息状态**：发送中、已发送、已读状态显示
- **消息历史**：本地和云端消息同步
- **表情系统**：丰富的表情包和自定义表情
- **语音消息**：录音和播放功能

### 👥 用户管理
- **用户注册/登录**：支持用户名和邮箱登录
- **个人资料**：头像、昵称、个性签名
- **联系人管理**：添加、删除、分组联系人
- **在线状态**：实时在线状态显示
- **好友系统**：好友申请和验证

### 🗂️ 群组功能
- **群组创建**：创建和管理群组
- **权限管理**：群主、管理员、成员三级权限
- **群组设置**：入群验证、全员禁言等
- **成员管理**：邀请、移除、角色变更
- **群组公告**：群组公告和置顶消息

### 📁 文件传输
- **多文件类型**：图片、文档、音频、视频
- **断点续传**：支持大文件断点续传
- **传输进度**：实时传输进度显示
- **文件预览**：图片和文档预览
- **安全存储**：加密文件存储

### 🎨 用户界面
- **现代化设计**：Material Design 风格
- **响应式布局**：适配不同屏幕尺寸
- **主题切换**：深色/浅色主题
- **自定义外观**：个性化界面设置
- **多语言支持**：国际化界面

### ⚡ 性能优化
- **缓存系统**：多级缓存策略
- **数据库优化**：智能索引和查询优化
- **内存管理**：高效内存使用
- **并发处理**：多线程异步操作
- **网络优化**：连接池和重连机制

## 🏗️ 技术架构

### 客户端架构
```
QKChatClient/
├── src/
│   ├── controllers/     # 控制器层
│   │   ├── UserController.cpp
│   │   └── ChatController.cpp
│   ├── models/         # 数据模型
│   │   └── UserModel.cpp
│   ├── network/        # 网络通信
│   │   └── NetworkClient.cpp
│   ├── database/       # 本地数据库
│   │   └── LocalDatabase.cpp
│   ├── crypto/         # 加密模块
│   │   └── CryptoManager.cpp
│   ├── utils/          # 工具类
│   │   ├── Validator.cpp
│   │   └── FileTransferManager.cpp
│   └── config/         # 配置管理
│       └── ConfigManager.cpp
├── qml/               # QML界面
│   ├── main.qml
│   ├── LoginWindow.qml
│   ├── ChatMainWindow.qml
│   └── components/
└── icons/             # 图标资源
```

### 服务器架构
```
QKChatServer/
├── src/
│   ├── core/          # 核心服务
│   │   ├── ChatServer.cpp
│   │   ├── SessionManager.cpp
│   │   └── GroupManager.cpp
│   ├── database/      # 数据库层
│   │   ├── Database.cpp
│   │   └── DatabaseOptimizer.cpp
│   ├── cache/         # 缓存系统
│   │   └── CacheManager.cpp
│   ├── network/       # 网络协议
│   │   └── ProtocolParser.cpp
│   ├── admin/         # 管理界面
│   │   ├── AdminWindow.cpp
│   │   └── DashboardWidget.cpp
│   └── utils/         # 工具类
│       └── AdminAuth.cpp
├── config/            # 配置文件
└── data/              # 数据文件
```

### 系统要求

- **操作系统**：Windows 10+, macOS 10.15+, Ubuntu 20.04+
- **Qt版本**：Qt 6.0 或更高版本
- **编译器**：支持 C++17 的编译器
- **内存**：至少 4GB RAM
- **存储**：至少 2GB 可用空间

### 依赖项

#### 必需依赖
- Qt6 Core, Qml, Quick, Network, Sql
- OpenSSL 1.1.1+ (用于加密功能)
- CMake 3.16+

#### 可选依赖
- MySQL 8.0+ (服务器端)
- Redis 6.0+ (缓存服务)


## 🔧 配置说明

### 客户端配置
配置文件：`client/config/dev.ini`
```ini
[Network]
server_host=localhost
server_port=8888
ssl_enabled=true

[Database]
local_db_path=./data/local.db
cache_size=100MB

[Security]
encryption_enabled=true
key_rotation_interval=24h
```

### 服务器配置
配置文件：`server/config/dev.conf`
```ini
[Server]
host=0.0.0.0
port=8888
max_connections=1000
ssl_certificate=./config/ssl/cert.pem
ssl_private_key=./config/ssl/key.pem

[Database]
host=localhost
port=3306
database=qkchat_db
username=qkchat_user
password=your_password

[Cache]
redis_host=localhost
redis_port=6379
max_memory=512MB
```


## 📚 开发指南

### 代码规范
- 遵循 Qt 开发规范
- 使用 C++17 标准
- 类名使用 PascalCase
- 函数名使用 camelCase
- 私有成员使用下划线前缀

### 架构模式
- **MVC模式**：分离模型、视图、控制器
- **依赖注入**：降低模块间耦合
- **观察者模式**：事件驱动通信
- **工厂模式**：对象创建管理

### 性能优化
- 使用智能指针管理内存
- 实现连接池减少资源消耗
- 采用缓存策略提升响应速度
- 异步处理避免阻塞

## 🔒 安全特性

### 加密算法
- **对称加密**：AES-256-GCM
- **非对称加密**：RSA-2048, ECC P-256
- **哈希算法**：SHA-256
- **密钥派生**：PBKDF2

### 安全措施
- 端到端加密保护消息内容
- 数字签名确保消息完整性
- 前向保密防止密钥泄露
- 安全随机数生成
- 输入验证和SQL注入防护