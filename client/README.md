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
- **诊断工具**：内置连接监控和诊断功能

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
├── CMakeLists.txt.user     # Qt Creator用户配置
├── README.md               # 项目说明文档
├── Resource.qrc            # Qt资源文件
├── src/                    # 源代码目录
│   ├── main.cpp           # 应用程序入口
│   ├── controllers/       # 控制器层
│   │   ├── ChatController.cpp
│   │   ├── ChatController.h
│   │   ├── UserController.cpp
│   │   └── UserController.h
│   ├── models/           # 数据模型层
│   │   ├── UserModel.cpp
│   │   └── UserModel.h
│   ├── database/         # 数据库层
│   │   ├── LocalDatabase.cpp
│   │   └── LocalDatabase.h
│   ├── network/          # 网络通信层
│   │   ├── NetworkClient.cpp
│   │   ├── NetworkClient.h
│   │   ├── ConnectionPool.cpp
│   │   ├── ConnectionPool.h
│   │   ├── ConnectionStateManager.cpp
│   │   ├── ConnectionStateManager.h
│   │   ├── ErrorHandler.cpp
│   │   ├── ErrorHandler.h
│   │   ├── HeartbeatManager.cpp
│   │   ├── HeartbeatManager.h
│   │   ├── ReconnectManager.cpp
│   │   ├── ReconnectManager.h
│   │   ├── SSLConfigManager.cpp
│   │   └── SSLConfigManager.h
│   ├── crypto/           # 加密模块
│   │   ├── CryptoManager.cpp
│   │   └── CryptoManager.h
│   ├── config/           # 配置管理
│   │   ├── ConfigManager.cpp
│   │   ├── ConfigManager.h
│   │   ├── DevelopmentConfig.cpp
│   │   └── DevelopmentConfig.h
│   ├── monitoring/       # 监控和诊断
│   │   ├── ConnectionMonitor.cpp
│   │   ├── ConnectionMonitor.h
│   │   ├── DiagnosticTool.cpp
│   │   └── DiagnosticTool.h
│   └── utils/            # 工具类
│       ├── FileTransferManager.cpp
│       ├── FileTransferManager.h
│       ├── ThreadPool.cpp
│       ├── ThreadPool.h
│       ├── Validator.cpp
│       ├── Validator.h
│       ├── LogManager.cpp
│       ├── LogManager.h
│       ├── LogViewer.h
│       ├── MonitorManager.cpp
│       ├── MonitorManager.h
│       └── DiagnosticManager.h
├── qml/                  # QML界面文件
│   ├── ChatMainWindow.qml # 聊天主窗口

│   ├── LoginWindow.qml  # 登录界面
│   ├── RegisterWindow.qml # 注册界面
│   ├── main.qml         # 主窗口
│   └── components/      # 可复用组件
│       ├── AddPage.qml        # 添加页
│       ├── AvatarSelector.qml # 头像选择器
│       ├── ChatWindow.qml     # 聊天窗口
│       ├── ContactsPage.qml   # 联系人页
│       ├── CustomButton.qml   # 自定义按钮
│       ├── CustomTextField.qml # 自定义输入框
│       ├── DefaultPage.qml    # 默认页
│       ├── EmojiPicker.qml    # 表情选择器
│       ├── GroupsPage.qml     # 群组页
│       ├── MessageBubble.qml  # 消息气泡
│       ├── ProfilePage.qml    # 个人资料页
│       ├── SettingsPage.qml   # 设置页
│       └── SideBarButton.qml  # 侧边栏按钮
├── icons/                # 图标资源
│   ├── add-contact.png   # 添加联系人
│   ├── add.png          # 添加图标
│   ├── add.svg          # 添加SVG图标
│   ├── arrow-down.png   # 向下箭头
│   ├── arrow-right.png  # 向右箭头
│   ├── attach.png       # 附件图标
│   ├── avatar1.png      # 头像1
│   ├── avatar2.png      # 头像2
│   ├── avatar3.png      # 头像3
│   ├── avatar4.png      # 头像4
│   ├── avatar5.png      # 头像5
│   ├── captcha.png      # 验证码图标
│   ├── chat-empty.png   # 空聊天状态
│   ├── chat.png         # 聊天图标
│   ├── contacts-empty.png # 空联系人状态
│   ├── create-group.png # 创建群组
│   ├── delete.png       # 删除图标
│   ├── edit.png         # 编辑图标

│   ├── email.png        # 邮箱图标
│   ├── emoji.png        # 表情图标
│   ├── exit.png         # 退出图标
│   ├── eye-off.png      # 隐藏密码
│   ├── eye.png          # 显示密码
│   ├── file.png         # 文件图标
│   ├── group.png        # 群组图标
│   ├── groups-empty.png # 空群组状态
│   ├── home.png         # 首页图标
│   ├── info.png         # 信息图标
│   ├── invite.png       # 邀请图标
│   ├── join-group.png   # 加入群组
│   ├── keyboard.png     # 键盘图标
│   ├── lock.png         # 锁图标
│   ├── logo.png         # 应用Logo
│   ├── message-sending.png # 消息发送中
│   ├── message-sent.png # 消息已发送
│   ├── message-settings.png # 消息设置
│   ├── mic.png          # 麦克风图标
│   ├── moon.png         # 深色主题
│   ├── more.png         # 更多选项
│   ├── phone.png        # 电话图标
│   ├── privacy.png      # 隐私图标
│   ├── profile.png      # 个人资料图标
│   ├── search.png       # 搜索图标
│   ├── send.png         # 发送图标
│   ├── settings.png     # 设置图标
│   ├── sun.png          # 浅色主题
│   ├── user.png         # 用户图标
│   └── video.png        # 视频图标
├── config/              # 配置文件
│   ├── dev.ini         # 开发环境配置
│   └── logging.conf    # 日志配置
├── tests/              # 测试文件
│   └── DiagnosticToolTest.cpp # 诊断工具测试
└── build/              # 构建输出目录
```

## 🚀 快速开始

### 环境要求
- **Qt 6.5+**：Qt Core, Qml, Quick, Network, Sql, QuickControls2, Multimedia, Concurrent, OpenGL, StateMachine
- **CMake 3.16+**：构建系统
- **C++17**：编译器支持
- **OpenSSL 1.1.1+**：SSL/TLS支持

### 开发环境

#### Qt Creator
1. **Qt Creator**: 16.0.2 或更高版本
2. **Qt版本**: Qt 6.5.3 MinGW 64-bit
3. **编译器**: MinGW 64-bit
4. **构建系统**: CMake 3.16+
5. **调试器**: GDB (MinGW)

#### 必需模块
- Qt6::Core
- Qt6::Qml
- Qt6::Quick
- Qt6::Network
- Qt6::Sql
- Qt6::QuickControls2
- Qt6::Concurrent
- Qt6::Multimedia
- Qt6::OpenGL
- Qt6::StateMachine

#### 可选依赖
- OpenSSL 1.1.1+ (SSL/TLS支持)
- SQLite3 (本地数据库)

## 📱 功能使用

### 用户注册
1. 点击"注册"按钮进入注册页面
2. 填写用户名（3-20字符，支持中文）
3. 输入有效的邮箱地址
4. 选择头像（5个预设头像可选）
5. 设置密码（8-20字符，包含大小写字母和数字）
6. 确认密码
7. 点击"注册"完成账户创建

### 用户登录
1. 输入用户名或邮箱地址
2. 输入账户密码
3. 如密码错误3次，需要输入图形验证码
4. 可选择"记住密码"功能
5. 点击"登录"进入主界面

### 主界面导航
- **首页**: 显示最近聊天和快捷功能
- **聊天**: 私聊和群聊消息列表
- **联系人**: 好友列表和搜索功能
- **群组**: 群组列表和管理功能
- **个人**: 个人资料和设置选项

### 聊天功能
1. **私聊**: 点击联系人开始一对一聊天
2. **群聊**: 创建或加入群组进行多人聊天
3. **表情**: 点击表情按钮选择表情包
4. **文件**: 拖拽或点击附件按钮发送文件
5. **语音**: 长按麦克风按钮录制语音消息
6. **图片**: 支持图片预览和发送
7. **视频**: 支持视频通话功能

### 消息功能
- **消息状态**: 发送中、已发送、已读状态显示
- **消息搜索**: 支持消息内容搜索
- **消息撤回**: 支持2分钟内撤回消息
- **消息转发**: 支持消息转发给其他联系人
- **消息引用**: 支持回复特定消息

### 群组管理
1. **创建群组**: 点击"+"创建新群组
2. **邀请成员**: 在群组中邀请好友加入
3. **权限管理**: 设置群主、管理员、成员角色
4. **群组设置**: 配置群组规则、公告和隐私设置
5. **群组公告**: 发布和管理群组公告

### 主题切换
- 点击界面右上角的主题切换按钮
- 支持浅色和深色主题自动切换
- 主题设置会自动保存到本地
- 支持自定义主题颜色

## 🔧 配置说明

### 网络配置 (`config/dev.ini`)
```ini
[Network]
server_host=localhost
server_port=8443
file_transfer_port=8444
timeout=30000
auto_reconnect=true
heartbeat_interval=30000
```

### 数据库配置
```ini
[Database]
cache_path=database/local_cache.db
max_messages=10000
cleanup_days=90
```

### 安全配置
```ini
[Security]
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
window_width=400
window_height=600
```

### 日志配置
```ini
[Logging]
level=INFO
file_path=logs/client.log
max_file_size=5MB
max_files=3
```

## 🛠️ 开发指南

### 添加新功能
1. **业务逻辑**：在 `src/controllers/` 中添加控制器
2. **数据模型**：在 `src/models/` 中定义数据模型
3. **界面组件**：在 `qml/components/` 中创建QML组件
4. **网络通信**：在 `src/network/` 中扩展网络功能
5. **加密功能**：在 `src/crypto/` 中实现加密算法
6. **文件处理**：在 `src/utils/` 中添加工具类
7. **监控诊断**：在 `src/monitoring/` 中添加诊断功能

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
- 心跳检测

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

## 🔍 诊断和监控

### 连接监控
- **实时连接状态**：监控网络连接状态
- **连接质量**：延迟和丢包率检测
- **自动重连**：智能重连机制
- **错误诊断**：详细的错误信息记录

### 性能监控
- **内存使用**：实时内存使用监控
- **CPU使用率**：性能瓶颈检测
- **网络流量**：数据传输统计
- **响应时间**：操作响应时间监控

### 日志系统
- **分级日志**：DEBUG、INFO、WARNING、ERROR
- **日志轮转**：自动日志文件管理
- **日志查看器**：内置日志查看工具
- **远程日志**：支持远程日志收集

---

**QK Chat 客户端** - 安全、高效、现代化的聊天体验 🚀

**版本**：1.0.0  
**最后更新**：2025年08月04日  
**Qt版本**：6.5+  
**C++标准**：C++17