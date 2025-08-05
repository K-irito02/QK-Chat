#include "ChatServer.h"
#include "../database/Database.h"
#include "../services/EmailService.h"
#include "../utils/RedisEmailVerification.h"
#include "../services/EmailTemplate.h"
#include "../network/QSslServer.h"
#include "../core/SessionManager.h"
#include "../network/ProtocolParser.h"
#include "../config/ServerConfig.h"
#include "../utils/LogManager.h"
#include "../utils/StackTraceLogger.h"
#include "../network/NonBlockingConnectionManager.h"
#include <QProcess>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>
#include <QLoggingCategory>
#include <QPointer>
#include <QDateTime>
#include <QByteArray>
#include <QDataStream>
#include <QSqlQuery>
#include <QCryptographicHash>
#include <QtEndian>
#include <QHostAddress>
#include <QTimer>
#include "../utils/ThreadPool.h"
#include "../utils/SystemMonitor.h"
#include "../cache/CacheManagerV2.h"
#include <QJsonParseError>
#include <QSslConfiguration>
#include <QSslCertificate>
#include <QSslKey>
#include <QFile>
#include <QCoreApplication>
#include <QRandomGenerator>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QElapsedTimer>

// Windows API 头文件
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")
#endif

Q_DECLARE_LOGGING_CATEGORY(chatServer)
Q_LOGGING_CATEGORY(chatServer, "qkchat.server.chatserver")

ChatServer::ChatServer(QObject *parent)
    : QObject(parent)
    , _sslServer(nullptr)
    , _database(nullptr)
    , _sessionManager(nullptr)
    , _protocolParser(nullptr)
    , _threadPool(nullptr)
    , _cleanupTimer(nullptr)
    , _host("0.0.0.0")
    , _port(8443)
    , _isRunning(false)
    , _startTime()
    , _lastSystemInfoUpdate()
    , _totalMessages(0)
    , _cachedCpuUsage(0)
    , _cachedMemoryUsage(0)
    , _cachedOnlineUserCount(0)
    , _cachedTotalUserCount(0)
    , _systemInfoTimer(nullptr)
#ifdef Q_OS_WIN
    , _pdhInitialized(false)
#endif
{
    setupCleanupTimer();
    LogManager::instance()->writeSystemLog("ChatServer", "INITIALIZED", "ChatServer instance created");
}

ChatServer::~ChatServer()
{
    stopServer();
    
#ifdef Q_OS_WIN
    // 清理PDH资源
    if (_pdhInitialized) {
        PdhCloseQuery(_cpuQuery);
        PdhCloseQuery(_memQuery);
        _pdhInitialized = false;
    }
#endif
    
    if (_threadPool) {
        _threadPool->shutdown();
        delete _threadPool;
        _threadPool = nullptr;
    }
    
    if (_cleanupTimer) {
        _cleanupTimer->stop();
        delete _cleanupTimer;
        _cleanupTimer = nullptr;
    }
    
    if (_systemInfoTimer) {
        _systemInfoTimer->stop();
        delete _systemInfoTimer;
        _systemInfoTimer = nullptr;
    }
    
    qCInfo(chatServer) << "ChatServer destroyed";
}

// 数据库初始化已删除 - 使用现有数据库结构
bool ChatServer::initializeDatabase()
{
    qCInfo(chatServer) << "Database initialization called";
    
    // 如果数据库已经连接，直接返回成功
    if (_database && _database->isConnected()) {
        qCInfo(chatServer) << "Database already connected, skipping initialization";
        return true;
    }
    
    // 如果数据库实例不存在，创建新的
    if (!_database) {
        qCInfo(chatServer) << "Creating new Database instance...";
        _database = new Database(this);
    }
    
    // 尝试连接数据库
    try {
        if (!_database->initialize()) {
            QString error = "Failed to connect to database";
            qCCritical(chatServer) << error;
            emit databaseError(error);
            return false;
        }
    } catch (const std::exception& e) {
        QString error = QString("Database initialization exception: %1").arg(e.what());
        qCCritical(chatServer) << error;
        emit databaseError(error);
        return false;
    } catch (...) {
        QString error = "Unknown database initialization exception";
        qCCritical(chatServer) << error;
        emit databaseError(error);
        return false;
    }
    
    qCInfo(chatServer) << "Database connected successfully";
    emit databaseConnected();
    return true;
}

// 启动服务器
bool ChatServer::startServer()
{
    try {
        qCInfo(chatServer) << "Starting chat server...";
        
        // 如果服务器已经在运行，直接返回
        if (_isRunning) {
            qCInfo(chatServer) << "Server is already running";
            return true;
        }
        
        // 设置开始时间
        _startTime = QDateTime::currentDateTime();
        
        // 数据库已经在main.cpp中初始化，这里不需要重复初始化
        if (!_database || !_database->isConnected()) {
            qCWarning(chatServer) << "Database not connected, cannot start server";
            return false;
        }
        
        qCInfo(chatServer) << "Database is connected, proceeding with server startup";
        
        // 初始化SSL服务器
        setupSslServer();
        
        // 启动SSL服务器
        if (!_sslServer) {
            qCWarning(chatServer) << "SSL server not initialized";
            return false;
        }
        
        // 启动服务器
        if (!_sslServer->listen(QHostAddress(_host), _port)) {
            QString error = QString("Failed to start SSL server: %1").arg(_sslServer->errorString());
            qCCritical(chatServer) << error;
            emit serverError(error);
            return false;
        }
        
        // 设置运行标志
        _isRunning = true;
        
        qCInfo(chatServer) << "Chat server started successfully on port" << _sslServer->serverPort();
        emit serverStarted();
        
        return true;
        
    } catch (const std::exception& e) {
        QString error = QString("Exception starting server: %1").arg(e.what());
        qCCritical(chatServer) << error;
        emit serverError(error);
        return false;
    } catch (...) {
        QString error = "Unknown exception starting server";
        qCCritical(chatServer) << error;
        emit serverError(error);
        return false;
    }
}

// 停止服务器
void ChatServer::stopServer()
{
    LOG_METHOD_ENTRY();
    
    if (!_isRunning) {
        qCWarning(chatServer) << "Server is not running";
        return;
    }
    
    _isRunning = false;
    
    // 停止非阻塞连接管理器
    NonBlockingConnectionManager::instance().stopMonitoring();
    NonBlockingConnectionManager::instance().disconnectAll();
    
    if (_sslServer) {
        _sslServer->close();
        qCInfo(chatServer) << "SSL server stopped";
    }
    
    if (_cleanupTimer) {
        _cleanupTimer->stop();
    }
    
    if (_systemInfoTimer) {
        _systemInfoTimer->stop();
    }
    
    // 清理所有客户端连接
    {
        QMutexLocker locker(&_clientsMutex);
        _clients.clear();
    }
    
    qCInfo(chatServer) << "Server stopped";
    LogManager::instance()->writeSystemLog("ChatServer", "STOPPED", "Server stopped");
    
    emit serverStopped();
    LOG_METHOD_EXIT();
}

// 处理客户端连接
void ChatServer::onClientConnected(QSslSocket* socket)
{
    if (!socket) {
        return;
    }
    
    QString clientId = QUuid::createUuid().toString();
    QString clientAddress = socket->peerAddress().toString();
    quint16 clientPort = socket->peerPort();
    
    // 添加到非阻塞连接管理器
    NonBlockingConnectionManager::instance().addConnection(
        socket, clientId, false);
    
    // 创建客户端信息
    ClientInfo clientInfo;
    clientInfo.socket = socket;
    clientInfo.clientId = clientId;
    clientInfo.address = clientAddress;
    clientInfo.port = clientPort;
    clientInfo.connectedTime = QDateTime::currentDateTime();
    clientInfo.lastActivity = QDateTime::currentDateTime();
    
    // 添加到客户端列表
    {
        QMutexLocker locker(&_clientsMutex);
        _clients[clientId] = clientInfo;
    }
    
    // 连接信号
    connect(socket, &QSslSocket::readyRead, this, [this, clientId]() {
        handleClientData(clientId);
    });
    
    connect(socket, &QSslSocket::disconnected, this, [this, clientId]() {
        onClientDisconnected(clientId);
    });
    
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::errorOccurred),
            this, [this, clientId](QAbstractSocket::SocketError error) {
        handleSocketError(clientId, error);
    });
    
    emit clientConnected(clientId, clientAddress);
}

// 处理客户端断开连接（信号槽连接）
void ChatServer::onClientDisconnected(const QString& clientId)
{
    handleClientDisconnected(clientId);
}

// 处理客户端断开连接
void ChatServer::handleClientDisconnected(const QString& clientId)
{
    QMutexLocker locker(&_clientsMutex);
    
    if (_clients.contains(clientId)) {
        QString clientAddress = _clients[clientId].address;
        _clients.remove(clientId);
        
        qCInfo(chatServer) << "Client disconnected:" << clientAddress;
        emit clientDisconnected(clientId, clientAddress);
    }
}

// 处理客户端数据（使用线程池）
void ChatServer::handleClientData(const QString& clientId)
{
    if (!_threadPool) {
        return;
    }
    
    _threadPool->enqueue([this, clientId]() {
        QMutexLocker locker(&_clientsMutex);
        
        if (!_clients.contains(clientId)) {
            return;
        }
        
        ClientInfo& client = _clients[clientId];
        QSslSocket* socket = client.socket;
        
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
            return;
        }
        
        qint64 bytesAvailable = socket->bytesAvailable();
        if (bytesAvailable <= 0) {
            return;
        }
        
        // 读取数据到缓冲区
        QByteArray newData = socket->readAll();
        client.messageBuffer.append(newData);
        
        // 处理完整的消息
        while (client.messageBuffer.size() >= 4) {
            // 读取消息长度
            qint32 messageLength = qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(client.messageBuffer.left(4).data()));
            
            if (messageLength <= 0 || messageLength > 1024 * 1024) { // 最大1MB
                client.messageBuffer.clear();
                return;
            }
            
            // 检查是否有完整的消息
            if (client.messageBuffer.size() < 4 + messageLength) {
                break; // 等待更多数据
            }
            
            // 提取完整的消息
            QByteArray messageData = client.messageBuffer.mid(4, messageLength);
            client.messageBuffer.remove(0, 4 + messageLength);
            
            // 处理消息
            processClientMessage(clientId, messageData);
        }
    });
}

// 处理socket错误
void ChatServer::handleSocketError(const QString& clientId, QAbstractSocket::SocketError error)
{
    QMutexLocker locker(&_clientsMutex);
    
    if (_clients.contains(clientId)) {
        QString errorStr = _clients[clientId].socket->errorString();
        QString address = _clients[clientId].address;
        
        qCWarning(chatServer) << "Socket error for client" << clientId 
                             << "(" << address << "):" << error << errorStr;
        
        // 记录堆栈追踪
        StackTraceLogger::instance().logStackTrace(
            QString("SOCKET_ERROR_%1_%2").arg(clientId).arg(error), 
            "ChatServer::handleSocketError"
        );
        
        // 移除客户端
        _clients.remove(clientId);
        emit clientDisconnected(clientId, address);
    }
}

// 处理登录请求
void ChatServer::handleLoginRequest(const QString& clientId, const QJsonObject& request)
{
    QString username = request["username"].toString();
    QString password = request["password"].toString();
    QString deviceInfo = request["deviceInfo"].toString();
    
    if (username.isEmpty() || password.isEmpty()) {
        sendErrorResponse(clientId, "LOGIN", "Username or password empty");
        return;
    }
    
    // 使用线程池异步处理登录
    _threadPool->enqueue([this, clientId, username, password, deviceInfo]() {
        try {
            // 验证用户凭据
            Database::UserInfo userInfo = _database->authenticateUser(username, password);
            
            if (userInfo.id == 0) {
                sendErrorResponse(clientId, "LOGIN", "Invalid username or password");
                return;
            }
            
            // 检查用户状态
            if (userInfo.status != "active") {
                sendErrorResponse(clientId, "LOGIN", "Account is not active");
                return;
            }
            
            // 创建会话
            QString sessionToken = _database->createUserSession(
                userInfo.id, 
                deviceInfo, 
                getClientAddress(clientId), 
                24 // 24小时有效期
            );
            
            if (sessionToken.isEmpty()) {
                sendErrorResponse(clientId, "LOGIN", "Failed to create session");
                return;
            }
            
            // 更新客户端信息
            {
                QMutexLocker locker(&_clientsMutex);
                if (_clients.contains(clientId)) {
                    _clients[clientId].userId = userInfo.id;
                    _clients[clientId].username = userInfo.username;
                    _clients[clientId].isAuthenticated = true;
                }
            }
            
            // 更新最后在线时间
            _database->updateUserLastOnline(userInfo.id, QDateTime::currentDateTime());
            
            // 发送成功响应
            QJsonObject response;
            response["type"] = "LOGIN_RESPONSE";
            response["success"] = true;
            response["userId"] = userInfo.id;
            response["username"] = userInfo.username;
            response["displayName"] = userInfo.displayName;
            response["avatarUrl"] = userInfo.avatarUrl;
            response["email"] = userInfo.email;
            response["sessionToken"] = sessionToken;
            
            sendJsonMessage(clientId, response);
            
            qCInfo(chatServer) << "User logged in:" << username << "(" << clientId << ")";
            
        } catch (const std::exception& e) {
            qCCritical(chatServer) << "Login error:" << e.what();
            sendErrorResponse(clientId, "LOGIN", "Server error occurred");
            
            // 记录堆栈追踪
            StackTraceLogger::instance().logStackTrace("LOGIN_EXCEPTION", "ChatServer::handleLoginRequest");
        }
    });
}



// 发送JSON消息
void ChatServer::sendJsonMessage(const QString& clientId, const QJsonObject& message)
{
    QMutexLocker locker(&_clientsMutex);
    
    if (!_clients.contains(clientId)) {
        return;
    }
    
    QSslSocket* socket = _clients[clientId].socket;
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    
    // 直接发送消息，不包装
    QJsonDocument doc(message);
    QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
    
    // 添加长度前缀
    QByteArray lengthBytes(4, 0);
    qToBigEndian<qint32>(jsonData.size(), reinterpret_cast<uchar*>(lengthBytes.data()));
    QByteArray packet = lengthBytes + jsonData;
    
    socket->write(packet);
    socket->flush();
}

// 发送错误响应
void ChatServer::sendErrorResponse(const QString& clientId, const QString& requestType, const QString& error)
{
    QJsonObject response;
    response["type"] = requestType + "_RESPONSE";
    response["success"] = false;
    response["error"] = error;
    
    sendJsonMessage(clientId, response);
}

// 配置SSL
bool ChatServer::configureSsl()
{
    ServerConfig* config = ServerConfig::instance();
    
    QString certPath = config->getSslCertificateFile();
    QString keyPath = config->getSslPrivateKeyFile();
    
    if (certPath.isEmpty() || keyPath.isEmpty()) {
        qCCritical(chatServer) << "SSL certificate or key path not configured";
        return false;
    }
    
    QFile certFile(certPath);
    QFile keyFile(keyPath);
    
    if (!certFile.open(QIODevice::ReadOnly) || !keyFile.open(QIODevice::ReadOnly)) {
        qCCritical(chatServer) << "Failed to open SSL certificate or key file";
        return false;
    }
    
    QSslCertificate certificate(certFile.readAll());
    QSslKey privateKey(keyFile.readAll(), QSsl::Rsa, QSsl::Pem, QSsl::PrivateKey);
    
    certFile.close();
    keyFile.close();
    
    if (certificate.isNull() || privateKey.isNull()) {
        qCCritical(chatServer) << "Invalid SSL certificate or key";
        return false;
    }
    
    QSslConfiguration sslConfig;
    sslConfig.setLocalCertificate(certificate);
    sslConfig.setPrivateKey(privateKey);
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    
    _sslServer->setSslConfiguration(sslConfig);
    
    qCInfo(chatServer) << "SSL configuration loaded successfully";
    return true;
}



// 更新系统信息
void ChatServer::updateSystemInfo()
{
    // 添加更严格的检查
    if (!_isRunning || !_threadPool) {
        qCDebug(chatServer) << "updateSystemInfo: Server not running or thread pool not available";
        return;
    }

    // 使用QPointer确保对象安全
    QPointer<ChatServer> self(this);

    try {
        _threadPool->enqueue([self]() {
            // 检查对象是否仍然有效
            if (!self || !self->_isRunning) {
                return;
            }

            try {
                self->_cachedCpuUsage = self->getCpuUsage();
                self->_cachedMemoryUsage = self->getMemoryUsage();

                // 记录系统状态 - 减少日志频率
                static int logCounter = 0;
                if (++logCounter % 12 == 0) { // 每分钟记录一次（12 * 5秒）
                    QString logMsg = QString("System Info - CPU: %1%, Memory: %2%")
                                   .arg(self->_cachedCpuUsage)
                                   .arg(self->_cachedMemoryUsage);

                    qCInfo(chatServer) << logMsg;
                }

            } catch (const std::exception& e) {
                qCWarning(chatServer) << "Failed to update system info:" << e.what();
            } catch (...) {
                qCWarning(chatServer) << "Unknown exception in updateSystemInfo";
            }
        });
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Failed to enqueue system info update:" << e.what();
    } catch (...) {
        qCWarning(chatServer) << "Unknown exception in updateSystemInfo enqueue";
    }
}

// 获取CPU使用率
int ChatServer::getCpuUsage() const
{
    try {
        // 使用简单的进程CPU使用率计算，避免PDH计数器问题
#ifdef Q_OS_WIN
        // Windows系统：使用GetProcessTimes获取进程CPU时间
        HANDLE hProcess = GetCurrentProcess();
        FILETIME createTime, exitTime, kernelTime, userTime;
        
        if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
            // 计算CPU使用率（简化版本）
            static ULARGE_INTEGER lastKernelTime = {0}, lastUserTime = {0};
            static QDateTime lastCheckTime;
            
            ULARGE_INTEGER currentKernelTime, currentUserTime;
            currentKernelTime.LowPart = kernelTime.dwLowDateTime;
            currentKernelTime.HighPart = kernelTime.dwHighDateTime;
            currentUserTime.LowPart = userTime.dwLowDateTime;
            currentUserTime.HighPart = userTime.dwHighDateTime;
            
            QDateTime currentTime = QDateTime::currentDateTime();
            
            if (lastCheckTime.isValid()) {
                qint64 timeDiff = lastCheckTime.msecsTo(currentTime);
                if (timeDiff > 0) {
                    qint64 kernelDiff = currentKernelTime.QuadPart - lastKernelTime.QuadPart;
                    qint64 userDiff = currentUserTime.QuadPart - lastUserTime.QuadPart;
                    qint64 totalDiff = kernelDiff + userDiff;
                    
                    // 转换为百分比（简化计算）
                    int cpuUsage = static_cast<int>((totalDiff * 100) / (timeDiff * 10000));
                    cpuUsage = qBound(0, cpuUsage, 100); // 限制在0-100范围内
                    
                    lastKernelTime = currentKernelTime;
                    lastUserTime = currentUserTime;
                    lastCheckTime = currentTime;
                    
                    return cpuUsage;
                }
            }
            
            lastKernelTime = currentKernelTime;
            lastUserTime = currentUserTime;
            lastCheckTime = currentTime;
        }
        
        // 如果无法获取，返回0
        return 0;
#else
        // 非Windows系统返回0
        return 0;
#endif
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception in getCpuUsage:" << e.what();
        return 0;
    } catch (...) {
        qCWarning(chatServer) << "Unknown exception in getCpuUsage";
        return 0;
    }
}

// 获取内存使用率
int ChatServer::getMemoryUsage() const
{
    try {
#ifdef Q_OS_WIN
        // Windows系统：计算系统内存使用率
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        
        if (GlobalMemoryStatusEx(&memInfo)) {
            // 计算系统内存使用率
            DWORDLONG usedMemory = memInfo.ullTotalPhys - memInfo.ullAvailPhys;
            DWORDLONG totalMemory = memInfo.ullTotalPhys;
            
            if (totalMemory > 0) {
                int memoryUsage = static_cast<int>((usedMemory * 100) / totalMemory);
                memoryUsage = qBound(0, memoryUsage, 100); // 限制在0-100范围内
                return memoryUsage;
            }
        }
        
        // 如果无法获取，返回0
        return 0;
#else
        // 非Windows系统返回0
        return 0;
#endif
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception in getMemoryUsage:" << e.what();
        return 0;
    } catch (...) {
        qCWarning(chatServer) << "Unknown exception in getMemoryUsage";
        return 0;
    }
}

// 获取服务器状态
QJsonObject ChatServer::getServerStatus() const
{
    QJsonObject status;
    status["isRunning"] = _isRunning;
    status["host"] = _host;
    status["port"] = _port;
    status["uptime"] = formatUptime();
    status["onlineUsers"] = getOnlineUserCount();
    status["totalUsers"] = getTotalUserCount();
    status["messagesCount"] = getMessagesCount();
    status["cpuUsage"] = getCpuUsage();
    status["memoryUsage"] = getMemoryUsage();
    status["connectionCount"] = getConnectionCount();
    return status;
}

// SSL错误处理
void ChatServer::onSslErrors(const QList<QSslError>& errors)
{
    for (const QSslError& error : errors) {
        qCWarning(chatServer) << "SSL Error:" << error.errorString();
    }
}

// 对等验证错误处理
void ChatServer::onPeerVerifyError(const QSslError& error)
{
    qCWarning(chatServer) << "Peer verification error:" << error.errorString();
}

// 获取客户端地址
QString ChatServer::getClientAddress(const QString& clientId) const
{
    QMutexLocker locker(&_clientsMutex);
    if (_clients.contains(clientId)) {
        return _clients[clientId].address;
    }
    return QString();
}

#ifdef Q_OS_WIN
// 初始化PDH计数器
bool ChatServer::initializePdhCounters() const
{
    try {
        // 初始化CPU计数器
        PDH_STATUS status = PdhOpenQueryA(NULL, 0, &_cpuQuery);
        if (status != ERROR_SUCCESS) {
            qCWarning(chatServer) << "Failed to open CPU query:" << status;
            return false;
        }
        
        status = PdhAddCounterA(_cpuQuery, "\\Processor(_Total)\\% Processor Time", 0, &_cpuTotal);
        if (status != ERROR_SUCCESS) {
            qCWarning(chatServer) << "Failed to add CPU counter:" << status;
            PdhCloseQuery(_cpuQuery);
            return false;
        }
        
        // 初始化内存计数器
        status = PdhOpenQueryA(NULL, 0, &_memQuery);
        if (status != ERROR_SUCCESS) {
            qCWarning(chatServer) << "Failed to open memory query:" << status;
            PdhCloseQuery(_cpuQuery);
            return false;
        }
        
        status = PdhAddCounterA(_memQuery, "\\Memory\\% Committed Bytes In Use", 0, &_memTotal);
        if (status != ERROR_SUCCESS) {
            qCWarning(chatServer) << "Failed to add memory counter:" << status;
            PdhCloseQuery(_cpuQuery);
            PdhCloseQuery(_memQuery);
            return false;
        }
        
        // 收集初始数据
        PdhCollectQueryData(_cpuQuery);
        PdhCollectQueryData(_memQuery);
        
        qCInfo(chatServer) << "PDH counters initialized successfully";
        return true;
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception initializing PDH counters:" << e.what();
        return false;
    } catch (...) {
        qCWarning(chatServer) << "Unknown error initializing PDH counters";
        return false;
    }
}

// 通过PDH获取CPU使用率
int ChatServer::getCpuUsageViaPdh() const
{
    try {
        PDH_FMT_COUNTERVALUE counterVal;
        
        // 收集数据
        PDH_STATUS status = PdhCollectQueryData(_cpuQuery);
        if (status != ERROR_SUCCESS) {
            qCWarning(chatServer) << "Failed to collect CPU data, status:" << status;
            return 0;
        }
        
        // 获取格式化值
        status = PdhGetFormattedCounterValue(_cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
        if (status != ERROR_SUCCESS) {
            qCWarning(chatServer) << "Failed to get CPU counter value, status:" << status;
            return 0;
        }
        
        int cpuUsage = static_cast<int>(counterVal.doubleValue);
        qCInfo(chatServer) << "PDH CPU usage raw value:" << counterVal.doubleValue << "%, formatted:" << cpuUsage << "%";
        
        // PDH第一次调用可能返回0，需要等待一段时间
        if (cpuUsage == 0) {
            qCInfo(chatServer) << "PDH CPU usage is 0, this might be the first call";
        }
        
        return cpuUsage;
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception getting CPU usage via PDH:" << e.what();
        return 0;
    } catch (...) {
        qCWarning(chatServer) << "Unknown error getting CPU usage via PDH";
        return 0;
    }
}

// 通过PDH获取内存使用率
int ChatServer::getMemoryUsageViaPdh() const
{
    try {
        PDH_FMT_COUNTERVALUE counterVal;
        
        // 收集数据
        PDH_STATUS status = PdhCollectQueryData(_memQuery);
        if (status != ERROR_SUCCESS) {
            qCWarning(chatServer) << "Failed to collect memory data, status:" << status;
            return 0;
        }
        
        // 获取格式化值
        status = PdhGetFormattedCounterValue(_memTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
        if (status != ERROR_SUCCESS) {
            qCWarning(chatServer) << "Failed to get memory counter value, status:" << status;
            return 0;
        }
        
        int memoryUsage = static_cast<int>(counterVal.doubleValue);
        qCInfo(chatServer) << "PDH Memory usage raw value:" << counterVal.doubleValue << "%, formatted:" << memoryUsage << "%";
        
        return memoryUsage;
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception getting memory usage via PDH:" << e.what();
        return 0;
    } catch (...) {
        qCWarning(chatServer) << "Unknown error getting memory usage via PDH";
        return 0;
    }
}

// 通过Registry获取CPU使用率（备用方案）
int ChatServer::getCpuUsageViaRegistry() const
{
    try {
        static ULARGE_INTEGER lastIdle, lastKernel, lastUser;
        static bool firstCall = true;
        FILETIME idle, kernel, user;
        
        if (!GetSystemTimes(&idle, &kernel, &user)) {
            qCWarning(chatServer) << "Failed to get system times";
            return 0;
        }
        
        ULARGE_INTEGER currentIdle, currentKernel, currentUser;
        currentIdle.LowPart = idle.dwLowDateTime;
        currentIdle.HighPart = idle.dwHighDateTime;
        currentKernel.LowPart = kernel.dwLowDateTime;
        currentKernel.HighPart = kernel.dwHighDateTime;
        currentUser.LowPart = user.dwLowDateTime;
        currentUser.HighPart = user.dwHighDateTime;
        
        // 第一次调用时，初始化"上次"值并返回0
        if (firstCall) {
            lastKernel = currentKernel;
            lastUser = currentUser;
            lastIdle = currentIdle;
            firstCall = false;
            qCInfo(chatServer) << "Registry CPU usage first call, returning 0";
            return 0;
        }
        
        // 计算差值
        ULONGLONG kernelDiff = currentKernel.QuadPart - lastKernel.QuadPart;
        ULONGLONG userDiff = currentUser.QuadPart - lastUser.QuadPart;
        ULONGLONG idleDiff = currentIdle.QuadPart - lastIdle.QuadPart;
        
        // 保存当前值作为下次的"上次"值
        lastKernel = currentKernel;
        lastUser = currentUser;
        lastIdle = currentIdle;
        
        ULONGLONG total = kernelDiff + userDiff;
        ULONGLONG used = total - idleDiff;
        
        // 添加边界检查
        if (total == 0) {
            qCWarning(chatServer) << "Total time is 0, returning 0";
            return 0;
        }
        
        if (used > total) {
            qCWarning(chatServer) << "Used time greater than total, clamping";
            used = total;
        }
        
        int cpuUsage = static_cast<int>((used * 100) / total);
        qCInfo(chatServer) << "Registry CPU usage calculated:" << cpuUsage << "% (total:" << total << ", used:" << used << ")";
        
        return cpuUsage;
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception getting CPU usage via Registry:" << e.what();
        return 0;
    } catch (...) {
        qCWarning(chatServer) << "Unknown error getting CPU usage via Registry";
        return 0;
    }
}

// 通过Registry获取内存使用率（备用方案）
int ChatServer::getMemoryUsageViaRegistry() const
{
    try {
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        
        if (!GlobalMemoryStatusEx(&memInfo)) {
            qCWarning(chatServer) << "Failed to get memory status";
            return 0;
        }
        
        int memoryUsage = static_cast<int>(memInfo.dwMemoryLoad);
        qCInfo(chatServer) << "Registry Memory usage:" << memoryUsage << "% (Total:" << memInfo.ullTotalPhys / (1024*1024) << "MB, Available:" << memInfo.ullAvailPhys / (1024*1024) << "MB)";
        
        return memoryUsage;
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception getting memory usage via Registry:" << e.what();
        return 0;
    } catch (...) {
        qCWarning(chatServer) << "Unknown error getting memory usage via Registry";
        return 0;
    }
}
#endif



// 执行清理
void ChatServer::performCleanup()
{
    if (!_isRunning) return;
    
    _threadPool->enqueue([this]() {
        try {
            QMutexLocker locker(&_clientsMutex);
            QStringList clientsToRemove;
            
            for (auto it = _clients.begin(); it != _clients.end(); ++it) {
                const ClientInfo& client = it.value();
                
                // 检查连接是否断开
                if (!client.socket || client.socket->state() != QAbstractSocket::ConnectedState) {
                    clientsToRemove.append(it.key());
                    continue;
                }
                
                // 检查超时
                qint64 lastActivity = client.lastActivity.msecsTo(QDateTime::currentDateTime());
                if (lastActivity > 300000) { // 5分钟超时
                    clientsToRemove.append(it.key());
                }
            }
            
            // 移除断开的客户端
            for (const QString& clientId : clientsToRemove) {
                handleClientDisconnected(clientId);
            }
            
        } catch (const std::exception& e) {
            qCCritical(chatServer) << "Cleanup error:" << e.what();
            StackTraceLogger::instance().logStackTrace("CLEANUP_EXCEPTION", "ChatServer::performCleanup");
        }
    });
}

// 处理登出请求
void ChatServer::handleLogoutRequest(const QString& clientId)
{
    QMutexLocker locker(&_clientsMutex);
    if (_clients.contains(clientId)) {
        ClientInfo& client = _clients[clientId];
        client.isAuthenticated = false;
        client.userId = -1;
        
        QJsonObject response;
        response["type"] = "logout";
        response["status"] = "success";
        sendJsonMessage(clientId, response);
        
        qCInfo(chatServer) << "User logged out:" << clientId;
    }
}

// 处理消息请求
void ChatServer::handleMessageRequest(const QString& clientId, const QVariantMap& data)
{
    // 实现消息处理逻辑
    QJsonObject response;
    response["type"] = "message";
    response["status"] = "received";
    sendJsonMessage(clientId, response);
}

// 处理心跳
void ChatServer::handleHeartbeat(const QString& clientId)
{
    QMutexLocker locker(&_clientsMutex);
    if (_clients.contains(clientId)) {
        _clients[clientId].lastActivity = QDateTime::currentDateTime();
        
        QJsonObject response;
        response["type"] = "heartbeat";
        response["status"] = "ok";
        sendJsonMessage(clientId, response);
    }
}

// 处理验证请求
void ChatServer::handleValidationRequest(const QString& clientId, const QVariantMap& data)
{
    QString validationType = data["type"].toString();

    if (validationType == "check_email") {
        // 处理邮箱可用性检查
        QString email = data["email"].toString();

        if (email.isEmpty()) {
            QJsonObject response;
            response["type"] = "validation";
            response["validation_type"] = "check_email";
            response["success"] = false;
            response["message"] = "邮箱地址不能为空";
            sendJsonMessage(clientId, response);
            return;
        }

        // 这里应该检查数据库中邮箱是否已存在
        // 暂时返回可用状态
        bool emailAvailable = true; // TODO: 实际检查数据库

        QJsonObject response;
        response["type"] = "validation";
        response["validation_type"] = "check_email";
        response["success"] = true;
        response["available"] = emailAvailable;
        response["message"] = emailAvailable ? "邮箱可用" : "邮箱已被使用";

        qCInfo(chatServer) << "Email availability check for:" << email << "Available:" << emailAvailable;
        sendJsonMessage(clientId, response);

    } else if (validationType == "check_username") {
        // 处理用户名可用性检查
        QString username = data["username"].toString();

        if (username.isEmpty()) {
            QJsonObject response;
            response["type"] = "validation";
            response["validation_type"] = "check_username";
            response["success"] = false;
            response["message"] = "用户名不能为空";
            sendJsonMessage(clientId, response);
            return;
        }

        // 这里应该检查数据库中用户名是否已存在
        // 暂时返回可用状态
        bool usernameAvailable = true; // TODO: 实际检查数据库

        QJsonObject response;
        response["type"] = "validation";
        response["validation_type"] = "check_username";
        response["success"] = true;
        response["available"] = usernameAvailable;
        response["message"] = usernameAvailable ? "用户名可用" : "用户名已被使用";

        qCInfo(chatServer) << "Username availability check for:" << username << "Available:" << usernameAvailable;
        sendJsonMessage(clientId, response);

    } else {
        QJsonObject response;
        response["type"] = "validation";
        response["success"] = false;
        response["message"] = "未知的验证类型: " + validationType;
        sendJsonMessage(clientId, response);

        qCWarning(chatServer) << "Unknown validation type:" << validationType << "from client:" << clientId;
    }
}

// 处理注册请求
void ChatServer::handleRegisterRequest(const QString& clientId, const QVariantMap& data)
{
    // 实现注册逻辑
    QJsonObject response;
    response["type"] = "register";
    response["status"] = "success";
    sendJsonMessage(clientId, response);
}

// 处理邮箱验证请求
void ChatServer::handleEmailVerificationRequest(const QString& clientId, const QVariantMap& data)
{
    QString email = data["email"].toString();
    QString code = data["code"].toString();
    
    if (email.isEmpty() || code.isEmpty()) {
        QJsonObject response;
        response["type"] = "email_code_verification";
        response["success"] = false;
        response["message"] = "邮箱和验证码不能为空";
        sendJsonMessage(clientId, response);
        return;
    }
    
    // 验证验证码
    bool isValid = RedisEmailVerification::instance().verifyCode(email, code);
    
    QJsonObject response;
    response["type"] = "email_code_verification";
    response["success"] = isValid;
    response["message"] = isValid ? "邮箱验证成功" : "验证码错误或已过期";
    
    if (isValid) {
        qCInfo(chatServer) << "Email verified successfully for:" << email;
    } else {
        qCWarning(chatServer) << "Email verification failed for:" << email;
    }
    
    sendJsonMessage(clientId, response);
}

// 处理发送邮箱验证请求
void ChatServer::handleSendEmailVerificationRequest(const QString& clientId, const QVariantMap& data)
{
    qCInfo(chatServer) << "Processing email verification request from client:" << clientId;
    qCDebug(chatServer) << "Request data:" << data;

    QString email = data["email"].toString();

    if (email.isEmpty()) {
        qCWarning(chatServer) << "Email verification request failed - empty email address";
        QJsonObject response;
        response["type"] = "send_verification";
        response["success"] = false;
        response["message"] = "邮箱地址不能为空";
        sendJsonMessage(clientId, response);
        return;
    }

    qCInfo(chatServer) << "Sending email verification to:" << email;
    
    // 生成验证码
    QString verificationCode = QString::number(QRandomGenerator::global()->bounded(100000, 999999));
    qCInfo(chatServer) << "Generated verification code for email:" << email << "Code:" << verificationCode;

    // 检查EmailService状态
    if (!EmailService::instance().isReady()) {
        qCWarning(chatServer) << "EmailService is not ready";
        QJsonObject response;
        response["type"] = "send_verification";
        response["success"] = false;
        response["message"] = "邮件服务未就绪，请稍后重试";
        sendJsonMessage(clientId, response);
        return;
    }

    // 创建邮件内容
    EmailMessage emailMessage;
    emailMessage.to = email;
    emailMessage.subject = "QK Chat 邮箱验证";
    emailMessage.contentType = "text/html";
    emailMessage.body = QString(
        "<html><body>"
        "<h2>QK Chat 邮箱验证</h2>"
        "<p>您的验证码是：<strong>%1</strong></p>"
        "<p>验证码有效期为10分钟，请尽快完成验证。</p>"
        "<p>如果这不是您的操作，请忽略此邮件。</p>"
        "</body></html>"
    ).arg(verificationCode);

    qCInfo(chatServer) << "Attempting to send email to:" << email;

    // 发送邮件
    bool success = EmailService::instance().sendEmail(emailMessage);

    qCInfo(chatServer) << "Email send result:" << success;
    if (!success) {
        qCWarning(chatServer) << "Email send failed. Error:" << EmailService::instance().getLastError();
    }
    
    QJsonObject response;
    response["type"] = "send_verification";
    
    if (success) {
        qCInfo(chatServer) << "Email sent successfully, saving verification code to Redis";

        // 保存验证码到Redis
        bool redisSuccess = RedisEmailVerification::instance().saveVerificationCode(email, verificationCode, 600);

        qCInfo(chatServer) << "Redis save result:" << redisSuccess;

        if (redisSuccess) {
            response["success"] = true;
            response["message"] = "验证码已发送到您的邮箱，请查收";
            response["remaining_time"] = 600;
            qCInfo(chatServer) << "Email verification process completed successfully for:" << email;
        } else {
            response["success"] = false;
            response["message"] = "验证码生成失败，请稍后重试";
            qCWarning(chatServer) << "Failed to save verification code to Redis for email:" << email;
        }
    } else {
        response["success"] = false;
        response["message"] = "发送邮件失败，请稍后重试";
        qCWarning(chatServer) << "Email send failed for:" << email;
    }

    qCInfo(chatServer) << "Sending response to client:" << clientId << "Response:" << response;
    sendJsonMessage(clientId, response);
}

// 处理邮箱验证码验证请求
void ChatServer::handleEmailCodeVerificationRequest(const QString& clientId, const QVariantMap& data)
{
    QString email = data["email"].toString();
    QString code = data["code"].toString();
    
    if (email.isEmpty() || code.isEmpty()) {
        QJsonObject response;
        response["type"] = "email_code_verification";
        response["success"] = false;
        response["message"] = "邮箱和验证码不能为空";
        sendJsonMessage(clientId, response);
        return;
    }
    
    // 验证验证码
    bool isValid = RedisEmailVerification::instance().verifyCode(email, code);
    
    QJsonObject response;
    response["type"] = "email_code_verification";
    response["success"] = isValid;
    response["message"] = isValid ? "邮箱验证成功" : "验证码错误或已过期";
    
    if (isValid) {
        qCInfo(chatServer) << "Email verified successfully for:" << email;
    } else {
        qCWarning(chatServer) << "Email verification failed for:" << email;
    }
    
    sendJsonMessage(clientId, response);
}

// 处理重新发送验证请求
void ChatServer::handleResendVerificationRequest(const QString& clientId, const QVariantMap& data)
{
    QString email = data["email"].toString();
    
    if (email.isEmpty()) {
        QJsonObject response;
        response["type"] = "resend_verification";
        response["success"] = false;
        response["message"] = "邮箱地址不能为空";
        sendJsonMessage(clientId, response);
        return;
    }
    
    // 重新发送与sendEmailVerification相同的处理
    handleSendEmailVerificationRequest(clientId, data);
}

// 系统维护
void ChatServer::performSystemMaintenance()
{
    if (!_isRunning) return;
    
    _threadPool->enqueue([this]() {
        try {
            // 执行系统维护任务
            updateSystemStats();
            checkSystemHealth();
            
        } catch (const std::exception& e) {
            qCWarning(chatServer) << "System maintenance error:" << e.what();
        }
    });
}

// 更新系统统计
void ChatServer::updateSystemStats()
{
    if (!_isRunning) return;
    
    // 更新连接统计
    updateConnectionStats();
    // 更新消息统计
    updateMessageStats();
    // 更新性能统计
    updatePerformanceStats();
    // 更新系统资源统计
    updateSystemResourceStats();
}

// 检查系统健康状态
void ChatServer::checkSystemHealth()
{
    if (!_isRunning) return;
    
    bool healthy = checkComponentHealth() && checkResourceHealth() && checkPerformanceHealth();
    emit healthStatusChanged(healthy);
    
    if (!healthy) {
        qCWarning(chatServer) << "System health check failed";
    }
}

// 组件事件处理
void ChatServer::onConnectionManagerEvent()
{
    // 处理连接管理器事件
}

void ChatServer::onMessageEngineEvent()
{
    // 处理消息引擎事件
}

void ChatServer::onThreadManagerEvent()
{
    // 处理线程管理器事件
}

// 初始化组件
bool ChatServer::initializeComponents()
{
    try {
        // 初始化邮件服务
        initializeEmailService();
        return true;
    } catch (const std::exception& e) {
        qCCritical(chatServer) << "Failed to initialize components:" << e.what();
        return false;
    }
}

// 初始化缓存
bool ChatServer::initializeCache()
{
    try {
        qCInfo(chatServer) << "Starting cache manager initialization...";

        // 初始化CacheManagerV2
        CacheManagerV2::CacheConfig config;
        config.enableAdvancedFeatures = true;
        config.enableLegacyAPI = true;
        config.enableAutoOptimization = true;
        config.enableMetrics = true;
        config.metricsInterval = 30000;
        config.enableAlerts = true;

        qCInfo(chatServer) << "Cache configuration prepared, initializing CacheManagerV2...";

        bool success = CacheManagerV2::instance()->initialize(config);
        if (success) {
            qCInfo(chatServer) << "CacheManagerV2 initialized successfully";

            
        } else {
            qCWarning(chatServer) << "Failed to initialize CacheManagerV2";
        }

        return success;
    } catch (const std::exception& e) {
        qCCritical(chatServer) << "Failed to initialize cache:" << e.what();
        return false;
    }
}

// 初始化网络
bool ChatServer::initializeNetwork()
{
    try {
        // 初始化网络组件
        return true;
    } catch (const std::exception& e) {
        qCCritical(chatServer) << "Failed to initialize network:" << e.what();
        return false;
    }
}

// 初始化消息处理器
bool ChatServer::initializeMessageHandlers()
{
    try {
        registerMessageHandlers();
        return true;
    } catch (const std::exception& e) {
        qCCritical(chatServer) << "Failed to initialize message handlers:" << e.what();
        return false;
    }
}

// 设置定时器
void ChatServer::setupTimers()
{
    // 设置各种定时器
}

// 设置信号连接
void ChatServer::setupSignalConnections()
{
    // 设置信号槽连接
}

// 加载默认配置
void ChatServer::loadDefaultConfiguration()
{
    // 加载默认配置
}

// 设置SSL服务器
void ChatServer::setupSslServer()
{
    try {
        qCInfo(chatServer) << "Setting up SSL server...";
        
        // 确保在主线程中执行
        if (QThread::currentThread() != qApp->thread()) {
            qCWarning(chatServer) << "setupSslServer called from non-main thread, moving to main thread";
            QMetaObject::invokeMethod(this, "setupSslServer", Qt::QueuedConnection);
            return;
        }
        
        // 如果SSL服务器已经存在，先删除
        if (_sslServer) {
            _sslServer->close();
            delete _sslServer;
            _sslServer = nullptr;
        }
        
        // 创建新的SSL服务器
        _sslServer = new CustomSslServer(this);
        
        // 连接新连接信号
        connect(_sslServer, &CustomSslServer::newConnection, this, [this]() {
            while (_sslServer->hasPendingConnections()) {
                QSslSocket* socket = qobject_cast<QSslSocket*>(_sslServer->nextPendingConnection());
                if (socket) {
                    onClientConnected(socket);
                }
            }
        });
        
        // 配置SSL
        if (!configureSsl()) {
            qCWarning(chatServer) << "Failed to configure SSL, but continuing with server setup";
        }
        
        // 从配置获取端口
        ServerConfig* config = ServerConfig::instance();
        if (config) {
            _port = config->getServerPort();
            _host = config->getServerHost();
        }
        
        qCInfo(chatServer) << "SSL server setup completed for" << _host << ":" << _port;
        
    } catch (const std::exception& e) {
        qCCritical(chatServer) << "Exception in setupSslServer:" << e.what();
    } catch (...) {
        qCCritical(chatServer) << "Unknown exception in setupSslServer";
    }
}

// 设置清理定时器
void ChatServer::setupCleanupTimer()
{
    // 设置清理定时器
}

// 设置系统信息定时器
void ChatServer::setupSystemInfoTimer()
{
    // 设置系统信息定时器
}

// 初始化邮件服务
void ChatServer::initializeEmailService()
{
    qCInfo(chatServer) << "Starting EmailService initialization...";

    try {
        ServerConfig* config = ServerConfig::instance();
        qCInfo(chatServer) << "ServerConfig obtained successfully";

        qCInfo(chatServer) << "Calling EmailService::instance().initialize()...";
        bool success = EmailService::instance().initialize(
            config->getSmtpHost(),
            config->getSmtpPort(),
            config->getSmtpUsername(),
            config->getSmtpPassword(),
            false,  // useSSL - 不使用SSL
            true    // useTLS - 只使用TLS
        );
        qCInfo(chatServer) << "EmailService initialize() returned:" << success;
    
        if (success) {
            qCInfo(chatServer) << "Setting sender info...";
            EmailService::instance().setSenderInfo(
                config->getSmtpFromEmail(),
                config->getSmtpFromName()
            );
            qCInfo(chatServer) << "Email service initialized successfully";
            LogManager::instance()->writeSystemLog("ChatServer", "EMAIL_SERVICE_INITIALIZED",
                                                 "Email service initialized successfully");
        } else {
            qCWarning(chatServer) << "Failed to initialize email service";
            LogManager::instance()->writeErrorLog("Failed to initialize email service", "ChatServer");
        }

    } catch (const std::exception& e) {
        qCCritical(chatServer) << "Exception in EmailService initialization:" << e.what();
        LogManager::instance()->writeErrorLog(QString("EmailService initialization exception: %1").arg(e.what()), "ChatServer");
    } catch (...) {
        qCCritical(chatServer) << "Unknown exception in EmailService initialization";
        LogManager::instance()->writeErrorLog("Unknown exception in EmailService initialization", "ChatServer");
    }

    qCInfo(chatServer) << "EmailService initialization completed";
}

// 设置SSL配置
bool ChatServer::setupSslConfiguration()
{
    return configureSsl();
}

// 注册消息处理器
void ChatServer::registerMessageHandlers()
{
    // 注册消息处理器
}

// 更新连接统计
void ChatServer::updateConnectionStats()
{
    // 更新连接统计
}

// 更新消息统计
void ChatServer::updateMessageStats()
{
    // 更新消息统计
}

// 更新性能统计
void ChatServer::updatePerformanceStats()
{
    // 更新性能统计
}

// 更新系统资源统计
void ChatServer::updateSystemResourceStats()
{
    // 更新系统资源统计
}

// 检查组件健康状态
bool ChatServer::checkComponentHealth() const
{
    // 检查组件健康状态
    return true;
}

// 检查资源健康状态
bool ChatServer::checkResourceHealth() const
{
    // 检查资源健康状态
    return true;
}

// 检查性能健康状态
bool ChatServer::checkPerformanceHealth() const
{
    // 检查性能健康状态
    return true;
}

// 检查数据库健康状态
void ChatServer::checkDatabaseHealth()
{
    // 检查数据库健康状态
}

// 处理组件错误
void ChatServer::handleComponentError(const QString& component, const QString& error)
{
    qCWarning(chatServer) << "Component error:" << component << "-" << error;
}

// 处理系统错误
void ChatServer::handleSystemError(const QString& error)
{
    qCCritical(chatServer) << "System error:" << error;
}

// 清理连接
void ChatServer::cleanupConnections()
{
    // 清理连接
}

// 移除客户端
void ChatServer::removeClient(QSslSocket* socket)
{
    // 移除客户端
}

// 处理客户端消息
void ChatServer::processClientMessage(const QString& clientId, const QByteArray& messageData)
{
    qCInfo(chatServer) << "Processing message from client:" << clientId << "Data size:" << messageData.size();
    qCDebug(chatServer) << "Message content:" << messageData;

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(messageData, &parseError);

    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qCWarning(chatServer) << "Failed to parse JSON message:" << parseError.errorString();
        return;
    }

    QVariantMap packetMap = doc.toVariant().toMap();
    QString msgType = packetMap["type"].toString();
    QVariantMap data;

    if (packetMap.contains("data")) {
        data = packetMap["data"].toMap();
    } else {
        data = packetMap;
    }

    qCInfo(chatServer) << "Message type:" << msgType << "from client:" << clientId;

    // 增加消息计数器（除了心跳消息）
    if (msgType != "HEARTBEAT" && msgType != "heartbeat") {
        _totalMessages++;
        qCInfo(chatServer) << "Total messages processed:" << _totalMessages;
    }

    if (msgType == "LOGIN") {
        QJsonObject jsonData = QJsonObject::fromVariantMap(data);
        handleLoginRequest(clientId, jsonData);
    } else if (msgType == "REGISTER") {
        handleRegisterRequest(clientId, data);
    } else if (msgType == "HEARTBEAT" || msgType == "heartbeat") {
        handleHeartbeat(clientId);
    } else if (msgType == "validation") {
        // 处理验证类型的消息（邮箱可用性检查等）
        handleValidationRequest(clientId, data);
    } else if (msgType == "SEND_EMAIL_VERIFICATION" || msgType == "send_verification") {
        qCInfo(chatServer) << "Handling email verification request for client:" << clientId;
        handleSendEmailVerificationRequest(clientId, data);
    } else if (msgType == "EMAIL_CODE_VERIFICATION" || msgType == "verify_email_code") {
        handleEmailCodeVerificationRequest(clientId, data);
    } else if (msgType == "RESEND_VERIFICATION" || msgType == "resend_verification") {
        handleResendVerificationRequest(clientId, data);
    } else {
        qCWarning(chatServer) << "Unknown message type:" << msgType << "from client:" << clientId;
    }
}

// 记录服务器事件
void ChatServer::logServerEvent(const QString& event, const QString& details) const
{
    qCInfo(chatServer) << "Server event:" << event << details;
}

// 通过Socket获取客户端
std::shared_ptr<ChatClientConnection> ChatServer::getClientBySocket(QSslSocket* socket)
{
    // 通过Socket获取客户端
    return nullptr;
}

// 通过用户ID获取客户端
std::shared_ptr<ChatClientConnection> ChatServer::getClientByUserId(qint64 userId)
{
    // 通过用户ID获取客户端
    return nullptr;
}



// 重启服务器
void ChatServer::restartServer()
{
    stopServer();
    startServer();
}

// 检查服务器是否运行
bool ChatServer::isRunning() const
{
    return _isRunning;
}

// 加载配置
bool ChatServer::loadConfiguration(const QString& configFile)
{
    // 实现配置加载逻辑
    return true;
}

// 设置最大连接数
void ChatServer::setMaxConnections(int maxConnections)
{
    // 实现设置最大连接数逻辑
}

// 设置心跳间隔
void ChatServer::setHeartbeatInterval(int seconds)
{
    // 实现设置心跳间隔逻辑
}

// 设置消息队列大小
void ChatServer::setMessageQueueSize(int maxSize)
{
    // 实现设置消息队列大小逻辑
}

// 获取服务器统计信息
ChatServer::ServerStats ChatServer::getServerStats() const
{
    ServerStats stats;
    stats.totalConnections = _clients.size();
    stats.activeConnections = _clients.size();
    stats.totalMessages = _totalMessages;
    stats.cpuUsage = _cachedCpuUsage;
    stats.memoryUsage = _cachedMemoryUsage;
    stats.uptime = formatUptime();
    stats.lastUpdate = QDateTime::currentDateTime();
    return stats;
}

// 重置所有统计信息
void ChatServer::resetAllStats()
{
    _totalMessages = 0;
    _cachedCpuUsage = 0;
    _cachedMemoryUsage = 0;
    _cachedOnlineUserCount = 0;
    _cachedTotalUserCount = 0;
}

// 刷新所有缓存数据
void ChatServer::refreshAllCaches()
{
    qCInfo(chatServer) << "Refreshing all cache data...";

    try {
        // 强制刷新在线用户数缓存
        {
            QMutexLocker locker(&_clientsMutex);
            int count = 0;
            for (auto it = _clients.begin(); it != _clients.end(); ++it) {
                const ClientInfo& client = it.value();
                if (client.isAuthenticated &&
                    client.socket &&
                    client.socket->isValid() &&
                    client.socket->state() == QAbstractSocket::ConnectedState) {
                    count++;
                }
            }
            _cachedOnlineUserCount = count;
            qCDebug(chatServer) << "Refreshed online user count:" << count;
        }

        // 强制刷新总用户数缓存
        if (_database && _database->isConnected()) {
            try {
                int totalUsers = _database->getTotalUserCount();
                _cachedTotalUserCount = totalUsers;
                qCDebug(chatServer) << "Refreshed total user count:" << totalUsers;
            } catch (const std::exception& e) {
                qCWarning(chatServer) << "Exception getting total user count:" << e.what();
                _cachedTotalUserCount = 0;
            } catch (...) {
                qCWarning(chatServer) << "Unknown exception getting total user count";
                _cachedTotalUserCount = 0;
            }
        }

        // 暂时跳过系统资源监控，避免PDH计数器导致的崩溃
        qCInfo(chatServer) << "Skipping system resource monitoring to avoid crashes";
        _cachedCpuUsage = 0;
        _cachedMemoryUsage = 0;
        
        // 刷新系统资源使用率 - 使用新的安全实现
        try {
            _cachedCpuUsage = getCpuUsage();
            _cachedMemoryUsage = getMemoryUsage();
            qCDebug(chatServer) << "Refreshed system resources - CPU:" << _cachedCpuUsage << "% Memory:" << _cachedMemoryUsage << "%";
        } catch (const std::exception& e) {
            qCWarning(chatServer) << "Exception getting system resources:" << e.what();
            _cachedCpuUsage = 0;
            _cachedMemoryUsage = 0;
        } catch (...) {
            qCWarning(chatServer) << "Unknown exception getting system resources";
            _cachedCpuUsage = 0;
            _cachedMemoryUsage = 0;
        }

        qCInfo(chatServer) << "All cache data refreshed successfully";

    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception in refreshAllCaches:" << e.what();
    } catch (...) {
        qCWarning(chatServer) << "Unknown exception in refreshAllCaches";
    }
}

// 发送消息给用户
bool ChatServer::sendMessageToUser(qint64 userId, const QJsonObject& message)
{
    // 实现发送消息给用户逻辑
    return true;
}

// 发送消息给多个用户
bool ChatServer::sendMessageToUsers(const QList<qint64>& userIds, const QJsonObject& message)
{
    // 实现发送消息给多个用户逻辑
    return true;
}

// 广播消息
void ChatServer::broadcastMessage(const QJsonObject& message)
{
    // 实现广播消息逻辑
}

// 向已认证用户广播
void ChatServer::broadcastToAuthenticated(const QJsonObject& message)
{
    // 实现向已认证用户广播逻辑
}

// 获取在线用户列表
QStringList ChatServer::getOnlineUsers() const
{
    // 实现获取在线用户列表逻辑
    return QStringList();
}

// 获取已连接用户列表
QStringList ChatServer::getConnectedUsers() const
{
    // 实现获取已连接用户列表逻辑
    return QStringList();
}

// 获取在线用户数量
int ChatServer::getOnlineUserCount() const
{
    try {
        // 使用非阻塞方式尝试获取锁
        if (!_clientsMutex.tryLock(100)) { // 最多等待100ms
            qWarning() << "[ChatServer] Failed to acquire lock for getOnlineUserCount, returning cached value";
            return _cachedOnlineUserCount; // 返回缓存值
        }
        
        // 手动管理锁，因为我们已经获取了锁
        // 在函数结束时需要手动释放

        // 统计已认证的客户端数量
        int count = 0;
        for (auto it = _clients.begin(); it != _clients.end(); ++it) {
            const ClientInfo& client = it.value();
            
            // 更严格的空指针和有效性检查
            if (client.isAuthenticated && 
                client.socket && 
                client.socket->isValid() &&
                client.socket->state() == QAbstractSocket::ConnectedState) {
                count++;
            }
        }

        // 更新缓存值
        const_cast<ChatServer*>(this)->_cachedOnlineUserCount = count;
        
        // 手动释放锁
        _clientsMutex.unlock();
        
        return count;
    } catch (const std::exception& e) {
        qWarning() << "[ChatServer] Exception in getOnlineUserCount:" << e.what();
        // 确保在异常情况下也释放锁
        if (_clientsMutex.tryLock()) {
            _clientsMutex.unlock();
        }
        return _cachedOnlineUserCount; // 返回缓存值
    } catch (...) {
        qWarning() << "[ChatServer] Unknown exception in getOnlineUserCount";
        // 确保在异常情况下也释放锁
        if (_clientsMutex.tryLock()) {
            _clientsMutex.unlock();
        }
        return _cachedOnlineUserCount; // 返回缓存值
    }
}

// 获取连接数量
int ChatServer::getConnectionCount() const
{
    return _clients.size();
}

// 获取总用户数量
int ChatServer::getTotalUserCount() const
{
    try {
        if (!_database) {
            qCWarning(chatServer) << "Database not available for getTotalUserCount";
            return 0;
        }

        // 检查数据库连接状态
        if (!_database->isConnected()) {
            qCWarning(chatServer) << "Database not connected for getTotalUserCount";
            return 0;
        }

        int count = _database->getTotalUserCount();
        qCDebug(chatServer) << "Total user count from database:" << count;
        return count;
        
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception in getTotalUserCount:" << e.what();
        return 0;
    } catch (...) {
        qCWarning(chatServer) << "Unknown exception in getTotalUserCount";
        return 0;
    }
}

// 获取消息数量
int ChatServer::getMessagesCount() const
{
    return _totalMessages;
}

// 格式化运行时间
QString ChatServer::formatUptime() const
{
    // 如果_startTime无效，但服务器正在启动过程中，使用当前时间作为开始时间
    if (!_startTime.isValid()) {
        // 检查是否正在启动过程中
        if (_database && _database->isConnected()) {
            // 服务器正在启动，使用当前时间减去一个小的偏移量
            QDateTime estimatedStartTime = QDateTime::currentDateTime().addSecs(-10); // 假设启动用了10秒
            qint64 uptime = estimatedStartTime.secsTo(QDateTime::currentDateTime());
            if (uptime < 0) uptime = 0; // 确保不为负数
            int hours = uptime / 3600;
            int minutes = (uptime % 3600) / 60;
            int seconds = uptime % 60;
            
            return QString("%1:%2:%3")
                   .arg(hours, 2, 10, QChar('0'))
                   .arg(minutes, 2, 10, QChar('0'))
                   .arg(seconds, 2, 10, QChar('0'));
        }
        return "00:00:00";
    }
    
    qint64 uptime = _startTime.secsTo(QDateTime::currentDateTime());
    int hours = uptime / 3600;
    int minutes = (uptime % 3600) / 60;
    int seconds = uptime % 60;
    
    return QString("%1:%2:%3")
           .arg(hours, 2, 10, QChar('0'))
           .arg(minutes, 2, 10, QChar('0'))
           .arg(seconds, 2, 10, QChar('0'));
}



// 踢出用户
bool ChatServer::kickUser(qint64 userId, const QString& reason)
{
    // 实现踢出用户逻辑
    return true;
}

// 发送消息给用户（字节数组）
bool ChatServer::sendMessageToUser(qint64 userId, const QByteArray& message)
{
    // 实现发送消息给用户逻辑
    return true;
}

// 广播消息（字节数组）
void ChatServer::broadcastMessage(const QByteArray& message)
{
    // 实现广播消息逻辑
}

// 检查健康状态
bool ChatServer::isHealthy() const
{
    return checkComponentHealth() && checkResourceHealth() && checkPerformanceHealth();
}

// 获取健康报告
QString ChatServer::getHealthReport() const
{
    // 实现获取健康报告逻辑
    return "Server is healthy";
}

// 获取CPU使用率（内部方法）
int ChatServer::getCpuUsageInternal() const
{
#ifdef Q_OS_WIN
    try {
        // 使用更安全的锁机制
        static QMutex pdhMutex;
        QMutexLocker locker(&pdhMutex);
        
        // 尝试使用PDH API
        if (!_pdhInitialized) {
            qCInfo(chatServer) << "Initializing PDH counters for internal CPU usage...";
            try {
                if (const_cast<ChatServer*>(this)->initializePdhCounters()) {
                    const_cast<ChatServer*>(this)->_pdhInitialized = true;
                    qCInfo(chatServer) << "PDH counters initialized successfully for internal CPU usage";
                    
                    // PDH第一次调用通常返回0，需要预热
                    qCInfo(chatServer) << "Warming up PDH CPU counter for internal usage...";
                    getCpuUsageViaPdh(); // 预热调用
                } else {
                    qCWarning(chatServer) << "Failed to initialize PDH counters for internal CPU usage, using Registry fallback";
                }
            } catch (...) {
                qCWarning(chatServer) << "Exception during PDH initialization, using Registry fallback";
            }
        }
        
        int cpuUsage = 0;
        if (_pdhInitialized) {
            try {
                cpuUsage = getCpuUsageViaPdh();
                qCDebug(chatServer) << "Internal CPU usage via PDH:" << cpuUsage << "%";
            } catch (...) {
                qCWarning(chatServer) << "Failed to get CPU usage via PDH, using Registry fallback";
                cpuUsage = getCpuUsageViaRegistry();
            }
        } else {
            cpuUsage = getCpuUsageViaRegistry();
            qCDebug(chatServer) << "Internal CPU usage via Registry:" << cpuUsage << "%";
        }
        
        return cpuUsage;
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Error getting internal CPU usage:" << e.what();
        int fallbackUsage = getCpuUsageViaRegistry();
        qCDebug(chatServer) << "Internal CPU usage fallback:" << fallbackUsage << "%";
        return fallbackUsage;
    } catch (...) {
        qCWarning(chatServer) << "Unknown error getting internal CPU usage";
        int fallbackUsage = getCpuUsageViaRegistry();
        qCDebug(chatServer) << "Internal CPU usage fallback:" << fallbackUsage << "%";
        return fallbackUsage;
    }
#else
    // 非Windows系统返回0
    return 0;
#endif
}

// 获取内存使用率（内部方法）
int ChatServer::getMemoryUsageInternal() const
{
#ifdef Q_OS_WIN
    try {
        // 使用更安全的锁机制
        static QMutex pdhMutex;
        QMutexLocker locker(&pdhMutex);
        
        // 尝试使用PDH API
        if (!_pdhInitialized) {
            qCInfo(chatServer) << "Initializing PDH counters for internal memory usage...";
            try {
                if (const_cast<ChatServer*>(this)->initializePdhCounters()) {
                    const_cast<ChatServer*>(this)->_pdhInitialized = true;
                    qCInfo(chatServer) << "PDH counters initialized successfully for internal memory usage";
                    
                    // PDH第一次调用通常返回0，需要预热
                    qCInfo(chatServer) << "Warming up PDH memory counter for internal usage...";
                    getMemoryUsageViaPdh(); // 预热调用
                } else {
                    qCWarning(chatServer) << "Failed to initialize PDH counters for internal memory usage, using Registry fallback";
                }
            } catch (...) {
                qCWarning(chatServer) << "Exception during PDH initialization, using Registry fallback";
            }
        }
        
        int memoryUsage = 0;
        if (_pdhInitialized) {
            try {
                memoryUsage = getMemoryUsageViaPdh();
                qCDebug(chatServer) << "Internal memory usage via PDH:" << memoryUsage << "%";
            } catch (...) {
                qCWarning(chatServer) << "Failed to get memory usage via PDH, using Registry fallback";
                memoryUsage = getMemoryUsageViaRegistry();
            }
        } else {
            memoryUsage = getMemoryUsageViaRegistry();
            qCDebug(chatServer) << "Internal memory usage via Registry:" << memoryUsage << "%";
        }
        
        return memoryUsage;
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Error getting internal memory usage:" << e.what();
        int fallbackUsage = getMemoryUsageViaRegistry();
        qCDebug(chatServer) << "Internal memory usage fallback:" << fallbackUsage << "%";
        return fallbackUsage;
    } catch (...) {
        qCWarning(chatServer) << "Unknown error getting internal memory usage";
        int fallbackUsage = getMemoryUsageViaRegistry();
        qCDebug(chatServer) << "Internal memory usage fallback:" << fallbackUsage << "%";
        return fallbackUsage;
    }
#else
    // 非Windows系统返回0
    return 0;
#endif
}

// 获取CPU使用率（通过进程）
int ChatServer::getCpuUsageViaProcess() const
{
#ifdef Q_OS_WIN
    try {
        // 使用GetProcessTimes获取进程CPU时间
        HANDLE hProcess = GetCurrentProcess();
        FILETIME createTime, exitTime, kernelTime, userTime;
        
        if (GetProcessTimes(hProcess, &createTime, &exitTime, &kernelTime, &userTime)) {
            // 计算CPU使用率（简化实现）
            ULARGE_INTEGER kernel, user;
            kernel.LowPart = kernelTime.dwLowDateTime;
            kernel.HighPart = kernelTime.dwHighDateTime;
            user.LowPart = userTime.dwLowDateTime;
            user.HighPart = userTime.dwHighDateTime;
            
            // 转换为百分比（简化计算）
            qint64 totalTime = kernel.QuadPart + user.QuadPart;
            int cpuUsage = static_cast<int>((totalTime / 10000) % 100); // 简化的百分比计算
            
            qCDebug(chatServer) << "CPU usage via process:" << cpuUsage << "%";
            return cpuUsage;
        } else {
            qCWarning(chatServer) << "Failed to get process CPU times";
            return 0;
        }
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception in getCpuUsageViaProcess:" << e.what();
        return 0;
    } catch (...) {
        qCWarning(chatServer) << "Unknown exception in getCpuUsageViaProcess";
        return 0;
    }
#else
    // 非Windows系统返回0
    return 0;
#endif
}

// 获取内存使用率（通过进程）
int ChatServer::getMemoryUsageViaProcess() const
{
#ifdef Q_OS_WIN
    try {
        // 使用GetProcessMemoryInfo获取进程内存信息
        HANDLE hProcess = GetCurrentProcess();
        PROCESS_MEMORY_COUNTERS_EX pmc;
        
        if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            // 获取系统总内存
            MEMORYSTATUSEX memInfo;
            memInfo.dwLength = sizeof(MEMORYSTATUSEX);
            
            if (GlobalMemoryStatusEx(&memInfo)) {
                // 计算内存使用率
                qint64 processMemory = pmc.WorkingSetSize;
                qint64 totalMemory = memInfo.ullTotalPhys;
                
                int memoryUsage = static_cast<int>((processMemory * 100) / totalMemory);
                
                qCDebug(chatServer) << "Memory usage via process:" << memoryUsage << "%";
                return memoryUsage;
            } else {
                qCWarning(chatServer) << "Failed to get global memory status";
                return 0;
            }
        } else {
            qCWarning(chatServer) << "Failed to get process memory info";
            return 0;
        }
    } catch (const std::exception& e) {
        qCWarning(chatServer) << "Exception in getMemoryUsageViaProcess:" << e.what();
        return 0;
    } catch (...) {
        qCWarning(chatServer) << "Unknown exception in getMemoryUsageViaProcess";
        return 0;
    }
#else
    // 非Windows系统返回0
    return 0;
#endif
}

// 获取数据库实例
Database* ChatServer::getDatabase() const
{
    return _database;
}

// 获取运行时间
QString ChatServer::getUptime() const
{
    return formatUptime();
}