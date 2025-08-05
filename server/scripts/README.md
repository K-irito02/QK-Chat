# QK Chat Server - 数据库脚本

这个目录包含了QK Chat Server的数据库管理脚本。

## 脚本说明

### 1. `safe_db_init.bat` - 安全数据库初始化（推荐）
- 检查数据库是否已存在
- 提供选项跳过或重新创建数据库
- 使用 `--force` 选项忽略重复索引错误
- 最安全的初始化方式

**使用方法：**
```bash
cd server/scripts
safe_db_init.bat
```

### 2. `diagnose_database.bat` - 数据库诊断
- 检查MySQL安装状态
- 检查MySQL服务运行状态
- 检查数据库是否存在
- 检查用户是否存在
- 测试数据库连接

**使用方法：**
```bash
cd server/scripts
diagnose_database.bat
```

## 数据库配置

- **数据库名：** `qkchat`
- **用户名：** `qkchat_user`
- **密码：** `3143285505`
- **主机：** `localhost`
- **端口：** `3306`

## 故障排除

### 1. MySQL未安装
```
错误：MySQL未安装或不在PATH中
```
**解决方案：**
- 下载并安装MySQL
- 将MySQL的bin目录添加到系统PATH

### 2. MySQL服务未运行
```
警告：MySQL服务可能未运行
```
**解决方案：**
- 启动MySQL服务
- Windows: `net start mysql`
- 或通过服务管理器启动MySQL服务

### 3. 数据库不存在
```
错误：qkchat数据库不存在
```
**解决方案：**
- 运行 `safe_db_init.bat` 创建数据库

### 4. 用户不存在
```
错误：qkchat_user不存在
```
**解决方案：**
- 运行 `safe_db_init.bat` 创建用户

### 5. 连接失败
```
错误：无法使用qkchat_user连接到数据库
```
**解决方案：**
- 检查密码是否正确
- 重新运行初始化脚本

## 手动数据库操作

如果脚本无法正常工作，可以手动执行以下SQL命令：

```sql
-- 创建数据库
CREATE DATABASE IF NOT EXISTS qkchat CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- 创建用户
CREATE USER IF NOT EXISTS 'qkchat_user'@'localhost' IDENTIFIED BY '3143285505';

-- 授权
GRANT ALL PRIVILEGES ON qkchat.* TO 'qkchat_user'@'localhost';
FLUSH PRIVILEGES;

-- 使用数据库
USE qkchat;

-- 运行初始化脚本
source ../data/mysql_init.sql;
```

## 使用步骤

1. **首次设置：**
   ```bash
   cd server/scripts
   safe_db_init.bat
   ```

2. **诊断数据库：**
   ```bash
   cd server/scripts
   diagnose_database.bat
   ```

3. **启动服务器：**
   ```bash
   cd ..
   cmake --build build --config Debug
   ```

## 注意事项

1. 确保MySQL已正确安装并添加到PATH
2. 确保MySQL服务正在运行
3. 需要MySQL root权限来创建数据库和用户
4. 初始化脚本只需要运行一次
5. 如果数据库已存在，脚本不会覆盖现有数据
6. 所有脚本现在使用简体中文交互 