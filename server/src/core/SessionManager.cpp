#include "SessionManager.h"
#include <QTimer>
#include <QUuid>
#include <QLoggingCategory>
#include <QMutexLocker>

Q_LOGGING_CATEGORY(sessionManager, "qkchat.server.sessionmanager")

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
    , _cleanupTimer(nullptr)
{
    setupCleanupTimer();
    qCInfo(sessionManager) << "SessionManager initialized";
}

SessionManager::~SessionManager()
{
    if (_cleanupTimer) {
        _cleanupTimer->stop();
        delete _cleanupTimer;
        _cleanupTimer = nullptr;
    }
}

QString SessionManager::createSession(qint64 userId, const QString &ipAddress, int expirationHours)
{
    return createSession(userId, "", ipAddress, expirationHours);
}

QString SessionManager::createSession(qint64 userId, const QString &deviceInfo, const QString &ipAddress, int expirationHours)
{
    QMutexLocker locker(&_mutex);
    
    QString sessionToken = generateSessionToken();
    QDateTime now = QDateTime::currentDateTime();
    QDateTime expiresAt = now.addSecs(expirationHours * 3600);
    
    SessionInfo session;
    session.userId = userId;
    session.deviceInfo = deviceInfo;
    session.ipAddress = ipAddress;
    session.createdAt = now;
    session.lastActive = now;
    session.expiresAt = expiresAt;
    session.isValid = true;
    
    _sessions[sessionToken] = session;
    
    // 添加到用户会话映射
    if (!_userSessions.contains(userId)) {
        _userSessions[userId] = QStringList();
    }
    _userSessions[userId].append(sessionToken);
    
    qCInfo(sessionManager) << "Session created for user" << userId << "token:" << sessionToken;
    emit sessionCreated(userId, sessionToken);
    
    return sessionToken;
}

bool SessionManager::validateSession(const QString &sessionToken, qint64 &userId)
{
    QMutexLocker locker(&_mutex);
    
    if (!_sessions.contains(sessionToken)) {
        return false;
    }
    
    SessionInfo &session = _sessions[sessionToken];
    
    if (!session.isValid || isSessionExpired(session)) {
        // 会话已过期，移除它
        removeSession(sessionToken);
        return false;
    }
    
    userId = session.userId;
    session.lastActive = QDateTime::currentDateTime();
    return true;
}

bool SessionManager::removeSession(const QString &sessionToken)
{
    QMutexLocker locker(&_mutex);
    
    if (!_sessions.contains(sessionToken)) {
        return false;
    }
    
    SessionInfo session = _sessions[sessionToken];
    qint64 userId = session.userId;
    
    // 从会话映射中移除
    _sessions.remove(sessionToken);
    
    // 从用户会话映射中移除
    if (_userSessions.contains(userId)) {
        _userSessions[userId].removeOne(sessionToken);
        if (_userSessions[userId].isEmpty()) {
            _userSessions.remove(userId);
        }
    }
    
    qCInfo(sessionManager) << "Session removed:" << sessionToken;
    emit sessionRemoved(sessionToken);
    
    return true;
}

bool SessionManager::removeUserSessions(qint64 userId)
{
    QMutexLocker locker(&_mutex);
    
    if (!_userSessions.contains(userId)) {
        return false;
    }
    
    QStringList sessionTokens = _userSessions[userId];
    bool removed = false;
    
    for (const QString &sessionToken : sessionTokens) {
        if (_sessions.contains(sessionToken)) {
            _sessions.remove(sessionToken);
            removed = true;
        }
    }
    
    _userSessions.remove(userId);
    
    if (removed) {
        qCInfo(sessionManager) << "All sessions removed for user" << userId;
    }
    
    return removed;
}

void SessionManager::cleanExpiredSessions()
{
    QMutexLocker locker(&_mutex);
    
    QStringList expiredTokens;
    
    for (auto it = _sessions.begin(); it != _sessions.end(); ++it) {
        if (isSessionExpired(it.value())) {
            expiredTokens.append(it.key());
        }
    }
    
    for (const QString &sessionToken : expiredTokens) {
        SessionInfo session = _sessions[sessionToken];
        qint64 userId = session.userId;
        
        _sessions.remove(sessionToken);
        
        // 从用户会话映射中移除
        if (_userSessions.contains(userId)) {
            _userSessions[userId].removeOne(sessionToken);
            if (_userSessions[userId].isEmpty()) {
                _userSessions.remove(userId);
            }
        }
        
        emit sessionExpired(sessionToken);
    }
    
    if (!expiredTokens.isEmpty()) {
        qCInfo(sessionManager) << "Cleaned" << expiredTokens.size() << "expired sessions";
    }
}

qint64 SessionManager::getUserIdBySession(const QString &sessionToken)
{
    QMutexLocker locker(&_mutex);
    
    if (!_sessions.contains(sessionToken)) {
        return -1;
    }
    
    SessionInfo &session = _sessions[sessionToken];
    
    if (!session.isValid || isSessionExpired(session)) {
        return -1;
    }
    
    return session.userId;
}

QString SessionManager::getDeviceInfo(const QString &sessionToken)
{
    QMutexLocker locker(&_mutex);
    
    if (!_sessions.contains(sessionToken)) {
        return QString();
    }
    
    return _sessions[sessionToken].deviceInfo;
}

bool SessionManager::updateSessionLastActive(const QString &sessionToken)
{
    QMutexLocker locker(&_mutex);
    
    if (!_sessions.contains(sessionToken)) {
        return false;
    }
    
    _sessions[sessionToken].lastActive = QDateTime::currentDateTime();
    return true;
}

QString SessionManager::getIpAddress(const QString &sessionToken)
{
    QMutexLocker locker(&_mutex);
    
    if (!_sessions.contains(sessionToken)) {
        return QString();
    }
    
    return _sessions[sessionToken].ipAddress;
}

QDateTime SessionManager::getSessionExpiry(const QString &sessionToken)
{
    QMutexLocker locker(&_mutex);
    
    if (!_sessions.contains(sessionToken)) {
        return QDateTime();
    }
    
    return _sessions[sessionToken].expiresAt;
}

int SessionManager::getActiveSessionCount() const
{
    QMutexLocker locker(&_mutex);
    
    int count = 0;
    for (const SessionInfo &session : _sessions.values()) {
        if (session.isValid && !isSessionExpired(session)) {
            count++;
        }
    }
    
    return count;
}

int SessionManager::getUserSessionCount(qint64 userId) const
{
    QMutexLocker locker(&_mutex);
    
    if (!_userSessions.contains(userId)) {
        return 0;
    }
    
    int count = 0;
    const QStringList &sessionTokens = _userSessions[userId];
    
    for (const QString &sessionToken : sessionTokens) {
        if (_sessions.contains(sessionToken)) {
            const SessionInfo &session = _sessions[sessionToken];
            if (session.isValid && !isSessionExpired(session)) {
                count++;
            }
        }
    }
    
    return count;
}

void SessionManager::setupCleanupTimer()
{
    _cleanupTimer = new QTimer(this);
    connect(_cleanupTimer, &QTimer::timeout, this, &SessionManager::cleanExpiredSessions);
    _cleanupTimer->start(CLEANUP_INTERVAL);
}

QString SessionManager::generateSessionToken() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

bool SessionManager::isSessionExpired(const SessionInfo &session) const
{
    return QDateTime::currentDateTime() > session.expiresAt;
}