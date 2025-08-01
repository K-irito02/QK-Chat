#include "FileTransferManager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <QStandardPaths>
#include <QDir>
#include <QMimeDatabase>
#include <QMimeType>
#include <QNetworkRequest>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QUuid>
#include <QLoggingCategory>
#include <QDebug>

Q_LOGGING_CATEGORY(fileTransfer, "qkchat.client.filetransfer")

// 静态常量定义
const QStringList FileTransferManager::IMAGE_EXTENSIONS = {
    "jpg", "jpeg", "png", "gif", "bmp", "tiff", "webp", "svg"
};

const QStringList FileTransferManager::VIDEO_EXTENSIONS = {
    "mp4", "avi", "mkv", "mov", "wmv", "flv", "webm", "m4v"
};

const QStringList FileTransferManager::AUDIO_EXTENSIONS = {
    "mp3", "wav", "flac", "aac", "ogg", "wma", "m4a"
};

FileTransferManager::FileTransferManager(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_maxConcurrentTransfers(MAX_CONCURRENT_TRANSFERS)
    , m_chunkSize(DEFAULT_CHUNK_SIZE)
    , m_uploadUrl(QUrl("https://localhost:8889/api/upload"))
    , m_downloadBaseUrl(QUrl("https://localhost:8889/api/download"))
{
    // 设置网络管理器
    m_networkManager->setTransferTimeout(30000); // 30秒超时
    
    qCInfo(fileTransfer) << "FileTransferManager created";
}

FileTransferManager::~FileTransferManager()
{
    // 取消所有活跃的传输
    QMutexLocker locker(&m_mutex);
    for (const QString &taskId : m_activeTransfers) {
        TransferTask &task = m_transferTasks[taskId];
        task.status = Cancelled;
    }
    locker.unlock();
    
    qCInfo(fileTransfer) << "FileTransferManager destroyed";
}

// 文件上传
QString FileTransferManager::uploadFile(const QString &filePath, qint64 receiverId, qint64 groupId, const QString &messageId)
{
    if (!validateFile(filePath)) {
        qCWarning(fileTransfer) << "Invalid file:" << filePath;
        return QString();
    }
    
    QString taskId = generateTaskId();
    
    TransferTask task;
    task.taskId = taskId;
    task.type = Upload;
    task.status = Pending;
    task.localFilePath = filePath;
    task.remoteUrl = m_uploadUrl;
    task.receiverId = receiverId;
    task.groupId = groupId;
    task.messageId = messageId;
    task.startTime = QDateTime::currentDateTime();
    task.chunkSize = m_chunkSize;
    task.progress = 0;
    
    QFileInfo fileInfo(filePath);
    task.fileSize = fileInfo.size();
    task.totalChunks = (task.fileSize + m_chunkSize - 1) / m_chunkSize;
    task.uploadedChunks = 0;
    
    QMutexLocker locker(&m_mutex);
    m_transferTasks[taskId] = task;
    m_transferQueue.enqueue(taskId);
    locker.unlock();
    
    emit queueChanged();
    
    // 尝试开始传输
    processQueue();
    
    qCInfo(fileTransfer) << "File upload queued:" << taskId << filePath;
    return taskId;
}

QString FileTransferManager::uploadAvatar(const QString &filePath)
{
    return uploadFile(filePath, 0, 0, "avatar");
}

// 文件下载
QString FileTransferManager::downloadFile(const QUrl &remoteUrl, const QString &savePath)
{
    QString taskId = generateTaskId();
    QString actualSavePath = savePath;
    
    if (actualSavePath.isEmpty()) {
        actualSavePath = getSaveDirectory();
        QFileInfo urlInfo(remoteUrl.path());
        actualSavePath += "/" + urlInfo.fileName();
    }
    
    TransferTask task;
    task.taskId = taskId;
    task.type = Download;
    task.status = Pending;
    task.localFilePath = actualSavePath;
    task.remoteUrl = remoteUrl;
    task.startTime = QDateTime::currentDateTime();
    task.progress = 0;
    
    QMutexLocker locker(&m_mutex);
    m_transferTasks[taskId] = task;
    m_transferQueue.enqueue(taskId);
    locker.unlock();
    
    emit queueChanged();
    
    // 尝试开始传输
    processQueue();
    
    qCInfo(fileTransfer) << "File download queued:" << taskId << remoteUrl.toString();
    return taskId;
}

// 传输控制
void FileTransferManager::pauseTransfer(const QString &taskId)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_transferTasks.contains(taskId)) {
        return;
    }
    
    TransferTask &task = m_transferTasks[taskId];
    if (task.status == Running) {
        task.status = Paused;
        
        // 从活跃传输中移除
        m_activeTransfers.removeOne(taskId);
        
        emit transferPaused(taskId);
        qCInfo(fileTransfer) << "Transfer paused:" << taskId;
        
        // 处理队列中的下一个任务
        QTimer::singleShot(0, this, &FileTransferManager::processQueue);
    }
}

void FileTransferManager::resumeTransfer(const QString &taskId)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_transferTasks.contains(taskId)) {
        return;
    }
    
    TransferTask &task = m_transferTasks[taskId];
    if (task.status == Paused) {
        task.status = Pending;
        m_transferQueue.enqueue(taskId);
        
        emit transferResumed(taskId);
        qCInfo(fileTransfer) << "Transfer resumed:" << taskId;
        
        // 处理队列
        QTimer::singleShot(0, this, &FileTransferManager::processQueue);
    }
}

void FileTransferManager::cancelTransfer(const QString &taskId)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_transferTasks.contains(taskId)) {
        return;
    }
    
    TransferTask &task = m_transferTasks[taskId];
    task.status = Cancelled;
    task.endTime = QDateTime::currentDateTime();
    
    // 从活跃传输和队列中移除
    m_activeTransfers.removeOne(taskId);
    
    // 从队列中移除（如果还在队列中）
    QQueue<QString> newQueue;
    while (!m_transferQueue.isEmpty()) {
        QString id = m_transferQueue.dequeue();
        if (id != taskId) {
            newQueue.enqueue(id);
        }
    }
    m_transferQueue = newQueue;
    
    emit transferCancelled(taskId);
    qCInfo(fileTransfer) << "Transfer cancelled:" << taskId;
    
    // 清理任务
    cleanupTask(taskId);
    
    // 处理队列中的下一个任务
    QTimer::singleShot(0, this, &FileTransferManager::processQueue);
}

void FileTransferManager::retryTransfer(const QString &taskId)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_transferTasks.contains(taskId)) {
        return;
    }
    
    TransferTask &task = m_transferTasks[taskId];
    if (task.status == Failed) {
        task.status = Pending;
        task.progress = 0;
        task.transferredSize = 0;
        task.errorMessage.clear();
        task.startTime = QDateTime::currentDateTime();
        task.endTime = QDateTime();
        
        // 重新加入队列
        m_transferQueue.enqueue(taskId);
        
        qCInfo(fileTransfer) << "Transfer retry queued:" << taskId;
        
        // 处理队列
        QTimer::singleShot(0, this, &FileTransferManager::processQueue);
    }
}

// 传输信息查询
FileTransferManager::TransferTask FileTransferManager::getTransferTask(const QString &taskId) const
{
    QMutexLocker locker(&m_mutex);
    return m_transferTasks.value(taskId, TransferTask{});
}

QList<FileTransferManager::TransferTask> FileTransferManager::getAllTransferTasks() const
{
    QMutexLocker locker(&m_mutex);
    return m_transferTasks.values();
}

QList<FileTransferManager::TransferTask> FileTransferManager::getActiveTransferTasks() const
{
    QMutexLocker locker(&m_mutex);
    QList<TransferTask> activeTasks;
    
    for (const QString &taskId : m_activeTransfers) {
        if (m_transferTasks.contains(taskId)) {
            activeTasks.append(m_transferTasks[taskId]);
        }
    }
    
    return activeTasks;
}

// 设置
void FileTransferManager::setMaxConcurrentTransfers(int maxCount)
{
    m_maxConcurrentTransfers = qMax(1, maxCount);
}

void FileTransferManager::setChunkSize(int chunkSize)
{
    m_chunkSize = qMax(64 * 1024, chunkSize); // 最小64KB
}

void FileTransferManager::setUploadUrl(const QUrl &url)
{
    m_uploadUrl = url;
}

void FileTransferManager::setDownloadBaseUrl(const QUrl &url)
{
    m_downloadBaseUrl = url;
}

// 工具函数
QString FileTransferManager::formatFileSize(qint64 bytes)
{
    const qint64 kb = 1024;
    const qint64 mb = kb * 1024;
    const qint64 gb = mb * 1024;
    
    if (bytes >= gb) {
        return QString::number(bytes / (double)gb, 'f', 2) + " GB";
    } else if (bytes >= mb) {
        return QString::number(bytes / (double)mb, 'f', 2) + " MB";
    } else if (bytes >= kb) {
        return QString::number(bytes / (double)kb, 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

QString FileTransferManager::getFileExtension(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    return fileInfo.suffix().toLower();
}

bool FileTransferManager::isImageFile(const QString &filePath)
{
    QString extension = getFileExtension(filePath);
    return IMAGE_EXTENSIONS.contains(extension);
}

bool FileTransferManager::isVideoFile(const QString &filePath)
{
    QString extension = getFileExtension(filePath);
    return VIDEO_EXTENSIONS.contains(extension);
}

bool FileTransferManager::isAudioFile(const QString &filePath)
{
    QString extension = getFileExtension(filePath);
    return AUDIO_EXTENSIONS.contains(extension);
}

// 私有槽函数
void FileTransferManager::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    QString taskId = m_replyToTaskMap.value(reply);
    
    if (!taskId.isEmpty() && m_transferTasks.contains(taskId)) {
        updateTaskProgress(taskId, bytesSent, bytesTotal);
    }
}

void FileTransferManager::onUploadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    reply->deleteLater();
    
    QMutexLocker locker(&m_mutex);
    QString taskId = m_replyToTaskMap.value(reply);
    m_replyToTaskMap.remove(reply);
    
    if (!taskId.isEmpty() && m_transferTasks.contains(taskId)) {
        TransferTask &task = m_transferTasks[taskId];
        
        if (reply->error() == QNetworkReply::NoError) {
            // 解析响应
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject obj = doc.object();
            
            if (obj["success"].toBool()) {
                QString resultUrl = obj["url"].toString();
                completeTask(taskId, resultUrl);
            } else {
                QString error = obj["message"].toString();
                failTask(taskId, error.isEmpty() ? "Upload failed" : error);
            }
        } else {
            failTask(taskId, reply->errorString());
        }
    }
    
    locker.unlock();
    processQueue();
}

void FileTransferManager::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    QString taskId = m_replyToTaskMap.value(reply);
    
    if (!taskId.isEmpty() && m_transferTasks.contains(taskId)) {
        updateTaskProgress(taskId, bytesReceived, bytesTotal);
    }
}

void FileTransferManager::onDownloadFinished()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    reply->deleteLater();
    
    QMutexLocker locker(&m_mutex);
    QString taskId = m_replyToTaskMap.value(reply);
    m_replyToTaskMap.remove(reply);
    
    if (!taskId.isEmpty() && m_transferTasks.contains(taskId)) {
        TransferTask &task = m_transferTasks[taskId];
        
        if (reply->error() == QNetworkReply::NoError) {
            // 保存文件
            QByteArray data = reply->readAll();
            QFile file(task.localFilePath);
            
            QFileInfo fileInfo(task.localFilePath);
            QDir dir = fileInfo.dir();
            if (!dir.exists()) {
                dir.mkpath(".");
            }
            
            if (file.open(QIODevice::WriteOnly)) {
                file.write(data);
                file.close();
                completeTask(taskId, task.localFilePath);
            } else {
                failTask(taskId, "Failed to save file: " + file.errorString());
            }
        } else {
            failTask(taskId, reply->errorString());
        }
    }
    
    locker.unlock();
    processQueue();
}

void FileTransferManager::onNetworkError(QNetworkReply::NetworkError error)
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        return;
    }
    
    QMutexLocker locker(&m_mutex);
    QString taskId = m_replyToTaskMap.value(reply);
    
    if (!taskId.isEmpty() && m_transferTasks.contains(taskId)) {
        QString errorString = QString("Network error: %1").arg(reply->errorString());
        failTask(taskId, errorString);
    }
}

void FileTransferManager::processQueue()
{
    QMutexLocker locker(&m_mutex);
    
    // 检查是否有空闲槽位
    if (m_activeTransfers.size() >= m_maxConcurrentTransfers) {
        return;
    }
    
    // 从队列中取出待处理的任务
    if (m_transferQueue.isEmpty()) {
        return;
    }
    
    QString taskId = m_transferQueue.dequeue();
    if (!m_transferTasks.contains(taskId)) {
        // 递归处理下一个任务
        QTimer::singleShot(0, this, &FileTransferManager::processQueue);
        return;
    }
    
    TransferTask &task = m_transferTasks[taskId];
    if (task.status != Pending) {
        // 递归处理下一个任务
        QTimer::singleShot(0, this, &FileTransferManager::processQueue);
        return;
    }
    
    // 开始传输
    task.status = Running;
    m_activeTransfers.append(taskId);
    
    locker.unlock();
    
    emit transferStarted(taskId);
    
    if (task.type == Upload) {
        startUploadTask(taskId);
    } else {
        startDownloadTask(taskId);
    }
    
    // 处理队列中的下一个任务
    QTimer::singleShot(0, this, &FileTransferManager::processQueue);
}

// 私有方法
QString FileTransferManager::generateTaskId() const
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void FileTransferManager::startNextTransfer()
{
    processQueue();
}

void FileTransferManager::startUploadTask(const QString &taskId)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_transferTasks.contains(taskId)) {
        return;
    }
    
    const TransferTask &task = m_transferTasks[taskId];
    
    // 检查文件大小决定是否使用分块上传
    if (task.fileSize > m_chunkSize * 2) {
        locker.unlock();
        startChunkedUpload(taskId);
    } else {
        // 普通上传
        QFile file(task.localFilePath);
        if (!file.open(QIODevice::ReadOnly)) {
            locker.unlock();
            failTask(taskId, "Failed to open file: " + file.errorString());
            return;
        }
        
        QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
        
        QHttpPart filePart;
        filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
        
        QFileInfo fileInfo(task.localFilePath);
        filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                          QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(fileInfo.fileName())));
        
        filePart.setBodyDevice(&file);
        file.setParent(multiPart);
        multiPart->append(filePart);
        
        // 添加额外参数
        if (task.receiverId > 0) {
            QHttpPart receiverPart;
            receiverPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"receiverId\""));
            receiverPart.setBody(QString::number(task.receiverId).toUtf8());
            multiPart->append(receiverPart);
        }
        
        if (task.groupId > 0) {
            QHttpPart groupPart;
            groupPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"groupId\""));
            groupPart.setBody(QString::number(task.groupId).toUtf8());
            multiPart->append(groupPart);
        }
        
        if (!task.messageId.isEmpty()) {
            QHttpPart messagePart;
            messagePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"messageId\""));
            messagePart.setBody(task.messageId.toUtf8());
            multiPart->append(messagePart);
        }
        
        QNetworkRequest request = createUploadRequest(task);
        locker.unlock();
        
        QNetworkReply *reply = m_networkManager->post(request, multiPart);
        multiPart->setParent(reply);
        
        // 连接信号
        connect(reply, &QNetworkReply::uploadProgress, this, &FileTransferManager::onUploadProgress);
        connect(reply, &QNetworkReply::finished, this, &FileTransferManager::onUploadFinished);
        connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
                this, &FileTransferManager::onNetworkError);
        
        locker.relock();
        m_replyToTaskMap[reply] = taskId;
    }
}

void FileTransferManager::startDownloadTask(const QString &taskId)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_transferTasks.contains(taskId)) {
        return;
    }
    
    const TransferTask &task = m_transferTasks[taskId];
    QNetworkRequest request = createDownloadRequest(task);
    locker.unlock();
    
    QNetworkReply *reply = m_networkManager->get(request);
    
    // 连接信号
    connect(reply, &QNetworkReply::downloadProgress, this, &FileTransferManager::onDownloadProgress);
    connect(reply, &QNetworkReply::finished, this, &FileTransferManager::onDownloadFinished);
    connect(reply, QOverload<QNetworkReply::NetworkError>::of(&QNetworkReply::errorOccurred),
            this, &FileTransferManager::onNetworkError);
    
    locker.relock();
    m_replyToTaskMap[reply] = taskId;
}

void FileTransferManager::startChunkedUpload(const QString &taskId)
{
    // TODO: 实现分块上传
    qCInfo(fileTransfer) << "Chunked upload not implemented yet, using normal upload";
    startUploadTask(taskId);
}

void FileTransferManager::uploadNextChunk(const QString &taskId)
{
    // TODO: 实现下一个分块的上传
    Q_UNUSED(taskId)
}

void FileTransferManager::mergeChunks(const QString &taskId)
{
    // TODO: 实现分块合并
    Q_UNUSED(taskId)
}

void FileTransferManager::updateTaskProgress(const QString &taskId, qint64 transferred, qint64 total)
{
    if (!m_transferTasks.contains(taskId)) {
        return;
    }
    
    TransferTask &task = m_transferTasks[taskId];
    task.transferredSize = transferred;
    if (total > 0) {
        task.fileSize = total;
        task.progress = (transferred * 100) / total;
    }
    
    emit transferProgress(taskId, task.progress, transferred, total);
}

void FileTransferManager::completeTask(const QString &taskId, const QString &resultUrl)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_transferTasks.contains(taskId)) {
        return;
    }
    
    TransferTask &task = m_transferTasks[taskId];
    task.status = Completed;
    task.progress = 100;
    task.endTime = QDateTime::currentDateTime();
    
    m_activeTransfers.removeOne(taskId);
    
    emit transferCompleted(taskId, resultUrl);
    emit activeTransfersChanged(m_activeTransfers.size());
    
    qCInfo(fileTransfer) << "Transfer completed:" << taskId;
    
    // 清理任务（延迟一段时间后）
    QTimer::singleShot(5000, this, [this, taskId]() {
        cleanupTask(taskId);
    });
}

void FileTransferManager::failTask(const QString &taskId, const QString &error)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_transferTasks.contains(taskId)) {
        return;
    }
    
    TransferTask &task = m_transferTasks[taskId];
    task.status = Failed;
    task.errorMessage = error;
    task.endTime = QDateTime::currentDateTime();
    
    m_activeTransfers.removeOne(taskId);
    
    emit transferFailed(taskId, error);
    emit activeTransfersChanged(m_activeTransfers.size());
    
    qCWarning(fileTransfer) << "Transfer failed:" << taskId << error;
}

void FileTransferManager::cleanupTask(const QString &taskId)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_transferTasks.contains(taskId)) {
        TransferTask task = m_transferTasks[taskId];
        m_transferTasks.remove(taskId);
        
        // 清理网络请求映射
        for (auto it = m_replyToTaskMap.begin(); it != m_replyToTaskMap.end();) {
            if (it.value() == taskId) {
                it = m_replyToTaskMap.erase(it);
            } else {
                ++it;
            }
        }
        
        qCDebug(fileTransfer) << "Task cleaned up:" << taskId;
    }
}

bool FileTransferManager::validateFile(const QString &filePath) const
{
    QFileInfo fileInfo(filePath);
    
    // 检查文件是否存在
    if (!fileInfo.exists()) {
        return false;
    }
    
    // 检查文件大小
    if (fileInfo.size() > MAX_FILE_SIZE) {
        return false;
    }
    
    // 检查是否为目录
    if (fileInfo.isDir()) {
        return false;
    }
    
    return true;
}

QString FileTransferManager::getSaveDirectory() const
{
    QString downloadDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QDir dir(downloadDir + "/QKChat");
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    return dir.absolutePath();
}

QNetworkRequest FileTransferManager::createUploadRequest(const TransferTask &task) const
{
    QNetworkRequest request;
    request.setUrl(task.remoteUrl);
    request.setRawHeader("User-Agent", "QKChat Client 1.0");
    
    return request;
}

QNetworkRequest FileTransferManager::createDownloadRequest(const TransferTask &task) const
{
    QNetworkRequest request;
    request.setUrl(task.remoteUrl);
    request.setRawHeader("User-Agent", "QKChat Client 1.0");
    
    return request;
} 