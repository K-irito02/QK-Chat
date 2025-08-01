# QK Chat 客户端

一个基于 Qt 6 和 QML 开发的现代化端到端聊天客户端应用。

## 📋 项目概述

QK Chat 客户端是一个功能完整的即时通讯应用，采用现代化的设计理念和先进的技术架构。支持用户注册、登录、实时消息通信、群组聊天、文件传输等功能，提供流畅的用户体验和强大的安全保护。

## ✨ 主要特性

### 🔐 安全加密系统
- **端到端加密**：所有消息使用 AES-256 加密
- **密钥管理**：支持 RSA-2048 和 ECC P-256 密钥对
- **前向保密**：定期密钥轮换机制
- **数字签名**：消息完整性验证
- **SSL/TLS 1.3**：安全传输层加密

### 👤 用户认证系统
- **双登录方式**：支持用户名或邮箱登录
- **安全验证**：密码错误3次后需要验证码验证
- **账户保护**：智能锁定机制防止暴力破解
- **密码强度**：要求包含大小写字母和数字
- **会话管理**：安全的会话令牌和自动续期

### 💬 实时通信
- **私聊消息**：一对一加密聊天
- **群组聊天**：支持最多 500 人的群组
- **消息状态**：发送中、已发送、已读状态显示
- **消息历史**：本地和云端消息同步
- **在线状态**：实时显示用户在线状态
- **正在输入**：显示对方正在输入状态

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
- **文件管理**：本地文件管理和清理

### 🎨 用户界面
- **现代化设计**：Material Design 风格界面
- **主题切换**：支持浅色和深色主题
- **响应式布局**：适配不同屏幕尺寸
- **实时验证**：表单输入实时验证反馈
- **表情系统**：丰富的表情包和自定义表情
- **语音消息**：录音和播放功能

### 🔧 技术特性
- **本地缓存**：SQLite数据库存储消息和用户信息
- **网络管理**：智能重连和心跳检测
- **配置管理**：应用设置持久化存储
- **错误处理**：完善的错误提示和恢复机制
- **性能优化**：多级缓存和数据库优化

## 🏗️ 技术架构

### 核心技术栈
- **Qt 6**：跨平台应用框架
- **QML**：声明式用户界面
- **C++17**：高性能业务逻辑
- **SQLite**：本地数据存储
- **OpenSSL**：SSL/TLS加密通信
- **Qt Multimedia**：音频和视频处理

### 项目结构
```
client/
├── CMakeLists.txt          # CMake构建配置
├── src/                    # 源代码目录
│   ├── main.cpp           # 应用程序入口
│   ├── controllers/       # 控制器层
│   │   ├── UserController.h
│   │   ├── UserController.cpp
│   │   ├── ChatController.h
│   │   └── ChatController.cpp
│   ├── models/           # 数据模型层
│   │   ├── UserModel.h
│   │   └── UserModel.cpp
│   ├── database/         # 数据库层
│   │   ├── LocalDatabase.h
│   │   └── LocalDatabase.cpp
│   ├── network/          # 网络通信层
│   │   ├── NetworkClient.h
│   │   └── NetworkClient.cpp
│   ├── crypto/           # 加密模块
│   │   ├── CryptoManager.h
│   │   └── CryptoManager.cpp
│   ├── utils/            # 工具类
│   │   ├── Validator.h
│   │   ├── Validator.cpp
│   │   ├── FileTransferManager.h
│   │   └── FileTransferManager.cpp
│   └── config/           # 配置管理
│       ├── ConfigManager.h
│       └── ConfigManager.cpp
├── qml/                  # QML界面文件
│   ├── main.qml         # 主窗口
│   ├── LoginWindow.qml  # 登录界面
│   ├── RegisterWindow.qml # 注册界面
│   ├── ChatMainWindow.qml # 聊天主窗口
│   └── components/      # 可复用组件
│       ├── CustomButton.qml
│       ├── CustomTextField.qml
│       ├── ThemeManager.qml
│       ├── AvatarSelector.qml
│       ├── ChatWindow.qml
│       ├── MessageBubble.qml
│       ├── EmojiPicker.qml
│       ├── ContactsPage.qml
│       ├── GroupsPage.qml
│       ├── ProfilePage.qml
│       └── SettingsPage.qml
├── icons/                # 图标资源
│   ├── avatar1.png
│   ├── avatar2.png
│   ├── avatar3.png
│   ├── avatar4.png
│   ├── avatar5.png
│   ├── logo.png
│   ├── emoji.png
│   ├── file.png
│   ├── voice.png
│   └── settings.png
├── config/              # 配置文件
│   └── dev.ini         # 开发环境配置
└── Resource.qrc        # Qt资源文件
```

## 🚀 快速开始

### 环境要求
- **Qt 6.5+**：Qt Core, Qml, Quick, Network, Sql, QuickControls2, Multimedia
- **CMake 3.16+**：构建系统
- **C++17**：编译器支持
- **OpenSSL 1.1.1+**：SSL/TLS支持

### 开发环境

#### Qt Creator
1. Qt Creator 16.0.2
2. Qt Quick Application
3. SQLite
4. Qt_6_5_3_MinGW_64_bit


## 📱 功能使用

### 用户注册
1. 点击"注册"按钮
2. 填写用户名（3-20字符，支持中文）
3. 输入邮箱地址
4. 选择头像（默认或自定义上传）
5. 设置密码（8-20字符，包含大小写字母和数字）
6. 确认密码
7. 点击"注册"完成

### 用户登录
1. 输入用户名或邮箱
2. 输入密码
3. 如密码错误3次，需要输入验证码
4. 可选择"记住密码"
5. 点击"登录"

### 聊天功能
1. **私聊**：点击联系人开始聊天
2. **群聊**：创建或加入群组
3. **表情**：点击表情按钮选择表情
4. **文件**：拖拽或点击发送文件
5. **语音**：长按录音按钮发送语音

### 群组管理
1. **创建群组**：点击"+"创建新群组
2. **邀请成员**：在群组中邀请好友
3. **权限管理**：设置成员角色和权限
4. **群组设置**：配置群组规则和公告

### 主题切换
- 点击界面右上角的主题切换按钮
- 支持浅色和深色主题
- 主题设置会自动保存

## 🔧 配置说明

### 网络配置 (`config/dev.ini`)
```ini
[Network]
server_host=localhost
server_port=8888
ssl_enabled=true
connection_timeout=10000
reconnect_interval=5000
heartbeat_interval=30000
```

### 数据库配置
```ini
[Database]
local_db_path=./data/local.db
cache_size=100MB
max_message_history=1000
auto_cleanup_days=30
```

### 安全配置
```ini
[Security]
encryption_enabled=true
key_rotation_interval=24h
remember_password=false
auto_login=false
encrypt_local_data=true
```

### UI配置
```ini
[UI]
theme=light
primary_color=#2196F3
accent_color=#FF4081
language=zh_CN
window_width=1200
window_height=800
```

## 🛠️ 开发指南

### 添加新功能
1. **业务逻辑**：在 `src/controllers/` 中添加控制器
2. **数据模型**：在 `src/models/` 中定义数据模型
3. **界面组件**：在 `qml/components/` 中创建QML组件
4. **网络通信**：在 `src/network/` 中扩展网络功能
5. **加密功能**：在 `src/crypto/` 中实现加密算法
6. **文件处理**：在 `src/utils/` 中添加工具类

### 代码规范
- **命名规范**：遵循Qt风格（PascalCase类名，camelCase方法名）
- **内存管理**：使用Qt对象树和智能指针
- **错误处理**：使用Qt的异常处理机制
- **日志记录**：使用QLoggingCategory进行结构化日志
- **线程安全**：使用QMutex和Qt::QueuedConnection

### 架构模式
- **MVC模式**：分离模型、视图、控制器
- **依赖注入**：降低模块间耦合
- **观察者模式**：事件驱动通信
- **工厂模式**：对象创建管理

## 📊 性能优化

### 数据库优化
- 使用索引优化查询性能
- 定期清理过期数据
- 批量操作减少I/O
- 连接池管理

### 网络优化
- 连接池管理
- 消息压缩
- 断点续传
- 智能重连机制

### 内存优化
- 对象复用
- 延迟加载
- 缓存策略
- 智能指针管理

### 缓存系统
- 多级缓存策略
- LRU/LFU淘汰算法
- 内存限制管理
- 缓存持久化

## 🔒 安全特性

### 数据加密
- **端到端加密**：AES-256-GCM对称加密
- **密钥交换**：RSA-2048/ECC P-256非对称加密
- **数字签名**：SHA-256哈希和RSA签名
- **前向保密**：定期密钥轮换
- **本地加密**：SQLite数据库加密存储

### 认证安全
- **会话令牌**：JWT令牌管理
- **账户锁定**：智能锁定机制
- **验证码保护**：图形验证码
- **密码强度**：复杂度验证

### 隐私保护
- **本地数据隔离**：沙盒存储
- **敏感信息脱敏**：日志安全记录
- **权限控制**：最小权限原则
- **数据清理**：自动清理过期数据

---

**QK Chat 客户端** - 安全、高效、现代化的聊天体验 🚀

**版本**：1.0.0  
**最后更新**：2025年08月02日  
**Qt版本**：6.5+  
**C++标准**：C++17 