#ifndef HEARTBEATMANAGER_H
#define HEARTBEATMANAGER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QQueue>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(heartbeatManager)

/**
 * @brief 心跳管理器
 * 
 * 负责管理心跳检测机制，包括：
 * - 心跳发送和接收
 * - 延迟监控
 * - 连接质量评估
 * - 自适应心跳间隔
 */
class HeartbeatManager : public QObject
{
    Q_OBJECT
    
public:
    enum HeartbeatState {
        Stopped,        // 停止状态
        Running,        // 运行状态
        WaitingResponse // 等待响应状态
    };
    Q_ENUM(HeartbeatState)
    
    struct HeartbeatRecord {
        QDateTime sentTime;
        QDateTime receivedTime;
        qint64 latency;
        bool successful;
        QString errorMessage;
    };
    
    explicit HeartbeatManager(QObject *parent = nullptr);
    ~HeartbeatManager();
    
    // 心跳控制
    void start();
    void stop();
    void pause();
    void resume();
    bool isRunning() const;
    HeartbeatState getState() const;
    
    // 心跳发送
    void sendHeartbeat();
    void handleHeartbeatResponse(const QDateTime &serverTime = QDateTime());
    
    // 配置
    void setInterval(int intervalMs);
    void setTimeout(int timeoutMs);
    void setMaxMissedBeats(int maxMissed);
    void setAdaptiveMode(bool enabled);
    void setLatencyThreshold(qint64 thresholdMs);
    
    // 状态查询
    int getInterval() const;
    int getTimeout() const;
    int getMissedBeats() const;
    int getMaxMissedBeats() const;
    qint64 getLastLatency() const;
    qint64 getAverageLatency() const;
    double getPacketLossRate() const;
    
    // 连接质量
    enum ConnectionQuality {
        Excellent,      // 优秀 (< 50ms, 0% loss)
        Good,          // 良好 (< 100ms, < 1% loss)
        Fair,          // 一般 (< 200ms, < 5% loss)
        Poor,          // 较差 (< 500ms, < 10% loss)
        Bad            // 很差 (>= 500ms or >= 10% loss)
    };
    Q_ENUM(ConnectionQuality)
    
    ConnectionQuality getConnectionQuality() const;
    QString getQualityDescription() const;
    
    // 统计信息
    int getTotalSent() const;
    int getTotalReceived() const;
    QList<HeartbeatRecord> getRecentRecords(int count = 10) const;
    void clearStatistics();
    
signals:
    void heartbeatSent(const QDateTime &timestamp);
    void heartbeatReceived(const QDateTime &timestamp, qint64 latency);
    void heartbeatTimeout();
    void connectionQualityChanged(ConnectionQuality quality);
    void latencyChanged(qint64 latency);
    void packetLossChanged(double lossRate);
    void maxMissedBeatsReached();
    
private slots:
    void onHeartbeatTimer();
    void onTimeoutTimer();
    void onQualityCheckTimer();
    
private:
    void updateStatistics(const HeartbeatRecord &record);
    void checkConnectionQuality();
    void adjustAdaptiveInterval();
    qint64 calculateAverageLatency() const;
    double calculatePacketLossRate() const;
    ConnectionQuality evaluateQuality(qint64 avgLatency, double lossRate) const;
    
    QTimer *_heartbeatTimer;
    QTimer *_timeoutTimer;
    QTimer *_qualityCheckTimer;
    
    HeartbeatState _state;
    
    // 配置参数
    int _interval;
    int _timeout;
    int _maxMissedBeats;
    bool _adaptiveMode;
    qint64 _latencyThreshold;
    
    // 状态信息
    int _missedBeats;
    QDateTime _lastSentTime;
    QDateTime _lastReceivedTime;
    qint64 _lastLatency;
    
    // 统计信息
    int _totalSent;
    int _totalReceived;
    QQueue<HeartbeatRecord> _recentRecords;
    ConnectionQuality _currentQuality;
    
    // 自适应参数
    qint64 _adaptiveBaseInterval;
    qint64 _adaptiveMinInterval;
    qint64 _adaptiveMaxInterval;
    
    // 默认配置
    static constexpr int DEFAULT_INTERVAL = 30000;          // 30秒
    static constexpr int DEFAULT_TIMEOUT = 10000;           // 10秒
    static constexpr int DEFAULT_MAX_MISSED_BEATS = 3;
    static constexpr qint64 DEFAULT_LATENCY_THRESHOLD = 200; // 200ms
    static constexpr int QUALITY_CHECK_INTERVAL = 60000;    // 60秒
    static constexpr int MAX_RECENT_RECORDS = 100;
    static constexpr int ADAPTIVE_MIN_INTERVAL = 10000;     // 10秒
    static constexpr int ADAPTIVE_MAX_INTERVAL = 120000;    // 120秒
};

#endif // HEARTBEATMANAGER_H
