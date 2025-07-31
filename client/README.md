# QK Chat 客户端

一个基于 Qt 6 和 QML 开发的现代化聊天客户端应用。

## 📋 项目概述

QK Chat 客户端是一个功能完整的即时通讯应用，采用现代化的设计理念和先进的技术架构。支持用户注册、登录、实时消息通信等功能，提供流畅的用户体验。

## ✨ 主要特性

### 🔐 用户认证系统
- **双登录方式**：支持用户名或邮箱登录
- **安全验证**：密码错误3次后需要验证码验证
- **账户保护**：智能锁定机制防止暴力破解
- **密码强度**：要求包含大小写字母和数字

### 👤 用户注册
- **邮箱注册**：使用邮箱作为主要注册方式
- **用户名验证**：支持中文、英文、数字和常见符号（3-20字符）
- **头像选择**：提供5个默认头像 + 自定义上传
- **图片处理**：智能裁剪为圆角正方形，支持JPG/PNG格式（最大2MB）

### 💬 实时通信
- **SSL加密**：TLS 1.3加密通信保障数据安全
- **实时消息**：即时发送和接收消息
- **在线状态**：实时显示用户在线状态
- **消息状态**：支持消息发送、投递、已读状态跟踪

### 🎨 用户界面
- **现代化设计**：Material Design风格界面
- **主题切换**：支持浅色和深色主题
- **响应式布局**：适配不同屏幕尺寸
- **实时验证**：表单输入实时验证反馈

### 🔧 技术特性
- **本地缓存**：SQLite数据库存储消息和用户信息
- **网络管理**：智能重连和心跳检测
- **配置管理**：应用设置持久化存储
- **错误处理**：完善的错误提示和恢复机制

## 🏗️ 技术架构

### 核心技术栈
- **Qt 6**：跨平台应用框架
- **QML**：声明式用户界面
- **C++**：高性能业务逻辑
- **SQLite**：本地数据存储
- **OpenSSL**：SSL/TLS加密通信

### 项目结构
```
client/
├── CMakeLists.txt          # CMake构建配置
├── src/                    # 源代码目录
│   ├── main.cpp           # 应用程序入口
│   ├── controllers/       # 控制器层
│   │   ├── UserController.h
│   │   └── UserController.cpp
│   ├── models/           # 数据模型层
│   │   ├── UserModel.h
│   │   └── UserModel.cpp
│   ├── database/         # 数据库层
│   │   ├── LocalDatabase.h
│   │   └── LocalDatabase.cpp
│   ├── network/          # 网络通信层
│   │   ├── NetworkClient.h
│   │   └── NetworkClient.cpp
│   ├── utils/            # 工具类
│   │   ├── Validator.h
│   │   └── Validator.cpp
│   └── config/           # 配置管理
│       ├── ConfigManager.h
│       └── ConfigManager.cpp
├── qml/                  # QML界面文件
│   ├── main.qml         # 主窗口
│   ├── LoginWindow.qml  # 登录界面
│   ├── RegisterWindow.qml # 注册界面
│   └── components/      # 可复用组件
│       ├── CustomButton.qml
│       ├── CustomTextField.qml
│       ├── ThemeManager.qml
│       └── AvatarSelector.qml
├── resources/            # 资源文件
│   └── icons/           # 图标资源
│       ├── avatar1.png
│       ├── avatar2.png
│       ├── avatar3.png
│       ├── avatar4.png
│       ├── avatar5.png
│       └── logo.png
└── config/              # 配置文件
    └── dev.ini         # 开发环境配置
```

## 🚀 快速开始

### 环境要求
- **Qt 6.5+**：Qt Core, QML, Quick, Network, Sql, QuickControls2
- **CMake 3.16+**：构建系统
- **C++17**：编译器支持
- **OpenSSL**：SSL/TLS支持

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

### 主题切换
- 点击界面右上角的主题切换按钮
- 支持浅色和深色主题
- 主题设置会自动保存

## 🔧 配置说明

### 网络配置 (`config/dev.ini`)
```ini
[Network]
server_host=localhost
server_port=8443
file_transfer_port=8444
connection_timeout=10000
reconnect_interval=5000
heartbeat_interval=30000
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

### 安全配置
```ini
[Security]
remember_password=false
auto_login=false
encrypt_local_data=true
```

## 🛠️ 开发指南

### 添加新功能
1. **业务逻辑**：在 `src/controllers/` 中添加控制器
2. **数据模型**：在 `src/models/` 中定义数据模型
3. **界面组件**：在 `qml/components/` 中创建QML组件
4. **网络通信**：在 `src/network/` 中扩展网络功能

### 代码规范
- **命名规范**：遵循Qt风格（PascalCase类名，camelCase方法名）
- **内存管理**：使用Qt对象树和智能指针
- **错误处理**：使用Qt的异常处理机制
- **日志记录**：使用QLoggingCategory进行结构化日志

## 📊 性能优化

### 数据库优化
- 使用索引优化查询性能
- 定期清理过期数据
- 批量操作减少I/O

### 网络优化
- 连接池管理
- 消息压缩
- 断点续传

### 内存优化
- 对象复用
- 延迟加载
- 缓存策略

## 🔒 安全特性

### 数据加密
- SSL/TLS 1.3加密通信
- 本地数据加密存储
- 密码SHA-256哈希

### 认证安全
- 会话令牌管理
- 账户锁定机制
- 验证码保护

### 隐私保护
- 本地数据隔离
- 敏感信息脱敏
- 日志安全记录

**版本**：1.0.0  
**最后更新**：2025年07月31日
**Qt版本**：6.5+ 