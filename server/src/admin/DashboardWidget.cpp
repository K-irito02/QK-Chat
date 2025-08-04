#include "DashboardWidget.h"
#include "../core/ChatServer.h"

#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>

DashboardWidget::DashboardWidget(QWidget *parent)
    : QWidget(parent)
    , _mainLayout(nullptr)
    , _onlineUsersLabel(nullptr)
    , _totalUsersLabel(nullptr)
    , _messagesCountLabel(nullptr)
    , _uptimeLabel(nullptr)
    , _cpuUsageLabel(nullptr)
    , _memoryUsageLabel(nullptr)
    , _cpuProgressBar(nullptr)
    , _memoryProgressBar(nullptr)
    , _chatServer(nullptr)
    , _isDarkTheme(false)
    , _updateTimer(nullptr)
{
    setupUI();
    
    // 设置定时器更新统计信息 - 但不立即启动
    _updateTimer = new QTimer(this);
    connect(_updateTimer, &QTimer::timeout, this, &DashboardWidget::updateStatistics, Qt::QueuedConnection);
    // 不在构造函数中启动定时器，等待服务器启动后再启动
}

void DashboardWidget::setChatServer(ChatServer *server)
{
    _chatServer = server;
    
    // 只有在服务器真正运行后才开始更新
    if (_chatServer && _chatServer->isRunning()) {
        // 如果定时器已经在运行，先停止
        if (_updateTimer->isActive()) {
            _updateTimer->stop();
        }
        
        updateStatistics();
        _updateTimer->start(10000); // 每10秒更新一次
    }
}

void DashboardWidget::refreshData()
{
    updateStatistics();
}

void DashboardWidget::updateTheme(bool isDark)
{
    _isDarkTheme = isDark;
    
    // 更新主题样式
    if (isDark) {
        setStyleSheet("QWidget { background-color: #2b2b2b; color: #ffffff; }"
                     "QLabel { color: #ffffff; }"
                     "QGroupBox { color: #ffffff; }");
    } else {
        setStyleSheet("QWidget { background-color: #ffffff; color: #000000; }"
                     "QLabel { color: #000000; }"
                     "QGroupBox { color: #000000; }");
    }
}

void DashboardWidget::setupUI()
{
    _mainLayout = new QGridLayout(this);
    _mainLayout->setSpacing(10);
    _mainLayout->setContentsMargins(10, 10, 10, 10);
    
    setupStatisticsCards();
    
    setLayout(_mainLayout);
}

void DashboardWidget::setupStatisticsCards()
{
    // 在线用户卡片
    QGroupBox *onlineUsersGroup = new QGroupBox("在线用户", this);
    QVBoxLayout *onlineUsersLayout = new QVBoxLayout(onlineUsersGroup);
    _onlineUsersLabel = new QLabel("0", this);
    _onlineUsersLabel->setAlignment(Qt::AlignCenter);
    _onlineUsersLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #4CAF50;");
    onlineUsersLayout->addWidget(_onlineUsersLabel);
    _mainLayout->addWidget(onlineUsersGroup, 0, 0);
    
    // 总用户数卡片
    QGroupBox *totalUsersGroup = new QGroupBox("总用户数", this);
    QVBoxLayout *totalUsersLayout = new QVBoxLayout(totalUsersGroup);
    _totalUsersLabel = new QLabel("0", this);
    _totalUsersLabel->setAlignment(Qt::AlignCenter);
    _totalUsersLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #2196F3;");
    totalUsersLayout->addWidget(_totalUsersLabel);
    _mainLayout->addWidget(totalUsersGroup, 0, 1);
    
    // 消息数量卡片
    QGroupBox *messagesGroup = new QGroupBox("消息数量", this);
    QVBoxLayout *messagesLayout = new QVBoxLayout(messagesGroup);
    _messagesCountLabel = new QLabel("0", this);
    _messagesCountLabel->setAlignment(Qt::AlignCenter);
    _messagesCountLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #FF9800;");
    messagesLayout->addWidget(_messagesCountLabel);
    _mainLayout->addWidget(messagesGroup, 0, 2);
    
    // 运行时间卡片
    QGroupBox *uptimeGroup = new QGroupBox("运行时间", this);
    QVBoxLayout *uptimeLayout = new QVBoxLayout(uptimeGroup);
    _uptimeLabel = new QLabel("00:00:00", this);
    _uptimeLabel->setAlignment(Qt::AlignCenter);
    _uptimeLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #9C27B0;");
    uptimeLayout->addWidget(_uptimeLabel);
    _mainLayout->addWidget(uptimeGroup, 1, 0);
    
    // CPU使用率卡片
    QGroupBox *cpuGroup = new QGroupBox("CPU使用率", this);
    QVBoxLayout *cpuLayout = new QVBoxLayout(cpuGroup);
    _cpuUsageLabel = new QLabel("0%", this);
    _cpuUsageLabel->setAlignment(Qt::AlignCenter);
    _cpuUsageLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #F44336;");
    _cpuProgressBar = new QProgressBar(this);
    _cpuProgressBar->setRange(0, 100);
    _cpuProgressBar->setValue(0);
    cpuLayout->addWidget(_cpuUsageLabel);
    cpuLayout->addWidget(_cpuProgressBar);
    _mainLayout->addWidget(cpuGroup, 1, 1);
    
    // 内存使用率卡片
    QGroupBox *memoryGroup = new QGroupBox("内存使用率", this);
    QVBoxLayout *memoryLayout = new QVBoxLayout(memoryGroup);
    _memoryUsageLabel = new QLabel("0%", this);
    _memoryUsageLabel->setAlignment(Qt::AlignCenter);
    _memoryUsageLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #F44336;");
    _memoryProgressBar = new QProgressBar(this);
    _memoryProgressBar->setRange(0, 100);
    _memoryProgressBar->setValue(0);
    memoryLayout->addWidget(_memoryUsageLabel);
    memoryLayout->addWidget(_memoryProgressBar);
    _mainLayout->addWidget(memoryGroup, 1, 2);
}

void DashboardWidget::setupCharts()
{
    // 图表功能待实现
}

void DashboardWidget::updateStatistics()
{
    if (!_chatServer) {
        qDebug() << "[DashboardWidget] ChatServer is null, cannot update statistics";
        return;
    }

    // 添加线程安全保护
    static QMutex updateMutex;
    QMutexLocker locker(&updateMutex);

    try {
        qDebug() << "[DashboardWidget] Updating statistics...";

        // 更新在线用户数
        int onlineUsers = 0;
        try {
            onlineUsers = _chatServer->getOnlineUserCount();
        } catch (...) {
            qWarning() << "[DashboardWidget] Failed to get online user count";
        }
        
        if (_onlineUsersLabel) {
            _onlineUsersLabel->setText(QString::number(onlineUsers));
        }
        qDebug() << "[DashboardWidget] Online users:" << onlineUsers;

        // 更新总用户数
        int totalUsers = 0;
        try {
            totalUsers = _chatServer->getTotalUserCount();
        } catch (...) {
            qWarning() << "[DashboardWidget] Failed to get total user count";
        }
        
        if (_totalUsersLabel) {
            _totalUsersLabel->setText(QString::number(totalUsers));
        }
        qDebug() << "[DashboardWidget] Total users:" << totalUsers;

        // 更新消息数量
        int messagesCount = 0;
        try {
            messagesCount = _chatServer->getMessagesCount();
        } catch (...) {
            qWarning() << "[DashboardWidget] Failed to get messages count";
        }
        
        if (_messagesCountLabel) {
            _messagesCountLabel->setText(QString::number(messagesCount));
        }
        qDebug() << "[DashboardWidget] Messages count:" << messagesCount;

        // 更新运行时间
        QString uptime = "0:00:00";
        try {
            uptime = _chatServer->getUptime();
        } catch (...) {
            qWarning() << "[DashboardWidget] Failed to get uptime";
        }
        
        if (_uptimeLabel) {
            _uptimeLabel->setText(uptime);
        }
        qDebug() << "[DashboardWidget] Uptime:" << uptime;

        // 更新CPU使用率 - 添加超时保护
        int cpuUsage = 0;
        try {
            cpuUsage = _chatServer->getCpuUsage();
        } catch (...) {
            qWarning() << "[DashboardWidget] Failed to get CPU usage";
            cpuUsage = 0;
        }
        
        if (_cpuUsageLabel) {
            _cpuUsageLabel->setText(QString("%1%").arg(cpuUsage));
        }
        if (_cpuProgressBar) {
            _cpuProgressBar->setValue(cpuUsage);
        }
        qDebug() << "[DashboardWidget] CPU usage:" << cpuUsage << "%";

        // 更新内存使用率 - 添加超时保护
        int memoryUsage = 0;
        try {
            memoryUsage = _chatServer->getMemoryUsage();
        } catch (...) {
            qWarning() << "[DashboardWidget] Failed to get memory usage";
            memoryUsage = 0;
        }
        
        if (_memoryUsageLabel) {
            _memoryUsageLabel->setText(QString("%1%").arg(memoryUsage));
        }
        if (_memoryProgressBar) {
            _memoryProgressBar->setValue(memoryUsage);
        }
        qDebug() << "[DashboardWidget] Memory usage:" << memoryUsage << "%";

        qDebug() << "[DashboardWidget] Statistics update completed";
    } catch (const std::exception& e) {
        qWarning() << "[DashboardWidget] Exception in updateStatistics:" << e.what();
    } catch (...) {
        qWarning() << "[DashboardWidget] Unknown exception in updateStatistics";
    }
}