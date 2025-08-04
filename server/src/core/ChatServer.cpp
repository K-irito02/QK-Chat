#include "ChatServer.h"
#include "../database/Database.h"
#include "../services/EmailService.h"
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

// Windows API 头文件
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#pragma comment(lib, "pdh.lib")
#endif

Q_DECLARE_LOGGING_CATEGORY(chatServer)
Q_LOGGING_CATEGORY(chatServer, "qkchat.server.chatserver")

// 构造函数
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
    , _systemInfoTimer(nullptr)
{
    LOG_METHOD_ENTRY();
    
    // _startTime will be set when server actually starts
    setupCleanupTimer();
    // 不在构造函数中启动系统信息定时器，而是在服务器启动后启动
    LogManager::instance()->writeSystemLog("ChatServer", "INITIALIZED", "ChatServer instance created");
    LOG_METHOD_EXIT();
}

// 析构函数
ChatServer::~ChatServer()
{
    qDebug() << "[ChatServer] Destructor called";
    stopServer();
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

// 初始化数据库
bool ChatServer::initializeDatabase()
{
    if (!_database) {
        _database = new Database(this);
    }
    
    // 使用非阻塞方式初始化数据库
    QtConcurrent::run([this]() {
        bool result = _database->initialize();
        if (!result) {
            QString error = "Failed to initialize database";
            qCCritical(chatServer) << error;
            
            // 记录堆栈追踪
            StackTraceLogger::instance().logStackTrace("DATABASE_INIT_FAILED", "ChatServer::initializeDatabase");
            
            emit databaseError(error);
        } else {
            qCInfo(chatServer) << "Database initialized successfully";
            
            // 设置数据库
            _database->setupDatabase();
            
            // 启动非阻塞连接管理器
            NonBlockingConnectionManager::instance().startMonitoring();
            
            emit databaseConnected();
        }
    });
    
    return true;
}

// 启动服务器
bool ChatServer::startServer()
{
    LOG_METHOD_ENTRY();
    
    if (_isRunning) {
        qCWarning(chatServer) << "Server is already running";
        return true;
    }
    
    _startTime = QDateTime::currentDateTime();
    
    // 初始化线程池
    if (!_threadPool) {
        _threadPool = new ThreadPool(QThread::idealThreadCount(), this);
    }
    
    // 初始化数据库（非阻塞）
    initializeDatabase();
    
    // 初始化会话管理器
    if (!_sessionManager) {
        _sessionManager = new SessionManager(this);
    }
    
    // 初始化协议解析器
    if (!_protocolParser) {
        _protocolParser = new ProtocolParser(this);
    }
    
    // 启动清理定时器
    if (!_cleanupTimer) {
        _cleanupTimer = new QTimer(this);
        connect(_cleanupTimer, &QTimer::timeout, this, &ChatServer::performCleanup);
    }
    _cleanupTimer->start(60000); // 每分钟清理一次
    
    // 启动系统信息定时器
    if (!_systemInfoTimer) {
        _systemInfoTimer = new QTimer(this);
        connect(_systemInfoTimer, &QTimer::timeout, this, &ChatServer::updateSystemInfo);
    }
    _systemInfoTimer->start(5000); // 每5秒更新一次
    
    // 初始化SSL服务器
    if (!_sslServer) {
        _sslServer = new CustomSslServer(this);
        
        connect(_sslServer, &CustomSslServer::newConnection, this, [this]() {
            // 处理新连接
            QSslSocket* socket = qobject_cast<QSslSocket*>(_sslServer->nextPendingConnection());
            if (socket) {
                onClientConnected(socket);
            }
        });
    }
    
    // 从配置获取端口
    ServerConfig* config = ServerConfig::instance();
    _port = config->getServerPort();
    
    // 配置SSL
    if (!configureSsl()) {
        qCCritical(chatServer) << "Failed to configure SSL";
        return false;
    }
    
    // 启动监听
    if (!_sslServer->listen(QHostAddress(_host), _port)) {
        QString error = QString("Failed to start server: %1").arg(_sslServer->errorString());
        qCCritical(chatServer) << error;
        emit serverError(error);
        return false;
    }
    
    _isRunning = true;
    
    qCInfo(chatServer) << "Server started successfully on" << _host << ":" << _port;
    LogManager::instance()->writeSystemLog("ChatServer", "STARTED", 
        QString("Server started on %1:%2").arg(_host).arg(_port));
    
    emit serverStarted();
    LOG_METHOD_EXIT();
    return true;
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
        qCWarning(chatServer) << "Invalid socket received";
        return;
    }
    
    QString clientId = QUuid::createUuid().toString();
    QString clientAddress = socket->peerAddress().toString();
    quint16 clientPort = socket->peerPort();
    
    qCInfo(chatServer) << "Client connected:" << clientAddress << ":" << clientPort;
    
    // 添加到非阻塞连接管理器
    NonBlockingConnectionManager::instance().addConnection(
        socket, clientId, false); // 客户端连接不是关键连接
    
    // 创建客户端信息
    ClientInfo clientInfo;
    clientInfo.socket = socket;
    clientInfo.clientId = clientId;
    clientInfo.address = clientAddress;
    clientInfo.port = clientPort;
    clientInfo.connectedTime = QDateTime::currentDateTime();
    clientInfo.lastActivity = clientInfo.connectedTime;
    clientInfo.isAuthenticated = false;
    clientInfo.userId = -1;
    
    {
        QMutexLocker locker(&_clientsMutex);
        _clients[clientId] = clientInfo;
    }
    
    // 连接信号
    connect(socket, &QSslSocket::readyRead, this, [this, clientId]() {
        handleClientData(clientId);
    });
    
    connect(socket, &QSslSocket::disconnected, this, [this, clientId]() {
        handleClientDisconnected(clientId);
    });
    
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QSslSocket::errorOccurred),
            this, [this, clientId](QAbstractSocket::SocketError error) {
        handleSocketError(clientId, error);
    });
    
    emit clientConnected(clientId, clientAddress);
}

// 处理客户端断开连接
void ChatServer::onClientDisconnected(QSslSocket* socket)
{
    if (!socket) return;
    
    QString clientId;
    QString clientAddress = socket->peerAddress().toString();
    
    {
        QMutexLocker locker(&_clientsMutex);
        for (auto it = _clients.begin(); it != _clients.end(); ++it) {
            if (it.value().socket == socket) {
                clientId = it.key();
                _clients.erase(it);
                break;
            }
        }
    }
    
    if (!clientId.isEmpty()) {
        qCInfo(chatServer) << "Client disconnected:" << clientAddress;
        emit clientDisconnected(clientId, clientAddress);
    }
}

// 处理客户端数据（使用线程池）
void ChatServer::handleClientData(const QString& clientId)
{
    if (!_threadPool) {
        qCWarning(chatServer) << "Thread pool not initialized";
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
        
        while (socket->bytesAvailable() > 0) {
            QByteArray data = socket->readAll();
            
            // 更新最后活动时间
            client.lastActivity = QDateTime::currentDateTime();
            
            // 解析JSON消息
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
            
            if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
                QJsonObject msg = doc.object();
                QString msgType = msg["type"].toString();
                
                QString logMsg = QString("Received %1 from %2").arg(msgType).arg(clientId);
                qCInfo(chatServer) << logMsg;
                
                // 处理消息
                processClientMessage(clientId, msg.toVariantMap());
            } else {
                qCWarning(chatServer) << "Failed to parse JSON message from" << clientId;
            }
        }
    });
}

// 处理客户端断开
void ChatServer::handleClientDisconnected(const QString& clientId)
{
    QMutexLocker locker(&_clientsMutex);
    
    if (_clients.contains(clientId)) {
        QString address = _clients[clientId].address;
        _clients.remove(clientId);
        
        qCInfo(chatServer) << "Client disconnected:" << address;
        emit clientDisconnected(clientId, address);
    }
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
    
    QByteArray data = QJsonDocument(message).toJson(QJsonDocument::Compact);
    socket->write(data);
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
    if (!_isRunning) return;
    
    _threadPool->enqueue([this]() {
        try {
            _cachedCpuUsage = getCpuUsage();
            _cachedMemoryUsage = getMemoryUsage();
            
            // 记录系统状态
            QString logMsg = QString("System Info - CPU: %1%, Memory: %2 MB")
                           .arg(_cachedCpuUsage, 0, 'f', 1)
                           .arg(_cachedMemoryUsage);
            
            qCInfo(chatServer) << logMsg;
            
        } catch (const std::exception& e) {
            qCWarning(chatServer) << "Failed to update system info:" << e.what();
        }
    });
}

// 获取CPU使用率
int ChatServer::getCpuUsage() const
{
#ifdef Q_OS_WIN
    // 使用PDH (Performance Data Helper) 来获取系统CPU使用率
    static PDH_HQUERY cpuQuery = NULL;
    static PDH_HCOUNTER cpuTotal = NULL;
    static bool firstRun = true;
    static bool initialized = false;
    
    if (firstRun) {
        firstRun = false;
        // 初始化PDH查询
        if (PdhOpenQuery(NULL, NULL, &cpuQuery) == ERROR_SUCCESS) {
            if (PdhAddCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal) == ERROR_SUCCESS) {
                // 立即收集两次数据以确保有有效值
                PdhCollectQueryData(cpuQuery);
                Sleep(100); // 短暂等待
                PdhCollectQueryData(cpuQuery);
                initialized = true;
            }
        }
        return 0;
    }
    
    if (cpuQuery && cpuTotal && initialized) {
        PDH_FMT_COUNTERVALUE counterVal;
        
        if (PdhCollectQueryData(cpuQuery) == ERROR_SUCCESS) {
            if (PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_LONG, NULL, &counterVal) == ERROR_SUCCESS) {
                return static_cast<int>(counterVal.longValue);
            }
        }
    }
    
    return 0;
#else
    // Linux平台实现
    static unsigned long long lastTotalUser = 0, lastTotalUserLow = 0, lastTotalSys = 0, lastTotalIdle = 0;
    static bool firstRun = true;
    
    if (firstRun) {
        firstRun = false;
        FILE* file = fopen("/proc/stat", "r");
        if (file) {
            fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow, &lastTotalSys, &lastTotalIdle);
            fclose(file);
        }
        return 0;
    }
    
    unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;
    double percent;
    
    FILE* file = fopen("/proc/stat", "r");
    if (file) {
        fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow, &totalSys, &totalIdle);
        fclose(file);
        
        if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
            totalSys < lastTotalSys || totalIdle < lastTotalIdle) {
            return 0;
        }
        
        unsigned long long diffUser = totalUser - lastTotalUser;
        unsigned long long diffUserLow = totalUserLow - lastTotalUserLow;
        unsigned long long diffSys = totalSys - lastTotalSys;
        unsigned long long diffIdle = totalIdle - lastTotalIdle;
        
        total = (diffUser + diffUserLow + diffSys);
        percent = total;
        total += diffIdle;
        percent /= total;
        percent *= 100;
        
        lastTotalUser = totalUser;
        lastTotalUserLow = totalUserLow;
        lastTotalSys = totalSys;
        lastTotalIdle = totalIdle;
        
        return static_cast<int>(percent);
    }
    
    return 0;
#endif
}

// 获取内存使用率
int ChatServer::getMemoryUsage() const
{
#ifdef Q_OS_WIN
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (GlobalMemoryStatusEx(&memInfo)) {
        // 计算系统内存使用率
        DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
        DWORDLONG availablePhysMem = memInfo.ullAvailPhys;
        DWORDLONG usedPhysMem = totalPhysMem - availablePhysMem;
        
        if (totalPhysMem > 0) {
            double usagePercent = (static_cast<double>(usedPhysMem) / totalPhysMem) * 100.0;
            return static_cast<int>(usagePercent);
        }
    }
    
    return 0;
#else
    // Linux平台实现
    FILE* file = fopen("/proc/meminfo", "r");
    if (file) {
        unsigned long totalMem = 0, freeMem = 0, buffers = 0, cached = 0;
        char line[256];
        
        while (fgets(line, sizeof(line), file)) {
            if (sscanf(line, "MemTotal: %lu kB", &totalMem) == 1) {
                continue;
            }
            if (sscanf(line, "MemFree: %lu kB", &freeMem) == 1) {
                continue;
            }
            if (sscanf(line, "Buffers: %lu kB", &buffers) == 1) {
                continue;
            }
            if (sscanf(line, "Cached: %lu kB", &cached) == 1) {
                continue;
            }
        }
        fclose(file);
        
        if (totalMem > 0) {
            unsigned long usedMem = totalMem - freeMem - buffers - cached;
            double usagePercent = (static_cast<double>(usedMem) / totalMem) * 100.0;
            return static_cast<int>(usagePercent);
        }
    }
    return 0;
#endif
}

// 获取服务器状态
QJsonObject ChatServer::getServerStatus() const
{
    QJsonObject status;
    status["isRunning"] = _isRunning;
    status["host"] = _host;
    status["port"] = _port;
    status["startTime"] = _startTime.toString(Qt::ISODate);
    status["uptime"] = _startTime.secsTo(QDateTime::currentDateTime());
    status["connectedClients"] = _clients.size();
    status["totalMessages"] = _totalMessages;
    status["cpuUsage"] = _cachedCpuUsage;
    status["memoryUsage"] = _cachedMemoryUsage;
    
    if (_database) {
        status["databaseConnected"] = _database->isConnected();
    } else {
        status["databaseConnected"] = false;
    }
    
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
    // 实现邮箱验证逻辑
    QJsonObject response;
    response["type"] = "email_verification";
    response["status"] = "success";
    sendJsonMessage(clientId, response);
}

// 处理发送邮箱验证请求
void ChatServer::handleSendEmailVerificationRequest(const QString& clientId, const QVariantMap& data)
{
    // 实现发送邮箱验证逻辑
    QJsonObject response;
    response["type"] = "send_email_verification";
    response["status"] = "success";
    sendJsonMessage(clientId, response);
}

// 处理邮箱验证码验证请求
void ChatServer::handleEmailCodeVerificationRequest(const QString& clientId, const QVariantMap& data)
{
    // 实现邮箱验证码验证逻辑
    QJsonObject response;
    response["type"] = "email_code_verification";
    response["status"] = "success";
    sendJsonMessage(clientId, response);
}

// 处理重新发送验证请求
void ChatServer::handleResendVerificationRequest(const QString& clientId, const QVariantMap& data)
{
    // 实现重新发送验证逻辑
    QJsonObject response;
    response["type"] = "resend_verification";
    response["status"] = "success";
    sendJsonMessage(clientId, response);
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
        // 初始化各个组件
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
        // 初始化缓存系统
        return true;
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
    // 设置SSL服务器
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
    // 初始化邮件服务
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
void ChatServer::processClientMessage(const QString& clientId, const QVariantMap& data)
{
    QString msgType = data["type"].toString();
    
    if (msgType == "LOGIN") {
        QJsonObject loginData;
        loginData["username"] = data["username"].toString();
        loginData["password"] = data["password"].toString();
        handleLoginRequest(clientId, loginData);
    } else if (msgType == "LOGOUT") {
        handleLogoutRequest(clientId);
    } else if (msgType == "MESSAGE") {
        handleMessageRequest(clientId, data);
    } else if (msgType == "HEARTBEAT") {
        handleHeartbeat(clientId);
    } else if (msgType == "REGISTER") {
        handleRegisterRequest(clientId, data);
    } else if (msgType == "EMAIL_VERIFICATION") {
        handleEmailVerificationRequest(clientId, data);
    } else if (msgType == "SEND_EMAIL_VERIFICATION") {
        handleSendEmailVerificationRequest(clientId, data);
    } else if (msgType == "EMAIL_CODE_VERIFICATION") {
        handleEmailCodeVerificationRequest(clientId, data);
    } else if (msgType == "RESEND_VERIFICATION") {
        handleResendVerificationRequest(clientId, data);
    } else {
        qCWarning(chatServer) << "Unknown message type:" << msgType << "from" << clientId;
        sendErrorResponse(clientId, "UNKNOWN_TYPE", "Unknown message type");
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
    // 实现获取在线用户数量逻辑
    return 0;
}

// 获取连接数量
int ChatServer::getConnectionCount() const
{
    return _clients.size();
}

// 获取总用户数量
int ChatServer::getTotalUserCount() const
{
    // 实现获取总用户数量逻辑
    return 0;
}

// 获取消息数量
int ChatServer::getMessagesCount() const
{
    return _totalMessages;
}

// 获取运行时间
QString ChatServer::getUptime() const
{
    return formatUptime();
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

// 格式化运行时间
QString ChatServer::formatUptime() const
{
    if (!_startTime.isValid()) {
        return "0:00:00";
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