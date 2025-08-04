# QK Chat 服务器端

一个基于 Qt 6 和 C++ 开发的聊天服务器，提供完整的即时通讯后端服务和管理界面。

## 📋 项目概述

QK Chat 服务器端是一个功能完整的即时通讯后端系统，采用现代化的架构设计。支持用户认证、实时消息路由、会话管理等功能，并提供友好的管理界面进行服务器监控和用户管理。

## 🎯 主要特性

### 🏗️ 架构设计
- **多级缓存系统**: 实现了内存缓存和智能缓存策略
- **异步处理**: 网络事件和消息的异步处理
- **线程管理**: 专用线程池架构
- **自动恢复**: 故障检测和恢复机制

### � 安全认证系统
- **SSL/TLS加密**: TLS 1.3加密通信保障数据安全
- **用户认证**: 支持用户名/邮箱 + 密码登录
- **会话管理**: 基于令牌的会话验证机制
- **管理员认证**: 独立的管理员登录系统，支持账户锁定

### 💬 实时通信引擎
- **高并发支持**: 基于线程池的并发连接处理
- **消息路由**: 智能消息转发和投递机制
- **在线状态**: 实时用户在线状态跟踪
- **心跳检测**: 连接状态监控和自动清理

### �️ 管理界面
- **可视化监控**: 实时服务器状态和性能监控
- **用户管理**: 用户信息查看、编辑、删除
- **系统配置**: 服务器参数动态配置
- **日志查看**: 系统日志实时查看和过滤

### 🗄️ 数据管理
- **MySQL数据库**: 用户数据、消息、会话存储
- **Redis缓存**: 验证码、热点数据缓存
- **多级缓存**: 内存缓存和智能缓存策略
- **数据备份**: 自动数据备份和恢复机制
- **性能优化**: 数据库索引和查询优化

### 🔧 系统特性
- **跨平台支持**: Windows、Linux、macOS
- **模块化设计**: 可扩展的组件架构
- **配置管理**: 灵活的配置文件系统
- **日志系统**: 结构化日志记录和分析
- **自动恢复**: 故障检测和自动恢复机制

## 🏗️ 技术架构

### 核心技术栈
- **Qt 6.5+**: 跨平台应用框架
- **C++17**: 高性能业务逻辑
- **MySQL 8.0+**: 关系型数据库
- **Redis 6.0+**: 内存缓存数据库
- **OpenSSL**: SSL/TLS加密通信
- **CMake**: 构建系统

### 项目结构
```
server/
├── CMakeLists.txt          # CMake构建配置
├── README.md               # 项目说明文档
├── src/                    # 源代码目录
│   ├── admin/             # 管理界面层
│   │   ├── AdminWindow.*
│   │   ├── DashboardWidget.*
│   │   └── LoginDialog.*
│   ├── cache/             # 缓存管理层
│   │   ├── CacheManager.*
│   │   ├── MultiLevelCache.*
│   │   ├── CacheStrategyManager.*
│   │   └── CachePreloader.*
│   ├── config/            # 配置管理
│   │   └── ServerConfig.*
│   ├── core/              # 核心服务层
│   │   ├── ChatServer.*
│   │   ├── ConnectionManager.*
│   │   ├── SessionManager.*
│   │   ├── GroupManager.*
│   │   ├── MessageEngine.*
│   │   ├── MessageHandlers.*
│   │   └── ThreadManager.*
│   ├── crypto/            # 加密模块
│   │   └── CryptoManager.*
│   ├── database/          # 数据库层
│   │   ├── Database.*
│   │   ├── DatabasePool.*
│   │   └── DatabaseOptimizer.*
│   ├── network/           # 网络通信层
│   │   ├── QSslServer.*
│   │   ├── ProtocolParser.*
│   │   └── NetworkEventHandler.*
│   ├── services/          # 服务层
│   │   ├── EmailService.*
│   │   └── EmailTemplate.*
│   ├── utils/             # 工具类
│   │   ├── LockFreeStructures.h
│   │   ├── AutoRecovery.*
│   │   ├── SystemMonitor.h
│   │   ├── PerformanceProfiler.h
│   │   ├── LogManager.*
│   │   ├── AdminAuth.*
│   │   ├── AdminManager.*
│   │   └── ThreadPool.*
│   └── main.cpp           # 应用程序入口
├── tests/                 # 测试套件
│   ├── CacheSystemTest.h
│   └── PerformanceTest.h
├── config/               # 配置文件
│   ├── dev.conf         # 开发环境配置
│   ├── database_config.sql
│   ├── redis.conf
│   └── ssl_config.cnf
├── data/                 # 数据文件
│   └── mysql_init.sql
└── certs/               # SSL证书
    ├── server.crt
    └── server.key
```

## 🚀 快速开始

### 环境要求
- **Qt 6.5+**: Qt Core, Widgets, Network, Sql
- **CMake 3.16+**: 构建系统
- **C++17**: 编译器支持
- **MySQL 8.0+**: 数据库服务器
- **Redis 6.0+**: 缓存服务器
- **OpenSSL**: SSL/TLS支持

### 开发环境

#### Qt Creator
1. **Qt Creator**: 16.0.2 或更高版本
2. **Qt版本**: Qt 6.5.3 MinGW 64-bit
3. **编译器**: MinGW 64-bit
4. **构建系统**: CMake 3.16+
5. **调试器**: GDB (MinGW)

#### 必需模块
- Qt6::Core
- Qt6::Widgets
- Qt6::Network
- Qt6::Sql

#### 数据库依赖
- **MySQL**: 8.0+ (主数据库)
- **Redis**: 6.0+ (缓存服务)
- **SQLite**: 3.35+ (本地配置存储)

#### 可选依赖
- OpenSSL 1.1.1+ (SSL/TLS支持)
- Zlib (数据压缩)

### 构建和运行

#### 本地开发
```bash
# 克隆项目
git clone https://github.com/K-irito02/QK-Chat.git
cd QK-Chat/server

# 构建项目
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make

# 启动服务
./QKChatServer
```

#### 生产环境
```bash
# 构建发布版本
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# 启动服务
./QKChatServer
```


## ⚙️ v2.0 系统配置

### 🔧 服务器配置 (`config/dev.conf`)
```ini
[Server]
host=localhost
port=8443
admin_port=8080
max_connections=5000              # v2.0: 提升至5000
thread_pool_size=32               # v2.0: 增加线程池大小
heartbeat_interval=30000
connection_timeout=10000
enable_performance_mode=true      # v2.0: 性能模式

[ThreadManager]                   # v2.0: 新增线程管理配置
network_threads=8                 # 网络处理线程
database_threads=4                # 数据库处理线程
service_threads=8                 # 业务逻辑线程
cache_threads=4                   # 缓存处理线程
admin_threads=2                   # 管理界面线程

[Database]
type=mysql
host=localhost
port=3306
database=qkchat
username=qkchat_user
password=qkchat_pass
pool_size=20                      # v2.0: 增加连接池
connection_timeout=5000
max_retry_attempts=3
enable_read_write_split=true      # v2.0: 读写分离
read_host=localhost               # v2.0: 读库地址
write_host=localhost              # v2.0: 写库地址

[Cache]                           # v2.0: 新增多级缓存配置
l1_enabled=true                   # L1内存缓存
l1_max_size=256MB
l1_max_items=100000
l2_enabled=true                   # L2本地缓存
l2_max_size=1GB
l2_path=cache/l2
l3_enabled=true                   # L3分布式缓存
l3_redis_cluster=true
enable_preloading=true            # 智能预加载
enable_adaptive=true              # 自适应优化

[Security]
ssl_enabled=true
tls_version=1.3                   # v2.0: 强制TLS 1.3
cert_file=certs/server.crt
key_file=certs/server.key
admin_username=admin
admin_password=QKchat2024!
session_timeout=1800
password_min_length=8
max_login_attempts=5
enable_2fa=false                  # v2.0: 双因子认证支持

[Redis]
host=localhost
port=6379
password=
database=0
connection_timeout=5000
max_connections=50                # v2.0: 增加连接数
cluster_enabled=false             # v2.0: 集群支持
sentinel_enabled=false            # v2.0: 哨兵模式

[Monitoring]                      # v2.0: 新增监控配置
enable_metrics=true
metrics_interval=30000
enable_profiling=true
enable_auto_recovery=true
alert_email=admin@qkchat.com

[Logging]
level=info
file=logs/server.log
max_size=100MB
backup_count=5
enable_console_log=true
enable_structured_log=true        # v2.0: 结构化日志
log_format=json                   # v2.0: JSON格式
```

### SSL证书配置
```bash
# 生成自签名证书（开发环境）
openssl req -x509 -newkey rsa:4096 -keyout certs/server.key -out certs/server.crt -days 365 -nodes
```

### 数据库配置 (`config/database_config.sql`)
```sql
-- 创建数据库用户和权限
CREATE USER 'qkchat_user'@'localhost' IDENTIFIED BY 'qkchat_pass';
GRANT ALL PRIVILEGES ON qkchat.* TO 'qkchat_user'@'localhost';
FLUSH PRIVILEGES;
```

### Redis配置 (`config/redis.conf`)
```ini
# Redis服务器配置
port 6379
bind 127.0.0.1
maxmemory 256mb
maxmemory-policy allkeys-lru
save 900 1
save 300 10
save 60 10000
```

## 🔧 管理界面使用

### 管理员登录
1. 启动服务器后，自动显示登录对话框
2. 输入管理员账号密码（默认：admin/QKchat2024!）
3. 登录成功后进入主管理界面
4. 首次登录后建议立即修改默认密码

### 管理员账号管理
- **默认账号**: admin
- **默认密码**: QKchat2024!
- **邮箱**: admin@qkchat.com
- **显示名称**: 系统管理员
- **权限级别**: 超级管理员

### 安全策略
- **密码策略**: 最少8位，包含大小写字母和数字
- **登录限制**: 连续失败5次后锁定账户
- **会话超时**: 30分钟无操作自动登出
- **审计日志**: 记录所有管理员操作

### 功能模块

#### 📊 仪表板
- 实时服务器状态监控
- 在线用户统计和趋势图
- 系统资源使用情况（CPU、内存、磁盘）
- 消息流量统计和图表
- 数据库连接池状态
- Redis缓存命中率

#### 👥 用户管理
- 用户列表查看和分页
- 用户信息编辑和更新
- 用户状态管理（启用/禁用）
- 用户搜索和高级过滤
- 用户权限管理
- 用户活动日志查看

#### ⚙️ 系统配置
- 服务器参数动态配置
- 数据库连接设置和测试
- 安全策略配置和验证
- 日志级别实时调整
- SSL证书管理
- 备份策略配置

#### 📋 日志查看
- 系统日志实时查看和过滤
- 日志级别分类显示
- 日志搜索和关键词高亮
- 日志导出和归档
- 错误日志统计和分析
- 性能日志监控

#### 📈 实时监控
- 连接数实时监控和告警
- 消息流量监控和统计
- 系统性能监控和趋势
- 异常告警和通知
- 资源使用率监控
- 网络流量分析

## 🔒 安全特性

### 通信安全
- **SSL/TLS 1.3**：端到端加密通信
- **证书验证**：服务器证书验证和客户端证书验证
- **会话管理**：基于JWT令牌的安全会话
- **密钥轮换**：定期密钥更新机制

### 数据安全
- **密码哈希**：SHA-256 + 盐值密码存储
- **数据加密**：敏感数据AES-256加密存储
- **访问控制**：基于角色的权限管理（RBAC）
- **数据备份**：自动加密备份和恢复

### 系统安全
- **账户锁定**：管理员账户智能锁定保护
- **日志审计**：完整的操作日志记录和分析
- **防火墙**：网络访问控制和IP白名单
- **入侵检测**：异常行为监控和告警
- **安全策略**：密码强度、会话超时等策略执行

## � 性能优化

### 数据库优化
- **连接池管理**: 动态连接池大小调整
- **查询优化**: 智能索引和查询计划优化
- **缓存策略**: 多级缓存和LRU淘汰

### 网络优化
- **连接复用**: 长连接复用机制
- **消息压缩**: 数据压缩算法
- **缓存策略**: Redis缓存热点数据

### 内存优化
- **对象池管理**: 连接和对象复用
- **内存监控**: 实时内存使用监控
- **缓存清理**: 定期缓存清理和优化


## 📈 监控和维护

### 系统监控
- **CPU使用率**: 实时CPU监控和多核利用率
- **内存使用**: 内存占用统计和趋势分析
- **网络流量**: 网络I/O监控和带宽使用
- **磁盘使用**: 存储空间监控和I/O性能
- **进程监控**: 进程状态和资源使用

### 性能指标
- **并发连接数**: 当前活跃连接数和峰值统计
- **消息吞吐量**: 每秒处理消息数和QPS监控
- **响应时间**: 平均响应时间和P95/P99延迟
- **错误率**: 系统错误统计和错误分类
- **缓存命中率**: Redis缓存命中率统计

### 维护任务
- **日志轮转**: 自动日志文件轮转和压缩
- **数据备份**: 定期数据库备份和恢复测试
- **证书更新**: SSL证书到期提醒和自动更新
- **系统更新**: 安全补丁更新和版本管理
- **性能调优**: 定期性能分析和优化建议

## 🔧 开发指南

### 代码规范

#### 命名规范（Qt风格）
- **类名**：使用PascalCase，如`ChatServer`、`SessionManager`
- **函数名**：使用camelCase，如`sendMessage()`、`connectToServer()`
- **变量名**：使用camelCase，如`messageList`、`connectionStatus`
- **常量**：使用UPPER_SNAKE_CASE，如`MAX_CONNECTIONS`、`DEFAULT_TIMEOUT`
- **私有成员**：使用下划线前缀，如`_socket`、`_isConnected`

#### 文件组织
- **头文件**：使用`.h`扩展名，包含类声明和必要的包含
- **源文件**：使用`.cpp`扩展名，包含实现代码
- **UI文件**：使用`.ui`扩展名，用于Qt Designer

#### 代码格式
- 使用4个空格缩进（不使用Tab）
- 大括号采用K&R风格（左大括号不换行）
- 行长度限制在120字符以内
- 使用空行分隔逻辑块

### 架构设计原则

#### SOLID原则
1. **单一职责原则（SRP）**：每个类只负责一个功能
2. **开闭原则（OCP）**：对扩展开放，对修改关闭
3. **里氏替换原则（LSP）**：子类可以替换父类
4. **接口隔离原则（ISP）**：客户端不应该依赖它不需要的接口
5. **依赖倒置原则（DIP）**：依赖抽象而不是具体实现

#### 设计模式应用
- 优先使用组合而非继承
- 使用接口定义契约
- 采用依赖注入管理依赖关系

### 内存管理

#### Qt对象树
- 使用Qt的对象树自动管理内存
- 父对象销毁时自动删除子对象
- 避免手动delete Qt对象

#### 智能指针
- 使用`std::unique_ptr`管理独占资源
- 使用`std::shared_ptr`管理共享资源
- 避免使用裸指针

### 线程安全

#### 信号槽连接
```cpp
// 跨线程连接使用Qt::QueuedConnection
connect(sender, &Sender::signal, 
        receiver, &Receiver::slot, 
        Qt::QueuedConnection);
```

#### 互斥锁使用
```cpp
class ThreadSafeClass
{
private:
    mutable QMutex _mutex;
    QVariant _data;
    
public:
    QVariant getData() const
    {
        QMutexLocker locker(&_mutex);
        return _data;
    }
};
```

### 错误处理

#### 异常处理
- 使用Qt的异常处理机制
- 避免在析构函数中抛出异常
- 使用RAII模式管理资源

#### 日志记录
```cpp
// 使用Qt的日志系统
qDebug() << "Debug message";
qWarning() << "Warning message";
qCritical() << "Critical error";
```

### 添加新功能
1. **业务逻辑**：在 `src/core/` 中添加核心服务
2. **管理界面**：在 `src/admin/` 中添加管理组件
3. **数据库操作**：在 `src/database/` 中扩展数据访问
4. **网络协议**：在 `src/network/` 中扩展协议支持
5. **缓存管理**：在 `src/cache/` 中添加缓存逻辑

### 性能优化

#### 避免不必要的拷贝
```cpp
// 使用const引用传递参数
void processMessage(const QString &message);

// 使用移动语义
QString createMessage() &&;
```

#### 缓存优化
- 缓存频繁访问的数据
- 使用`QCache`管理缓存
- 实现LRU淘汰策略

### 文档规范

#### 代码注释
```cpp
/**
 * @brief 发送消息到服务器
 * @param message 要发送的消息内容
 * @param receiver 接收者ID
 * @return 发送是否成功
 */
bool sendMessage(const QString &message, const QString &receiver);
```

## 📝 更新日志

### v1.1.0 (2025-08-03) - 架构重构版本
#### 🎯 架构升级
- **多级缓存系统**: 实现L1内存缓存、智能缓存策略和预加载机制
- **异步处理引擎**: 重构消息处理为异步架构
- **线程管理**: 实现专用线程池管理器
- **自动故障恢复**: 添加故障检测和自动恢复机制

#### 🔧 核心功能增强
- **缓存管理**: 新增多级缓存、策略管理和预加载器
- **消息处理**: 重构消息处理器和路由机制
- **连接管理**: 优化连接池和会话管理
- **数据库**: 添加连接池和事务管理
- **网络处理**: 实现异步网络事件处理

#### 🛠️ 新增组件
- **MultiLevelCache**: 多级缓存核心组件
- **CacheStrategyManager**: 智能缓存策略管理
- **CachePreloader**: 缓存预加载器
- **MessageHandlers**: 消息处理器集合
- **AutoRecovery**: 自动故障恢复系统
- **LockFreeStructures**: 无锁数据结构

#### 🔒 安全性增强
- **TLS 1.3**: 支持最新加密标准
- **会话管理**: 增强的会话验证机制
- **数据加密**: 敏感数据加密存储
- **安全审计**: 完整的操作日志记录

### v1.0.0 (2025-08-02) - 初始版本
- 基础聊天功能实现
- 管理界面开发
- SSL/TLS安全通信
- MySQL数据库集成
- Redis缓存支持

---

**当前版本**: v1.1.0
**最后更新**: 2025年08月03日
**Qt版本**: 6.5+
**C++标准**: C++17
**开发环境**: Qt Creator 16.0.2+
**架构**: 多级缓存异步架构