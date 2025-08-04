#include "ErrorHandler.h"
#include "../utils/LogManager.h"
#include "SSLConfigManager.h"
#include <QDateTime>

Q_LOGGING_CATEGORY(errorHandler, "qkchat.client.errorhandler")

ErrorHandler::ErrorHandler(QObject *parent)
    : QObject(parent)
    , _maxRetryAttempts(DEFAULT_MAX_RETRY_ATTEMPTS)
    , _retryDelay(DEFAULT_RETRY_DELAY)
    , _developmentMode(true)
{
    _retryTimer = new QTimer(this);
    _retryTimer->setSingleShot(true);
    connect(_retryTimer, &QTimer::timeout, this, &ErrorHandler::onRetryTimerTimeout);
    
    qCInfo(errorHandler) << "ErrorHandler initialized";
}

ErrorHandler::~ErrorHandler()
{
    clearErrorHistory();
}

void ErrorHandler::handleNetworkError(QAbstractSocket::SocketError error, const QString &details)
{
    ErrorInfo errorInfo = analyzeNetworkError(error);
    errorInfo.details = details;
    
    logError(errorInfo);
    addToHistory(errorInfo);
    
    emit errorOccurred(errorInfo);
    
    if (errorInfo.severity == Critical) {
        emit criticalErrorOccurred(errorInfo);
    }
    
    if (canRecover(errorInfo)) {
        executeRecoveryStrategy(errorInfo);
    }
}

void ErrorHandler::handleSslError(const QList<QSslError> &errors)
{
    for (const QSslError &sslError : errors) {
        ErrorInfo errorInfo = analyzeSslError(sslError);
        
        logError(errorInfo);
        addToHistory(errorInfo);
        
        emit errorOccurred(errorInfo);
        
        if (errorInfo.severity == Critical) {
            emit criticalErrorOccurred(errorInfo);
        }
        
        // SSL错误的特殊处理
        if (_developmentMode && SSLConfigManager::instance()->shouldIgnoreSslErrors(errors)) {
            qCInfo(errorHandler) << "SSL error ignored in development mode:" << errorInfo.message;
            LogManager::instance()->writeSslLog("ERROR_IGNORED_DEV", errorInfo.message);
        } else if (canRecover(errorInfo)) {
            executeRecoveryStrategy(errorInfo);
        }
    }
}

void ErrorHandler::handleProtocolError(const QString &message, const QString &details)
{
    ErrorInfo errorInfo;
    errorInfo.type = ProtocolError;
    errorInfo.severity = Warning;
    errorInfo.message = message;
    errorInfo.details = details;
    errorInfo.solution = "检查协议版本兼容性，确保客户端和服务端使用相同的协议版本";
    errorInfo.strategy = Retry;
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.errorCode = 0;
    
    logError(errorInfo);
    addToHistory(errorInfo);
    
    emit errorOccurred(errorInfo);
    
    if (canRecover(errorInfo)) {
        executeRecoveryStrategy(errorInfo);
    }
}

void ErrorHandler::handleAuthenticationError(const QString &message, const QString &details)
{
    ErrorInfo errorInfo;
    errorInfo.type = AuthenticationError;
    errorInfo.severity = Critical;
    errorInfo.message = message;
    errorInfo.details = details;
    errorInfo.solution = "检查用户名和密码是否正确，或联系管理员";
    errorInfo.strategy = UserIntervention;
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.errorCode = 0;
    
    logError(errorInfo);
    addToHistory(errorInfo);
    
    emit errorOccurred(errorInfo);
    emit criticalErrorOccurred(errorInfo);
    emit userInterventionRequired(errorInfo);
}

void ErrorHandler::handleTimeoutError(const QString &operation, const QString &details)
{
    ErrorInfo errorInfo;
    errorInfo.type = TimeoutError;
    errorInfo.severity = Warning;
    errorInfo.message = QString("操作超时: %1").arg(operation);
    errorInfo.details = details;
    errorInfo.solution = "检查网络连接，增加超时时间，或重试操作";
    errorInfo.strategy = Retry;
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.errorCode = 0;
    
    logError(errorInfo);
    addToHistory(errorInfo);
    
    emit errorOccurred(errorInfo);
    
    if (canRecover(errorInfo)) {
        executeRecoveryStrategy(errorInfo);
    }
}

void ErrorHandler::handleCustomError(ErrorType type, const QString &message, const QString &details)
{
    ErrorInfo errorInfo;
    errorInfo.type = type;
    errorInfo.severity = Warning;
    errorInfo.message = message;
    errorInfo.details = details;
    errorInfo.solution = getErrorSolution(type, 0);
    errorInfo.strategy = getRecoveryStrategy(type, Warning);
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.errorCode = 0;
    
    logError(errorInfo);
    addToHistory(errorInfo);
    
    emit errorOccurred(errorInfo);
    
    if (canRecover(errorInfo)) {
        executeRecoveryStrategy(errorInfo);
    }
}

ErrorHandler::ErrorInfo ErrorHandler::analyzeNetworkError(QAbstractSocket::SocketError error) const
{
    ErrorInfo errorInfo;
    errorInfo.type = NetworkError;
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.errorCode = static_cast<int>(error);
    
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        errorInfo.severity = Critical;
        errorInfo.message = "连接被拒绝";
        errorInfo.solution = "检查服务器是否运行，端口是否正确，防火墙设置";
        errorInfo.strategy = Retry;
        break;
        
    case QAbstractSocket::RemoteHostClosedError:
        errorInfo.severity = Warning;
        errorInfo.message = "远程主机关闭连接";
        errorInfo.solution = "检查网络连接，服务器可能重启或维护中";
        errorInfo.strategy = Reconnect;
        break;
        
    case QAbstractSocket::HostNotFoundError:
        errorInfo.severity = Critical;
        errorInfo.message = "找不到主机";
        errorInfo.solution = "检查主机名或IP地址是否正确，DNS设置";
        errorInfo.strategy = UserIntervention;
        break;
        
    case QAbstractSocket::SocketTimeoutError:
        errorInfo.severity = Warning;
        errorInfo.message = "连接超时";
        errorInfo.solution = "检查网络连接，增加超时时间";
        errorInfo.strategy = Retry;
        break;
        
    case QAbstractSocket::NetworkError:
        errorInfo.severity = Warning;
        errorInfo.message = "网络错误";
        errorInfo.solution = "检查网络连接状态";
        errorInfo.strategy = Retry;
        break;
        
    case QAbstractSocket::SslHandshakeFailedError:
        errorInfo.severity = Critical;
        errorInfo.message = "SSL握手失败";
        errorInfo.solution = "检查SSL证书配置，时间同步";
        errorInfo.strategy = _developmentMode ? Fallback : UserIntervention;
        break;
        
    default:
        errorInfo.severity = Warning;
        errorInfo.message = "未知网络错误";
        errorInfo.solution = "检查网络连接和配置";
        errorInfo.strategy = Retry;
        break;
    }
    
    return errorInfo;
}

ErrorHandler::ErrorInfo ErrorHandler::analyzeSslError(const QSslError &error) const
{
    ErrorInfo errorInfo;
    errorInfo.type = SslError;
    errorInfo.timestamp = QDateTime::currentDateTime();
    errorInfo.errorCode = static_cast<int>(error.error());
    errorInfo.message = error.errorString();
    
    // 使用SSL配置管理器分析错误
    SSLConfigManager* sslManager = SSLConfigManager::instance();
    errorInfo.details = sslManager->analyzeSslError(error);
    errorInfo.solution = sslManager->getSslErrorSolution(error);
    
    // 根据开发模式和错误类型确定严重程度和策略
    if (_developmentMode && sslManager->shouldIgnoreSslErrors({error})) {
        errorInfo.severity = Info;
        errorInfo.strategy = NoRecovery; // 在开发模式下忽略
    } else {
        switch (error.error()) {
        case QSslError::SelfSignedCertificate:
        case QSslError::SelfSignedCertificateInChain:
        case QSslError::CertificateUntrusted:
            errorInfo.severity = _developmentMode ? Warning : Critical;
            errorInfo.strategy = _developmentMode ? Fallback : UserIntervention;
            break;
            
        case QSslError::HostNameMismatch:
            errorInfo.severity = Warning;
            errorInfo.strategy = _developmentMode ? Fallback : UserIntervention;
            break;
            
        case QSslError::CertificateExpired:
        case QSslError::CertificateNotYetValid:
            errorInfo.severity = Critical;
            errorInfo.strategy = UserIntervention;
            break;
            
        default:
            errorInfo.severity = Warning;
            errorInfo.strategy = Retry;
            break;
        }
    }
    
    return errorInfo;
}

QString ErrorHandler::getErrorSolution(ErrorType type, int errorCode) const
{
    Q_UNUSED(errorCode)
    
    switch (type) {
    case NetworkError:
        return "检查网络连接，确认服务器地址和端口正确";
    case SslError:
        return "检查SSL证书配置，确保证书有效且受信任";
    case ProtocolError:
        return "检查协议版本兼容性，更新客户端或服务端";
    case AuthenticationError:
        return "检查用户名和密码，确认账户状态正常";
    case TimeoutError:
        return "检查网络延迟，增加超时时间或重试";
    default:
        return "请联系技术支持";
    }
}

ErrorHandler::RecoveryStrategy ErrorHandler::getRecoveryStrategy(ErrorType type, ErrorSeverity severity) const
{
    if (severity == Critical) {
        if (type == AuthenticationError) {
            return UserIntervention;
        } else {
            return Reconnect;
        }
    } else if (severity == Warning) {
        return Retry;
    } else {
        return NoRecovery;
    }
}

bool ErrorHandler::canRecover(const ErrorInfo &error) const
{
    return error.strategy != NoRecovery && error.strategy != UserIntervention;
}

void ErrorHandler::executeRecoveryStrategy(const ErrorInfo &error)
{
    qCInfo(errorHandler) << "Executing recovery strategy:" << getRecoveryStrategyString(error.strategy);
    LogManager::instance()->writeErrorLog(
        QString("Executing recovery: %1 for error: %2")
        .arg(getRecoveryStrategyString(error.strategy), error.message),
        "ErrorHandler"
    );
    
    emit recoveryAttempted(error, error.strategy);
    
    switch (error.strategy) {
    case Retry:
        // 延迟重试
        _retryTimer->start(_retryDelay);
        break;
        
    case Reconnect:
        // 触发重连
        emit recoverySucceeded(error);
        break;
        
    case Fallback:
        // 降级处理（例如忽略SSL错误）
        emit recoverySucceeded(error);
        break;
        
    default:
        emit recoveryFailed(error);
        break;
    }
}

int ErrorHandler::getErrorCount(ErrorType type) const
{
    return _errorCounts.value(type, 0);
}

QList<ErrorHandler::ErrorInfo> ErrorHandler::getRecentErrors(int maxCount) const
{
    if (maxCount <= 0 || maxCount >= _errorHistory.size()) {
        return _errorHistory;
    }
    
    return _errorHistory.mid(_errorHistory.size() - maxCount);
}

void ErrorHandler::clearErrorHistory()
{
    _errorHistory.clear();
    _errorCounts.clear();
}

void ErrorHandler::setMaxRetryAttempts(int maxAttempts)
{
    _maxRetryAttempts = maxAttempts;
}

void ErrorHandler::setRetryDelay(int delayMs)
{
    _retryDelay = delayMs;
}

void ErrorHandler::setDevelopmentMode(bool enabled)
{
    _developmentMode = enabled;
}

void ErrorHandler::onRetryTimerTimeout()
{
    qCInfo(errorHandler) << "Retry timer timeout, attempting recovery";
    // 这里可以触发具体的重试逻辑
}

QString ErrorHandler::getErrorTypeString(ErrorType type) const
{
    switch (type) {
    case NetworkError: return "Network";
    case SslError: return "SSL";
    case ProtocolError: return "Protocol";
    case AuthenticationError: return "Authentication";
    case TimeoutError: return "Timeout";
    default: return "Unknown";
    }
}

QString ErrorHandler::getErrorSeverityString(ErrorSeverity severity) const
{
    switch (severity) {
    case Critical: return "Critical";
    case Warning: return "Warning";
    case Info: return "Info";
    default: return "Unknown";
    }
}

QString ErrorHandler::getRecoveryStrategyString(RecoveryStrategy strategy) const
{
    switch (strategy) {
    case NoRecovery: return "NoRecovery";
    case Retry: return "Retry";
    case Reconnect: return "Reconnect";
    case Fallback: return "Fallback";
    case UserIntervention: return "UserIntervention";
    default: return "Unknown";
    }
}

void ErrorHandler::logError(const ErrorInfo &error)
{
    QString logMessage = QString("[%1][%2] %3")
        .arg(getErrorTypeString(error.type))
        .arg(getErrorSeverityString(error.severity))
        .arg(error.message);
    
    if (!error.details.isEmpty()) {
        logMessage += QString(" - Details: %1").arg(error.details);
    }
    
    if (!error.solution.isEmpty()) {
        logMessage += QString(" - Solution: %1").arg(error.solution);
    }
    
    LogManager::instance()->writeErrorLog(logMessage, "ErrorHandler");
}

void ErrorHandler::addToHistory(const ErrorInfo &error)
{
    _errorHistory.append(error);
    _errorCounts[error.type]++;
    
    // 限制历史记录大小
    while (_errorHistory.size() > MAX_ERROR_HISTORY) {
        ErrorInfo removed = _errorHistory.takeFirst();
        _errorCounts[removed.type]--;
    }
}
