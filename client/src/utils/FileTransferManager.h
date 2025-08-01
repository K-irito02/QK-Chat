#ifndef FILETRANSFERMANAGER_H
#define FILETRANSFERMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(fileTransfer)

/**
 * @brief 文件传输管理器
 * 
 * 负责处理文件的上传和下载功能，包括：
 * - 文件分块上传
 * - 断点续传
 * - 传输进度监控
 * - 传输队列管理
 * - 并发传输控制
 */
class FileTransferManager : public QObject
{
    Q_OBJECT
    
public:
    enum TransferType {
        Upload,
        Download
    };
    
    enum TransferStatus {
        Pending,    // 等待中
        Running,    // 传输中
        Paused,     // 已暂停
        Completed,  // 已完成
        Failed,     // 失败
        Cancelled   // 已取消
    };
    
    struct TransferTask {
        QString taskId;
        TransferType type;
        TransferStatus status;
        QString localFilePath;
        QUrl remoteUrl;
        qint64 fileSize;
        qint64 transferredSize;
        int progress;  // 0-100
        QDateTime startTime;
        QDateTime endTime;
        QString errorMessage;
        
        // 上传特定字段
        qint64 receiverId;    // 接收者ID（私聊）
        qint64 groupId;       // 群组ID（群聊）
        QString messageId;    // 关联的消息ID
        
        // 分块上传字段
        int chunkSize;
        int totalChunks;
        int uploadedChunks;
        QStringList uploadedChunkIds;
    };
    
    explicit FileTransferManager(QObject *parent = nullptr);
    ~FileTransferManager();
    
    // 文件上传
    QString uploadFile(const QString &filePath, qint64 receiverId = 0, qint64 groupId = 0, const QString &messageId = "");
    QString uploadAvatar(const QString &filePath);
    
    // 文件下载
    QString downloadFile(const QUrl &remoteUrl, const QString &savePath = "");
    
    // 传输控制
    void pauseTransfer(const QString &taskId);
    void resumeTransfer(const QString &taskId);
    void cancelTransfer(const QString &taskId);
    void retryTransfer(const QString &taskId);
    
    // 传输信息查询
    TransferTask getTransferTask(const QString &taskId) const;
    QList<TransferTask> getAllTransferTasks() const;
    QList<TransferTask> getActiveTransferTasks() const;
    
    // 设置
    void setMaxConcurrentTransfers(int maxCount);
    void setChunkSize(int chunkSize);
    void setUploadUrl(const QUrl &url);
    void setDownloadBaseUrl(const QUrl &url);
    
    // 工具函数
    static QString formatFileSize(qint64 bytes);
    static QString getFileExtension(const QString &filePath);
    static bool isImageFile(const QString &filePath);
    static bool isVideoFile(const QString &filePath);
    static bool isAudioFile(const QString &filePath);
    
signals:
    // 传输进度信号
    void transferStarted(const QString &taskId);
    void transferProgress(const QString &taskId, int progress, qint64 bytesTransferred, qint64 bytesTotal);
    void transferCompleted(const QString &taskId, const QString &resultUrl = "");
    void transferFailed(const QString &taskId, const QString &error);
    void transferPaused(const QString &taskId);
    void transferResumed(const QString &taskId);
    void transferCancelled(const QString &taskId);
    
    // 队列状态信号
    void queueChanged();
    void activeTransfersChanged(int activeCount);
    
private slots:
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void onUploadFinished();
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onNetworkError(QNetworkReply::NetworkError error);
    void processQueue();
    
private:
    // 内部方法
    QString generateTaskId() const;
    void startNextTransfer();
    void startUploadTask(const QString &taskId);
    void startDownloadTask(const QString &taskId);
    void startChunkedUpload(const QString &taskId);
    void uploadNextChunk(const QString &taskId);
    void mergeChunks(const QString &taskId);
    void updateTaskProgress(const QString &taskId, qint64 transferred, qint64 total);
    void completeTask(const QString &taskId, const QString &resultUrl = "");
    void failTask(const QString &taskId, const QString &error);
    void cleanupTask(const QString &taskId);
    bool validateFile(const QString &filePath) const;
    QString getSaveDirectory() const;
    QNetworkRequest createUploadRequest(const TransferTask &task) const;
    QNetworkRequest createDownloadRequest(const TransferTask &task) const;
    
    // 成员变量
    QNetworkAccessManager *m_networkManager;
    QMap<QString, TransferTask> m_transferTasks;
    QMap<QNetworkReply*, QString> m_replyToTaskMap;
    QQueue<QString> m_transferQueue;
    QStringList m_activeTransfers;
    mutable QMutex m_mutex;
    
    // 配置参数
    int m_maxConcurrentTransfers;
    int m_chunkSize;
    QUrl m_uploadUrl;
    QUrl m_downloadBaseUrl;
    
    // 常量
    static const int DEFAULT_CHUNK_SIZE = 1024 * 1024; // 1MB
    static const int MAX_CONCURRENT_TRANSFERS = 3;
    static const int MAX_RETRY_COUNT = 3;
    static const qint64 MAX_FILE_SIZE = 100 * 1024 * 1024; // 100MB
    
    // 支持的文件类型
    static const QStringList IMAGE_EXTENSIONS;
    static const QStringList VIDEO_EXTENSIONS;
    static const QStringList AUDIO_EXTENSIONS;
};

Q_DECLARE_METATYPE(FileTransferManager::TransferTask)

#endif // FILETRANSFERMANAGER_H