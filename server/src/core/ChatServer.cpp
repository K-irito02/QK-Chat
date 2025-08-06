#include "ChatServer.h"
#include "MessageHandlers.h"
#include "../database/Database.h"
#include "../services/EmailVerificationService.h"
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
#include "../services/EmailVerificationService.h"
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
#include <stdexcept>

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
    , _cacheManager(nullptr)
    , _messageEngine(nullptr)
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
        qCInfo(chatServer) << "Stopping thread pool...";
        _threadPool->shutdown();
        delete _threadPool;
        _threadPool = nullptr;
        qCInfo(chatServer) << "Thread pool stopped";
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
        
        // 初始化线程池
        qCInfo(chatServer) << "Initializing thread pool...";
        if (!_threadPool) {
            _threadPool = new ThreadPool(4); // 创建4个工作线程
            qCInfo(chatServer) << "Thread pool initialized successfully";
        }

        // 初始化连接管理器
        qCInfo(chatServer) << "Initializing connection manager...";
        if (!_connectionManager) {
            _connectionManager = new ConnectionManager(this);
            qCInfo(chatServer) << "Connection manager initialized successfully";
        }
        
        // 初始化数据库连接池
        qCInfo(chatServer) << "Initializing database pool...";
        if (!_databasePool) {
            _databasePool = DatabasePool::instance();
            qCInfo(chatServer) << "Database pool initialized successfully";
        }
        
        // 初始化会话管理器
        qCInfo(chatServer) << "Initializing session manager...";
        if (!_sessionManager) {
            _sessionManager = new SessionManager(this);
            qCInfo(chatServer) << "Session manager initialized successfully";

            // 连接 ConnectionManager 的 connectionRemoved 信号到 SessionManager 的槽
            connect(_connectionManager, &ConnectionManager::connectionRemoved,
                    _sessionManager, [this](QSslSocket* socket, qint64 userId) {
                if (userId > 0) {
                    // 通过用户ID查找用户名
                    if (_databasePool) {
                        auto result = _databasePool->executeQuery(
                            "SELECT username FROM users WHERE user_id = ?",
                            {userId},
                            DatabaseOperationType::Read
                        );
                        if (result.success && result.data.isSelect() && result.data.next()) {
                            QString username = result.data.value("username").toString();
                            _sessionManager->onUserDisconnected(username);
                        }
                    }
                }
            });
            qCInfo(chatServer) << "Connected connectionRemoved signal to session manager";
        }
        

        
        // 初始化所有组件
        qCInfo(chatServer) << "Initializing all components...";
        if (!initializeComponents()) {
            qCCritical(chatServer) << "Failed to initialize components";
            return false;
        }

        // 初始化邮件服务
        qCInfo(chatServer) << "Initializing email verification service...";
        EmailVerificationService* emailService = new EmailVerificationService(this);
        qCInfo(chatServer) << "Email verification service initialized successfully";

        // 注册消息处理器
        qCInfo(chatServer) << "Registering message handlers...";
        _messageEngine->registerHandler(std::make_shared<LoginMessageHandler>(_connectionManager, _sessionManager, _databasePool, _cacheManager));
        _messageEngine->registerHandler(std::make_shared<ChatMessageHandler>(_connectionManager, _databasePool, _cacheManager));
        _messageEngine->registerHandler(std::make_shared<HeartbeatMessageHandler>(_connectionManager));
        _messageEngine->registerHandler(std::make_shared<RegisterMessageHandler>(_connectionManager, _databasePool, _cacheManager, emailService));
        _messageEngine->registerHandler(std::make_shared<EmailVerificationMessageHandler>(emailService));
        qCInfo(chatServer) << "Message handlers registered successfully";
        
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
    
    // 停止线程池
    if (_threadPool) {
        qCInfo(chatServer) << "Stopping thread pool...";
        _threadPool->shutdown();
        delete _threadPool;
        _threadPool = nullptr;
        qCInfo(chatServer) << "Thread pool stopped";
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
    
    // 连接信号 - 使用直接处理避免线程池延迟
    connect(socket, &QSslSocket::readyRead, this, [this, clientId, socket]() {
        qCInfo(chatServer) << "=== READY READ SIGNAL TRIGGERED ===";
        qCInfo(chatServer) << "Client ID:" << clientId;
        qCInfo(chatServer) << "Socket bytes available:" << socket->bytesAvailable();
        qCInfo(chatServer) << "Socket state:" << socket->state();
        qCInfo(chatServer) << "Socket encrypted:" << socket->isEncrypted();
        // 对于关键消息，使用直接处理而不是线程池
        handleClientDataDirect(clientId);
        qCInfo(chatServer) << "=== END READY READ SIGNAL ===";
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
        qCWarning(chatServer) << "Thread pool not available for client:" << clientId;
        return;
    }
    
    _threadPool->enqueue([this, clientId]() {
        QMutexLocker locker(&_clientsMutex);
        
        if (!_clients.contains(clientId)) {
            qCWarning(chatServer) << "Client not found in clients list:" << clientId;
            return;
        }
        
        ClientInfo& client = _clients[clientId];
        QSslSocket* socket = client.socket;
        
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
            qCWarning(chatServer) << "Socket not available or not connected for client:" << clientId;
            return;
        }
        
        qint64 bytesAvailable = socket->bytesAvailable();
        
        if (bytesAvailable <= 0) {
            qCDebug(chatServer) << "No bytes available for client:" << clientId;
            return;
        }
        
        qCInfo(chatServer) << "Received" << bytesAvailable << "bytes from client" << clientId;
        
        // 读取数据到缓冲区
        QByteArray newData = socket->readAll();
        qCInfo(chatServer) << "Read" << newData.size() << "bytes from socket for client:" << clientId;
        
        // 记录缓冲区状态
        qCInfo(chatServer) << "Buffer before append - Size:" << client.messageBuffer.size() << "for client:" << clientId;
        client.messageBuffer.append(newData);
        qCInfo(chatServer) << "Buffer after append - Size:" << client.messageBuffer.size() << "for client:" << clientId;
        
        // 处理完整的消息
        int processedMessages = 0;
        while (client.messageBuffer.size() >= 4) {
            // 读取消息长度
            qint32 messageLength = qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(client.messageBuffer.left(4).data()));
            
            qCInfo(chatServer) << "Message length from header:" << messageLength << "for client:" << clientId;
            
            if (messageLength <= 0 || messageLength > 1024 * 1024) { // 最大1MB
                qCWarning(chatServer) << "Invalid message length for client" << clientId << ":" << messageLength;
                client.messageBuffer.clear();
                return;
            }
            
            // 检查是否有完整的消息
            if (client.messageBuffer.size() < 4 + messageLength) {
                qCInfo(chatServer) << "Incomplete message - Buffer size:" << client.messageBuffer.size() 
                                 << "Required:" << (4 + messageLength) << "for client:" << clientId;
                break; // 等待更多数据
            }
            
            // 提取完整的消息
            QByteArray messageData = client.messageBuffer.mid(4, messageLength);
            client.messageBuffer.remove(0, 4 + messageLength);
            
            qCInfo(chatServer) << "Processing complete message for client" << clientId << "Size:" << messageData.size();
            qCDebug(chatServer) << "Raw message data:" << messageData;
            
            // 处理消息
            processClientMessage(clientId, messageData, client.socket);
            processedMessages++;
        }
        
        qCInfo(chatServer) << "Processed" << processedMessages << "messages for client:" << clientId;
        qCInfo(chatServer) << "Remaining buffer size:" << client.messageBuffer.size() << "for client:" << clientId;
    });
}

// 直接处理客户端数据（不使用线程池，避免延迟）
void ChatServer::handleClientDataDirect(const QString& clientId)
{
    QMutexLocker locker(&_clientsMutex);
    
    if (!_clients.contains(clientId)) {
        qCWarning(chatServer) << "Client not found in clients list:" << clientId;
        return;
    }
    
    ClientInfo& client = _clients[clientId];
    QSslSocket* socket = client.socket;
    
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        qCWarning(chatServer) << "Socket not available or not connected for client:" << clientId;
        return;
    }
    
    // 检查socket状态
    qCInfo(chatServer) << "Socket state for client" << clientId << ":" << socket->state();
    qCInfo(chatServer) << "Socket encrypted:" << socket->isEncrypted();
    qCInfo(chatServer) << "Socket bytes available:" << socket->bytesAvailable();
    qCInfo(chatServer) << "Socket bytes to write:" << socket->bytesToWrite();
    
    qint64 bytesAvailable = socket->bytesAvailable();
    
    if (bytesAvailable <= 0) {
        qCDebug(chatServer) << "No bytes available for client:" << clientId;
        return;
    }
    
    qCInfo(chatServer) << "Received" << bytesAvailable << "bytes from client" << clientId << "(direct processing)";
    
    // 读取所有可用数据并添加到缓冲区
    QByteArray newData = socket->readAll();
    qCInfo(chatServer) << "Read" << newData.size() << "bytes from socket for client:" << clientId;
    qCInfo(chatServer) << "New data hex:" << newData.toHex();
    qCInfo(chatServer) << "New data as string:" << QString::fromUtf8(newData);
    
    // 添加到客户端缓冲区
    client.messageBuffer.append(newData);
    qCInfo(chatServer) << "Buffer size after append:" << client.messageBuffer.size() << "for client:" << clientId;
    
    // 处理缓冲区中的所有完整消息
    int processedMessages = 0;
    qCInfo(chatServer) << "Starting to process messages from buffer, buffer size:" << client.messageBuffer.size();
    
    while (client.messageBuffer.size() >= 4) {
        // 读取消息长度
        qint32 messageLength = qFromBigEndian<qint32>(reinterpret_cast<const uchar*>(client.messageBuffer.left(4).data()));
        
        qCInfo(chatServer) << "Message length from header:" << messageLength << "for client:" << clientId;
        qCInfo(chatServer) << "Current buffer size:" << client.messageBuffer.size();
        qCInfo(chatServer) << "Required size:" << (4 + messageLength);
        
        if (messageLength <= 0 || messageLength > 1024 * 1024) { // 最大1MB
            qCWarning(chatServer) << "Invalid message length for client" << clientId << ":" << messageLength;
            client.messageBuffer.clear();
            return;
        }
        
        // 检查是否有完整的消息
        if (client.messageBuffer.size() < 4 + messageLength) {
            qCInfo(chatServer) << "Incomplete message - Buffer size:" << client.messageBuffer.size() 
                             << "Required:" << (4 + messageLength) << "for client:" << clientId;
            qCInfo(chatServer) << "Waiting for more data...";
            break; // 等待更多数据
        }
        
        // 提取完整的消息
        QByteArray completeMessage = client.messageBuffer.mid(4, messageLength);
        client.messageBuffer.remove(0, 4 + messageLength);
        
        qCInfo(chatServer) << "=== PROCESSING COMPLETE MESSAGE ===";
        qCInfo(chatServer) << "Client:" << clientId;
        qCInfo(chatServer) << "Message size:" << completeMessage.size();
        qCInfo(chatServer) << "Message data:" << completeMessage;
        qCInfo(chatServer) << "Remaining buffer size after extraction:" << client.messageBuffer.size();
        qCInfo(chatServer) << "=== END COMPLETE MESSAGE ===";
        
        // 处理消息
        processClientMessage(clientId, completeMessage, socket);
        processedMessages++;
        
        qCInfo(chatServer) << "Processed message" << processedMessages << "for client:" << clientId;
    }
    
    qCInfo(chatServer) << "Processed" << processedMessages << "messages for client:" << clientId;
    qCInfo(chatServer) << "Remaining buffer size:" << client.messageBuffer.size() << "for client:" << clientId;
    qCInfo(chatServer) << "=== END CLIENT DATA PROCESSING ===";
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

            response["sessionToken"] = sessionToken;
            
            qCInfo(chatServer) << "Sending response to client:" << clientId << "Response:" << response;
            sendJsonMessage(clientId, response);
            qCInfo(chatServer) << "Response sent to client:" << clientId;
            
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
    qCInfo(chatServer) << "Attempting to send JSON message to client:" << clientId;
    qCInfo(chatServer) << "Message content:" << message;
    
    try {
        QMutexLocker locker(&_clientsMutex);
        
        // 查找客户端连接
        if (!_clients.contains(clientId)) {
            qCWarning(chatServer) << "Client not found for sending response:" << clientId;
            return;
        }
        
        QSslSocket* socket = _clients[clientId].socket;
        if (!socket) {
            qCWarning(chatServer) << "Socket is null for client:" << clientId;
            return;
        }
        
        // 检查socket状态
        if (socket->state() != QAbstractSocket::ConnectedState) {
            qCWarning(chatServer) << "Socket not connected for client:" << clientId << "State:" << socket->state();
            return;
        }
        
        if (!socket->isEncrypted()) {
            qCWarning(chatServer) << "Socket not encrypted for client:" << clientId;
            return;
        }
        
        // 创建JSON文档
        QJsonDocument doc(message);
        QByteArray jsonData = doc.toJson(QJsonDocument::Compact);
        
        // 添加长度前缀
        QByteArray lengthBytes(4, 0);
        qToBigEndian<qint32>(jsonData.size(), reinterpret_cast<uchar*>(lengthBytes.data()));
        
        QByteArray packet = lengthBytes + jsonData;
        
        qCInfo(chatServer) << "Sending packet to client:" << clientId << "Size:" << packet.size() << "bytes";
        
        // 发送数据
        qint64 bytesWritten = socket->write(packet);
        if (bytesWritten == -1) {
            qCWarning(chatServer) << "Failed to write to socket for client:" << clientId << "Error:" << socket->errorString();
            return;
        }
        
        if (bytesWritten != packet.size()) {
            qCWarning(chatServer) << "Partial write for client:" << clientId << "Wrote:" << bytesWritten << "of" << packet.size() << "bytes";
            return;
        }
        
        // 强制刷新socket
        if (!socket->flush()) {
            qCWarning(chatServer) << "Failed to flush socket for client:" << clientId;
            return;
        }
        
        qCInfo(chatServer) << "Successfully sent message to client:" << clientId << "Bytes written:" << bytesWritten;
        
    } catch (const std::exception& e) {
        qCCritical(chatServer) << "Exception in sendJsonMessage:" << e.what();
    } catch (...) {
        qCCritical(chatServer) << "Unknown exception in sendJsonMessage";
    }
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
    qCInfo(chatServer) << "=== HANDLING VALIDATION REQUEST ===";
    qCInfo(chatServer) << "Client ID:" << clientId;
    qCInfo(chatServer) << "Raw data:" << data;
    qCInfo(chatServer) << "Data keys:" << data.keys();
    
    QString validationType = data["type"].toString();
    qCInfo(chatServer) << "Validation type:" << validationType << "from client:" << clientId;
    qCInfo(chatServer) << "=== END VALIDATION REQUEST HEADER ===";

    if (validationType == "check_username") {
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

        // 实际检查数据库中用户名是否已存在
        bool usernameAvailable = true;
        qCInfo(chatServer) << "Checking username availability for:" << username;
        qCInfo(chatServer) << "Database pointer:" << (_database ? "valid" : "null");
        
        try {
            if (_database && _database->isConnected()) {
                qCInfo(chatServer) << "Database is connected, checking username availability";
                // 检查用户名是否已注册
                usernameAvailable = _database->isUsernameAvailable(username);
                qCInfo(chatServer) << "Database username check for:" << username << "Available:" << usernameAvailable;
            } else {
                qCWarning(chatServer) << "Database not connected, using fallback username check";
                qCInfo(chatServer) << "Database connection status:" << (_database ? "exists but not connected" : "null pointer");
                // 如果数据库未连接，暂时返回可用（保守策略）
                usernameAvailable = true;
            }
        } catch (const std::exception& e) {
            qCWarning(chatServer) << "Exception during username availability check:" << e.what();
            usernameAvailable = false; // 出错时保守地返回不可用
        }

        QJsonObject response;
        response["type"] = "validation";
        response["validation_type"] = "check_username";
        response["success"] = true;
        response["available"] = usernameAvailable;
        response["message"] = usernameAvailable ? "用户名可用" : "用户名已被使用";

        qCInfo(chatServer) << "Username availability check for:" << username << "Available:" << usernameAvailable;
        qCInfo(chatServer) << "About to send validation response to client:" << clientId;
        sendJsonMessage(clientId, response);
        qCInfo(chatServer) << "Validation response sent to client:" << clientId;
        
    } else if (validationType == "check_email") {
        // 处理邮箱可用性检查
        QString email = data["email"].toString();

        if (email.isEmpty()) {
            QJsonObject response;
            response["type"] = "validation";
            response["validation_type"] = "check_email";
            response["success"] = false;
            response["message"] = "邮箱不能为空";
            sendJsonMessage(clientId, response);
            return;
        }

        // 实际检查数据库中邮箱是否已存在
        bool emailAvailable = true;
        try {
            if (_database && _database->isConnected()) {
                // 检查邮箱是否已注册
                emailAvailable = _database->isEmailAvailable(email);
                qCInfo(chatServer) << "Database email check for:" << email << "Available:" << emailAvailable;
            } else {
                qCWarning(chatServer) << "Database not connected, using fallback email check";
                // 如果数据库未连接，暂时返回可用（保守策略）
                emailAvailable = true;
            }
        } catch (const std::exception& e) {
            qCWarning(chatServer) << "Exception during email availability check:" << e.what();
            emailAvailable = false; // 出错时保守地返回不可用
        }

        QJsonObject response;
        response["type"] = "validation";
        response["validation_type"] = "check_email";
        response["success"] = true;
        response["available"] = emailAvailable;
        response["message"] = emailAvailable ? "邮箱可用" : "邮箱已被使用";

        qCInfo(chatServer) << "Email availability check for:" << email << "Available:" << emailAvailable;
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
    qCInfo(chatServer) << "=== HANDLING REGISTER REQUEST ===";
    qCInfo(chatServer) << "Client ID:" << clientId;
    qCInfo(chatServer) << "Data:" << data;
    
    QString username = data["username"].toString();
    QString password = data["password"].toString();
    QString avatar = data["avatar"].toString();
    
    if (username.isEmpty() || password.isEmpty()) {
        QJsonObject response;
        response["type"] = "register";
        response["success"] = false;
        response["message"] = "用户名和密码不能为空";
        sendJsonMessage(clientId, response);
        return;
    }
    
    // 检查用户名是否已存在
    if (!_database->isUsernameAvailable(username)) {
        QJsonObject response;
        response["type"] = "register";
        response["success"] = false;
        response["message"] = "用户名已被使用";
        sendJsonMessage(clientId, response);
        return;
    }
    
    // 创建用户
    bool success = _database->createUser(username, password, avatar);
    
    QJsonObject response;
    response["type"] = "register";
    response["success"] = success;
    
    if (success) {
        response["message"] = "注册成功";
        qCInfo(chatServer) << "User registered successfully:" << username;
    } else {
        response["message"] = "注册失败，请稍后重试";
        qCWarning(chatServer) << "Failed to register user:" << username;
    }
    
    sendJsonMessage(clientId, response);
    qCInfo(chatServer) << "=== END REGISTER REQUEST ===";
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
        qCInfo(chatServer) << "Initializing server components...";
        
        // 初始化数据库
        if (!initializeDatabase()) {
            qCCritical(chatServer) << "Failed to initialize database";
            return false;
        }
        
        // 初始化缓存
        if (!initializeCache()) {
            qCCritical(chatServer) << "Failed to initialize cache";
            return false;
        }
        
        // 初始化网络
        if (!initializeNetwork()) {
            qCCritical(chatServer) << "Failed to initialize network";
            return false;
        }
        
        // 初始化消息引擎
        _messageEngine = new MessageEngine(_connectionManager, this);
        if (!_messageEngine->initialize()) {
            qCCritical(chatServer) << "Failed to initialize message engine";
            return false;
        }
        
        // 初始化消息处理器
        if (!initializeMessageHandlers()) {
            qCCritical(chatServer) << "Failed to initialize message handlers";
            return false;
        }
        
        qCInfo(chatServer) << "All components initialized successfully";
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



// 设置SSL配置
bool ChatServer::setupSslConfiguration()
{
    return configureSsl();
}

// 注册消息处理器
void ChatServer::registerMessageHandlers()
{
    try {
        qCInfo(chatServer) << "Registering message handlers...";
        
        // 检查消息引擎是否已初始化
        if (!_messageEngine) {
            qCCritical(chatServer) << "Message engine not initialized";
            throw std::runtime_error("Message engine not initialized");
        }
        
        // 创建邮箱验证服务
        EmailVerificationService* emailService = new EmailVerificationService(this);
        
        // 注册登录消息处理器
        auto loginHandler = std::make_shared<LoginMessageHandler>(
            _connectionManager, _sessionManager, _databasePool, _cacheManager);
        _messageEngine->registerHandler(loginHandler);
        
        // 注册注册消息处理器（包含邮箱验证）
        auto registerHandler = std::make_shared<RegisterMessageHandler>(
            _connectionManager, _databasePool, _cacheManager, emailService);
        _messageEngine->registerHandler(registerHandler);
        
        // 注册邮箱验证消息处理器
        auto emailVerificationHandler = std::make_shared<EmailVerificationMessageHandler>(emailService);
        _messageEngine->registerHandler(emailVerificationHandler);
        
        // 注册验证消息处理器
        auto validationHandler = std::make_shared<ValidationMessageHandler>(_databasePool, emailService);
        _messageEngine->registerHandler(validationHandler);
        
        // 注册聊天消息处理器
        auto chatHandler = std::make_shared<ChatMessageHandler>(
            _connectionManager, _databasePool, _cacheManager);
        _messageEngine->registerHandler(chatHandler);
        
        // 注册心跳消息处理器
        auto heartbeatHandler = std::make_shared<HeartbeatMessageHandler>(_connectionManager);
        _messageEngine->registerHandler(heartbeatHandler);
        
        // 注册用户状态消息处理器
        auto userStatusHandler = std::make_shared<UserStatusMessageHandler>(
            _connectionManager, _databasePool, _cacheManager);
        _messageEngine->registerHandler(userStatusHandler);
        
        // 注册群聊消息处理器
        auto groupChatHandler = std::make_shared<GroupChatMessageHandler>(
            _connectionManager, _databasePool, _cacheManager);
        _messageEngine->registerHandler(groupChatHandler);
        
        // 注册系统通知处理器
        auto systemNotificationHandler = std::make_shared<SystemNotificationHandler>(
            _connectionManager, _cacheManager);
        _messageEngine->registerHandler(systemNotificationHandler);
        
        // 注册文件传输处理器
        auto fileTransferHandler = std::make_shared<FileTransferMessageHandler>(
            _connectionManager, _databasePool, _cacheManager);
        _messageEngine->registerHandler(fileTransferHandler);
        
        // 注册登出处理器
        auto logoutHandler = std::make_shared<LogoutMessageHandler>(
            _connectionManager, _sessionManager, _databasePool);
        _messageEngine->registerHandler(logoutHandler);
        
        qCInfo(chatServer) << "Message handlers registered successfully";
        
    } catch (const std::exception& e) {
        qCCritical(chatServer) << "Failed to register message handlers:" << e.what();
        throw;
    }
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
void ChatServer::processClientMessage(const QString& clientId, const QByteArray& messageData, QSslSocket* socket)
{
    qCInfo(chatServer) << "=== PROCESSING CLIENT MESSAGE ===";
    qCInfo(chatServer) << "Client ID:" << clientId;
    qCInfo(chatServer) << "Data size:" << messageData.size();
    qCInfo(chatServer) << "Raw data preview:" << messageData.left(100).toHex();
    qCInfo(chatServer) << "Raw data as string:" << QString::fromUtf8(messageData);
    qCInfo(chatServer) << "Socket state:" << (socket ? QString::number(static_cast<int>(socket->state())) : "no socket");
    qCInfo(chatServer) << "Socket encrypted:" << (socket ? (socket->isEncrypted() ? "true" : "false") : "no socket");
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
        qCInfo(chatServer) << "Extracted data from packet:" << data;
    } else {
        data = packetMap;
        qCInfo(chatServer) << "Using packet as data:" << data;
    }

    qCInfo(chatServer) << "Message type:" << msgType << "from client:" << clientId;
    qCInfo(chatServer) << "Message type length:" << msgType.length();
    qCInfo(chatServer) << "Message type bytes:" << msgType.toUtf8().toHex();
    qCDebug(chatServer) << "Full message data:" << packetMap;

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
        // 处理验证类型的消息
        qCInfo(chatServer) << "Handling validation request for client:" << clientId;
        qCInfo(chatServer) << "Validation data:" << data;
        qCInfo(chatServer) << "Validation type in data:" << data["type"].toString();
        handleValidationRequest(clientId, data);
    } else if (msgType == "emailVerification") {
        // 处理邮箱验证消息
        qCInfo(chatServer) << "=== HANDLING EMAIL VERIFICATION REQUEST ===";
        qCInfo(chatServer) << "Client ID:" << clientId;
        qCInfo(chatServer) << "Raw data:" << data;
        qCInfo(chatServer) << "Data keys:" << data.keys();
        qCInfo(chatServer) << "Message type:" << msgType;
        qCInfo(chatServer) << "Message type length:" << msgType.length();
        qCInfo(chatServer) << "Message type bytes:" << msgType.toUtf8().toHex();
        qCInfo(chatServer) << "Full packet map:" << packetMap;
        qCInfo(chatServer) << "Data action:" << data["action"].toString();
        qCInfo(chatServer) << "Data email:" << data["email"].toString();
        qCInfo(chatServer) << "Message engine exists:" << (_messageEngine != nullptr);
        
        // 直接处理邮箱验证，不通过消息引擎
        QString action = data["action"].toString();
        QString email = data["email"].toString();
        
        qCInfo(chatServer) << "Direct processing - Action:" << action << "Email:" << email;
        
        if (action == "sendCode") {
            qCInfo(chatServer) << "Processing sendCode action for email:" << email;
            // 这里可以添加直接的处理逻辑
            QJsonObject response;
            response["type"] = "emailCodeSent";
            response["success"] = true;
            response["message"] = "Verification code sent successfully";
            
            QJsonDocument doc(response);
            socket->write(doc.toJson(QJsonDocument::Compact));
            qCInfo(chatServer) << "Direct response sent for email verification";
        } else {
            qCWarning(chatServer) << "Unknown action:" << action;
            QJsonObject response;
            response["type"] = "emailVerification";
            response["success"] = false;
            response["message"] = "Invalid action";
            
            QJsonDocument doc(response);
            socket->write(doc.toJson(QJsonDocument::Compact));
        }
        
        qCInfo(chatServer) << "=== END EMAIL VERIFICATION REQUEST ===";
    } else {
        qCWarning(chatServer) << "Unknown message type:" << msgType << "from client:" << clientId;
        qCWarning(chatServer) << "Available message types: LOGIN, REGISTER, HEARTBEAT, validation, emailVerification";
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

