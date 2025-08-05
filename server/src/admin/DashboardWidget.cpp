#include "DashboardWidget.h"
#include "../core/ChatServer.h"

#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTimer>
#include <QPushButton>
#include <QMutex>
#include <QMutexLocker>
#include <QPointer>
#include <QLoggingCategory>
#include <QThread>
#include <QApplication>
#include <QElapsedTimer>
#include <QtConcurrent>

Q_LOGGING_CATEGORY(dashboardWidget, "qkchat.server.admin.dashboardwidget")

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
    , _uiUpdateTimer(nullptr)
    , _safetyTimer(nullptr)
{
    setupUI();

    // 设置定时器更新统计信息 - 但不立即启动
    _updateTimer = new QTimer(this);
    connect(_updateTimer, &QTimer::timeout, this, &DashboardWidget::performSafeStatisticsUpdate, Qt::QueuedConnection);
    
    // 设置UI更新定时器，用于分批更新UI元素
    _uiUpdateTimer = new QTimer(this);
    connect(_uiUpdateTimer, &QTimer::timeout, this, &DashboardWidget::updateUIElements, Qt::QueuedConnection);
    _uiUpdateTimer->start(50); // 每50ms更新一次UI，提高响应性
    
    // 设置安全定时器，防止长时间阻塞
    _safetyTimer = new QTimer(this);
    connect(_safetyTimer, &QTimer::timeout, this, &DashboardWidget::onSafetyTimeout);
    _safetyTimer->setSingleShot(true);
    
    // 设置运行时间更新定时器
    _uptimeTimer = new QTimer(this);
    connect(_uptimeTimer, &QTimer::timeout, this, &DashboardWidget::updateUptime);
    _uptimeTimer->start(1000); // 每秒更新一次运行时间

    // 设置用户数据更新定时器
    _userDataTimer = new QTimer(this);
    connect(_userDataTimer, &QTimer::timeout, this, &DashboardWidget::updateUserData);
    _userDataTimer->start(3000); // 每3秒更新一次用户数据

    // 不在构造函数中启动定时器，等待服务器启动后再启动
    
    // 初始化UI缓存为默认值
    _uiCache.onlineUsers = 0;
    _uiCache.totalUsers = 0;
    _uiCache.messagesCount = 0;
    _uiCache.uptime = "00:00:00";
    _uiCache.cpuUsage = 0;
    _uiCache.memoryUsage = 0;
    _uiCache.needsUpdate = false;
    _uiCache.lastUpdateTime = QDateTime::currentDateTime();
}

DashboardWidget::~DashboardWidget()
{
    // 确保定时器被正确停止
    if (_updateTimer) {
        _updateTimer->stop();
        _updateTimer = nullptr;
    }

    // 清空ChatServer引用
    _chatServer = nullptr;
}

void DashboardWidget::setChatServer(ChatServer* chatServer)
{
    try {
        qCInfo(dashboardWidget) << "DashboardWidget::setChatServer called";
        
        // 确保在主线程中执行
        if (QThread::currentThread() != qApp->thread()) {
            qCWarning(dashboardWidget) << "setChatServer called from non-main thread, moving to main thread";
            QMetaObject::invokeMethod(this, [this, chatServer]() {
                setChatServer(chatServer);
            }, Qt::QueuedConnection);
            return;
        }
        
        _chatServer = chatServer;
        
        if (_chatServer) {
            qCInfo(dashboardWidget) << "ChatServer set successfully";
            
            // 检查服务器是否正在运行
            bool serverRunning = false;
            try {
                serverRunning = _chatServer->isRunning();
            } catch (const std::exception& e) {
                qCWarning(dashboardWidget) << "Exception checking server running status:" << e.what();
                serverRunning = false;
            } catch (...) {
                qCWarning(dashboardWidget) << "Unknown exception checking server running status";
                serverRunning = false;
            }
            
            if (serverRunning) {
                qCInfo(dashboardWidget) << "Server is running, initializing data immediately";
                
                // 刷新服务器缓存数据
                qCInfo(dashboardWidget) << "Refreshing server cache data...";
                _chatServer->refreshAllCaches();
                
                // 执行初始统计更新
                qCInfo(dashboardWidget) << "Performing initial statistics update...";
                initializeStaticData();
                
                // 立即更新一次统计数据
                updateStatistics();
                
                // 启动自动更新定时器
                QTimer::singleShot(3000, this, &DashboardWidget::startStatisticsUpdate);
            } else {
                qCInfo(dashboardWidget) << "Server is not running yet, will initialize data later";
                
                // 设置默认值
                if (_onlineUsersLabel) _onlineUsersLabel->setText("0");
                if (_totalUsersLabel) _totalUsersLabel->setText("0");
                if (_messagesCountLabel) _messagesCountLabel->setText("0");
                if (_uptimeLabel) _uptimeLabel->setText("00:00:00");
                if (_cpuUsageLabel) _cpuUsageLabel->setText("0%");
                if (_memoryUsageLabel) _memoryUsageLabel->setText("0%");
                if (_cpuProgressBar) _cpuProgressBar->setValue(0);
                if (_memoryProgressBar) _memoryProgressBar->setValue(0);
                
                // 延迟启动自动更新定时器，等待服务器启动
                QTimer::singleShot(5000, this, &DashboardWidget::startStatisticsUpdate);
            }
        } else {
            qCWarning(dashboardWidget) << "ChatServer is null";
        }
        
    } catch (const std::exception& e) {
        qCWarning(dashboardWidget) << "Exception in setChatServer:" << e.what();
    } catch (...) {
        qCWarning(dashboardWidget) << "Unknown exception in setChatServer";
    }
}

void DashboardWidget::refreshData()
{
    // 使用安全的统计更新方法
    performSafeStatisticsUpdate();
}

void DashboardWidget::manualRefresh()
{
    try {
        qCInfo(dashboardWidget) << "Manual refresh requested";
        
        // 检查ChatServer是否可用
        if (!_chatServer) {
            qCWarning(dashboardWidget) << "ChatServer is null, cannot refresh";
            return;
        }
        
        // 检查是否已经在更新中
        if (_isUpdating) {
            qCInfo(dashboardWidget) << "Statistics update already in progress, skipping manual refresh";
            return;
        }
        
        // 设置更新标志
        _isUpdating = true;
        
        // 更新按钮状态
        if (_refreshButton) {
            _refreshButton->setText("更新中...");
            _refreshButton->setEnabled(false);
        }
        
        // 设置安全超时
        if (_safetyTimer) {
            _safetyTimer->start(8000); // 8秒超时
        }
        
        // 执行统计更新
        updateStatistics();
        
        // 重置更新标志
        _isUpdating = false;
        
        // 恢复按钮状态
        if (_refreshButton) {
            _refreshButton->setText("刷新统计");
            _refreshButton->setEnabled(true);
        }
        
        // 停止安全定时器
        if (_safetyTimer) {
            _safetyTimer->stop();
        }
        
        qCInfo(dashboardWidget) << "Manual refresh completed";
        
    } catch (const std::exception& e) {
        qCWarning(dashboardWidget) << "Exception in manualRefresh:" << e.what();
        _isUpdating = false;
        if (_refreshButton) {
            _refreshButton->setText("刷新统计");
            _refreshButton->setEnabled(true);
        }
        if (_safetyTimer) {
            _safetyTimer->stop();
        }
    } catch (...) {
        qCWarning(dashboardWidget) << "Unknown exception in manualRefresh";
        _isUpdating = false;
        if (_refreshButton) {
            _refreshButton->setText("刷新统计");
            _refreshButton->setEnabled(true);
        }
        if (_safetyTimer) {
            _safetyTimer->stop();
        }
    }
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
    setupRefreshButton();
    
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

void DashboardWidget::setupRefreshButton()
{
    // 创建手动刷新按钮
    _refreshButton = new QPushButton("刷新统计", this);
    _refreshButton->setMinimumHeight(40);
    _refreshButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 5px;"
        "    font-size: 14px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #3d8b40;"
        "}"
    );
    
    // 连接按钮点击信号
    connect(_refreshButton, &QPushButton::clicked, this, &DashboardWidget::manualRefresh);
    
    // 将按钮添加到布局中
    _mainLayout->addWidget(_refreshButton, 2, 0, 1, 3); // 跨越3列
}

void DashboardWidget::updateStatistics()
{
    try {
        // 确保在主线程中执行
        if (QThread::currentThread() != qApp->thread()) {
            qCWarning(dashboardWidget) << "updateStatistics called from non-main thread, moving to main thread";
            QMetaObject::invokeMethod(this, "updateStatistics", Qt::QueuedConnection);
            return;
        }
        
        // 减少调试输出，避免日志过多
        static int updateCount = 0;
        updateCount++;
        if (updateCount % 10 == 0) { // 每10次更新输出一次日志
            qCInfo(dashboardWidget) << "Updating statistics... (update count:" << updateCount << ")";
        }

        if (!_chatServer) {
            qCWarning(dashboardWidget) << "ChatServer is null, cannot update statistics";
            return;
        }

        // 改进服务器运行状态检查 - 使用更可靠的方法
        bool serverRunning = false;
        try {
            serverRunning = _chatServer->isRunning();
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception checking server running status:" << e.what();
            serverRunning = false;
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception checking server running status";
            serverRunning = false;
        }

        if (!serverRunning) {
            qCInfo(dashboardWidget) << "ChatServer is not running, setting default values";
            if (_onlineUsersLabel) _onlineUsersLabel->setText("0");
            if (_totalUsersLabel) _totalUsersLabel->setText("0");
            if (_messagesCountLabel) _messagesCountLabel->setText("0");
            if (_uptimeLabel) _uptimeLabel->setText("00:00:00");
            if (_cpuUsageLabel) _cpuUsageLabel->setText("0%");
            if (_memoryUsageLabel) _memoryUsageLabel->setText("0%");
            if (_cpuProgressBar) _cpuProgressBar->setValue(0);
            if (_memoryProgressBar) _memoryProgressBar->setValue(0);
            return;
        }

        // 服务器正在运行，更新实际数据
        qCInfo(dashboardWidget) << "ChatServer is running, updating actual data";

        // 更新在线用户数 - 使用安全的缓存值
        int onlineUsers = 0;
        try {
            // 使用非阻塞方式获取统计信息，添加超时保护
            QElapsedTimer timer;
            timer.start();
            
            onlineUsers = _chatServer->getOnlineUserCount();
            
            // 如果获取时间超过100ms，记录警告
            if (timer.elapsed() > 100) {
                qCWarning(dashboardWidget) << "getOnlineUserCount took" << timer.elapsed() << "ms";
            }
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception getting online user count:" << e.what();
            onlineUsers = 0; // 使用默认值
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception getting online user count";
            onlineUsers = 0; // 使用默认值
        }
        
        // 更新总用户数 - 使用安全的缓存值
        int totalUsers = 0;
        try {
            // 使用非阻塞方式获取统计信息，添加超时保护
            QElapsedTimer timer;
            timer.start();
            
            totalUsers = _chatServer->getTotalUserCount();
            
            // 如果获取时间超过100ms，记录警告
            if (timer.elapsed() > 100) {
                qCWarning(dashboardWidget) << "getTotalUserCount took" << timer.elapsed() << "ms";
            }
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception getting total user count:" << e.what();
            totalUsers = 0; // 使用默认值
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception getting total user count";
            totalUsers = 0; // 使用默认值
        }

        // 更新UI显示
        if (_onlineUsersLabel) {
            _onlineUsersLabel->setText(QString::number(onlineUsers));
        }
        if (_totalUsersLabel) {
            _totalUsersLabel->setText(QString::number(totalUsers));
        }

        // 更新缓存而不是直接更新UI
        _uiCache.onlineUsers = onlineUsers;
        _uiCache.totalUsers = totalUsers;
        _uiCache.needsUpdate = true;

        // 更新消息数量 - 添加超时保护
        int messagesCount = 0;
        try {
            QElapsedTimer timer;
            timer.start();
            messagesCount = _chatServer->getMessagesCount();
            if (timer.elapsed() > 100) {
                qCWarning(dashboardWidget) << "getMessagesCount took" << timer.elapsed() << "ms";
            }
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception getting messages count:" << e.what();
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception getting messages count";
        }

        // 更新运行时间 - 添加超时保护
        QString uptime = "00:00:00";
        try {
            QElapsedTimer timer;
            timer.start();
            uptime = _chatServer->getUptime();
            if (timer.elapsed() > 100) {
                qCWarning(dashboardWidget) << "getUptime took" << timer.elapsed() << "ms";
            }
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception getting uptime:" << e.what();
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception getting uptime";
        }

        // 更新CPU使用率 - 添加超时保护
        int cpuUsage = 0;
        try {
            QElapsedTimer timer;
            timer.start();
            cpuUsage = _chatServer->getCpuUsage();
            if (timer.elapsed() > 100) {
                qCWarning(dashboardWidget) << "getCpuUsage took" << timer.elapsed() << "ms";
            }
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception getting CPU usage:" << e.what();
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception getting CPU usage";
        }

        // 更新内存使用率 - 添加超时保护
        int memoryUsage = 0;
        try {
            QElapsedTimer timer;
            timer.start();
            memoryUsage = _chatServer->getMemoryUsage();
            if (timer.elapsed() > 100) {
                qCWarning(dashboardWidget) << "getMemoryUsage took" << timer.elapsed() << "ms";
            }
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception getting memory usage:" << e.what();
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception getting memory usage";
        }

        // 更新UI显示
        if (_messagesCountLabel) {
            _messagesCountLabel->setText(QString::number(messagesCount));
        }
        if (_uptimeLabel) {
            _uptimeLabel->setText(uptime);
        }
        if (_cpuUsageLabel) {
            _cpuUsageLabel->setText(QString("%1%").arg(cpuUsage));
        }
        if (_memoryUsageLabel) {
            _memoryUsageLabel->setText(QString("%1%").arg(memoryUsage));
        }
        if (_cpuProgressBar) {
            _cpuProgressBar->setValue(cpuUsage);
        }
        if (_memoryProgressBar) {
            _memoryProgressBar->setValue(memoryUsage);
        }

        // 更新缓存
        _uiCache.messagesCount = messagesCount;
        _uiCache.uptime = uptime;
        _uiCache.cpuUsage = cpuUsage;
        _uiCache.memoryUsage = memoryUsage;
        _uiCache.lastUpdateTime = QDateTime::currentDateTime();

        qCInfo(dashboardWidget) << "Statistics updated - Online:" << onlineUsers 
                                << "Total:" << totalUsers << "Messages:" << messagesCount
                                << "CPU:" << cpuUsage << "% Memory:" << memoryUsage << "%";
        
    } catch (const std::exception& e) {
        qCWarning(dashboardWidget) << "Exception in updateStatistics:" << e.what();
    } catch (...) {
        qCWarning(dashboardWidget) << "Unknown exception in updateStatistics";
    }
}

void DashboardWidget::startStatisticsUpdate()
{
    try {
        qCInfo(dashboardWidget) << "Starting statistics update timer";
        
        // 立即执行一次统计更新
        performSafeStatisticsUpdate();
        
        // 启动定时器，每3秒更新一次用户数据，确保实时性
        if (_updateTimer) {
            _updateTimer->start(3000); // 3秒更新一次，提高实时性
            qCInfo(dashboardWidget) << "Statistics update timer started successfully (3s interval)";
        } else {
            qCWarning(dashboardWidget) << "Update timer not available";
        }
    } catch (const std::exception& e) {
        qCWarning(dashboardWidget) << "Exception in startStatisticsUpdate:" << e.what();
    } catch (...) {
        qCWarning(dashboardWidget) << "Unknown exception in startStatisticsUpdate";
    }
}

void DashboardWidget::performSafeStatisticsUpdate()
{
    try {
        qCInfo(dashboardWidget) << "Performing safe statistics update...";
        
        // 重置更新标志
        _isUpdating = false;
        
        // 执行统计更新
        updateStatistics();
        
        // 确保更新完成后重置标志
        _isUpdating = false;
        
        qCInfo(dashboardWidget) << "Safe statistics update completed";
    } catch (const std::exception& e) {
        qCWarning(dashboardWidget) << "Exception in performSafeStatisticsUpdate:" << e.what();
        _isUpdating = false; // 确保异常时也重置标志
    } catch (...) {
        qCWarning(dashboardWidget) << "Unknown exception in performSafeStatisticsUpdate";
        _isUpdating = false; // 确保异常时也重置标志
    }
}

void DashboardWidget::onSafetyTimeout()
{
    qCWarning(dashboardWidget) << "Statistics update timeout, forcing reset";
    _isUpdating.storeRelease(0);

    // 恢复刷新按钮状态
    if (_refreshButton) {
        _refreshButton->setText("刷新统计");
        _refreshButton->setEnabled(true);
    }

    qCWarning(dashboardWidget) << "Statistics update timeout recovery completed";
}

void DashboardWidget::updateUIElements()
{
    // 只在需要更新时才更新UI元素
    if (!_uiCache.needsUpdate) {
        return;
    }
    
    // 确保在UI线程中执行
    if (QThread::currentThread() != qApp->thread()) {
        return;
    }
    
    // 检查更新频率，避免过于频繁的更新
    static QDateTime lastUIUpdate;
    QDateTime now = QDateTime::currentDateTime();
    if (lastUIUpdate.isValid() && lastUIUpdate.msecsTo(now) < 100) {
        return; // 100ms内不重复更新
    }
    lastUIUpdate = now;
    
    try {
        // 使用批量更新，减少重绘次数
        setUpdatesEnabled(false);
        
        // 更新在线用户数
        if (_onlineUsersLabel) {
            _onlineUsersLabel->setText(QString::number(_uiCache.onlineUsers));
        }
        
        // 更新总用户数
        if (_totalUsersLabel) {
            _totalUsersLabel->setText(QString::number(_uiCache.totalUsers));
        }
        
        // 更新消息数量
        if (_messagesCountLabel) {
            _messagesCountLabel->setText(QString::number(_uiCache.messagesCount));
        }
        
        // 更新运行时间 - 只在缓存数据更新时才更新，避免与实时更新冲突
        // 运行时间由独立的定时器实时更新，这里不需要更新
        // if (_uptimeLabel) {
        //     _uptimeLabel->setText(_uiCache.uptime);
        // }
        
        // 更新CPU使用率
        if (_cpuUsageLabel) {
            _cpuUsageLabel->setText(QString("%1%").arg(_uiCache.cpuUsage));
        }
        if (_cpuProgressBar) {
            _cpuProgressBar->setValue(_uiCache.cpuUsage);
        }
        
        // 更新内存使用率
        if (_memoryUsageLabel) {
            _memoryUsageLabel->setText(QString("%1%").arg(_uiCache.memoryUsage));
        }
        if (_memoryProgressBar) {
            _memoryProgressBar->setValue(_uiCache.memoryUsage);
        }
        
        setUpdatesEnabled(true);
        
        // 标记更新完成
        _uiCache.needsUpdate = false;
        
        // 记录更新日志
        static int updateCount = 0;
        updateCount++;
        if (updateCount % 20 == 0) { // 每20次更新输出一次日志
            qCInfo(dashboardWidget) << "UI elements updated successfully, count:" << updateCount;
        }
        
    } catch (const std::exception& e) {
        qCWarning(dashboardWidget) << "Exception in updateUIElements:" << e.what();
        setUpdatesEnabled(true);
    } catch (...) {
        qCWarning(dashboardWidget) << "Unknown exception in updateUIElements";
        setUpdatesEnabled(true);
    }
}

void DashboardWidget::updateUptime()
{
    try {
        if (_chatServer) {
            bool serverRunning = false;
            try {
                serverRunning = _chatServer->isRunning();
            } catch (...) {
                qCWarning(dashboardWidget) << "Failed to check server running status in updateUptime";
                serverRunning = false;
            }

            if (serverRunning) {
                QString uptime = _chatServer->getUptime();

                // 更新缓存
                _uiCache.uptime = uptime;

                // 直接更新运行时间标签，确保实时显示
                if (_uptimeLabel) {
                    _uptimeLabel->setText(uptime);
                }
            } else {
                // 服务器未运行时显示默认值
                if (_uptimeLabel) {
                    _uptimeLabel->setText("00:00:00");
                }
                _uiCache.uptime = "00:00:00";
            }
        } else {
            // ChatServer为空时显示默认值
            if (_uptimeLabel) {
                _uptimeLabel->setText("00:00:00");
            }
            _uiCache.uptime = "00:00:00";
        }
    } catch (const std::exception& e) {
        qCWarning(dashboardWidget) << "Exception in updateUptime:" << e.what();
    } catch (...) {
        qCWarning(dashboardWidget) << "Unknown exception in updateUptime";
    }
}

void DashboardWidget::updateUserData()
{
    try {
        if (_chatServer) {
            bool serverRunning = false;
            try {
                serverRunning = _chatServer->isRunning();
            } catch (...) {
                qCWarning(dashboardWidget) << "Failed to check server running status in updateUserData";
                serverRunning = false;
            }

            if (serverRunning) {
                // 更新在线用户数
                int onlineUsers = 0;
                try {
                    onlineUsers = _chatServer->getOnlineUserCount();
                } catch (...) {
                    qCWarning(dashboardWidget) << "Failed to get online user count in updateUserData";
                    onlineUsers = 0;
                }

                if (_onlineUsersLabel) {
                    _onlineUsersLabel->setText(QString::number(onlineUsers));
                }
                _uiCache.onlineUsers = onlineUsers;

                // 更新总用户数
                int totalUsers = 0;
                try {
                    totalUsers = _chatServer->getTotalUserCount();
                } catch (...) {
                    qCWarning(dashboardWidget) << "Failed to get total user count in updateUserData";
                    totalUsers = 0;
                }

                if (_totalUsersLabel) {
                    _totalUsersLabel->setText(QString::number(totalUsers));
                }
                _uiCache.totalUsers = totalUsers;
            } else {
                // 服务器未运行时显示默认值
                if (_onlineUsersLabel) {
                    _onlineUsersLabel->setText("0");
                }
                if (_totalUsersLabel) {
                    _totalUsersLabel->setText("0");
                }
                _uiCache.onlineUsers = 0;
                _uiCache.totalUsers = 0;
            }
        } else {
            // ChatServer为空时显示默认值
            if (_onlineUsersLabel) {
                _onlineUsersLabel->setText("0");
            }
            if (_totalUsersLabel) {
                _totalUsersLabel->setText("0");
            }
            _uiCache.onlineUsers = 0;
            _uiCache.totalUsers = 0;
        }
    } catch (const std::exception& e) {
        qCWarning(dashboardWidget) << "Exception in updateUserData:" << e.what();
    } catch (...) {
        qCWarning(dashboardWidget) << "Unknown exception in updateUserData";
    }
}

void DashboardWidget::initializeStaticData()
{
    try {
        qCInfo(dashboardWidget) << "Initializing static data...";
        
        // 确保在主线程中执行
        if (QThread::currentThread() != qApp->thread()) {
            qCWarning(dashboardWidget) << "initializeStaticData called from non-main thread, moving to main thread";
            QMetaObject::invokeMethod(this, "initializeStaticData", Qt::QueuedConnection);
            return;
        }
        
        if (!_chatServer) {
            qCWarning(dashboardWidget) << "ChatServer is null, cannot initialize static data";
            return;
        }

        // 检查服务器是否正在启动过程中 - 避免重复调用initializeDatabase
        bool serverRunning = false;
        try {
            serverRunning = _chatServer->isRunning();
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception checking server running status:" << e.what();
            serverRunning = false;
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception checking server running status";
            serverRunning = false;
        }

        // 简化逻辑：如果服务器没有运行，就设置默认值
        if (!serverRunning) {
            qCInfo(dashboardWidget) << "ChatServer not running, setting default values";
            // 设置默认值
            if (_messagesCountLabel) _messagesCountLabel->setText("0");
            if (_cpuUsageLabel) _cpuUsageLabel->setText("0%");
            if (_memoryUsageLabel) _memoryUsageLabel->setText("0%");
            if (_cpuProgressBar) _cpuProgressBar->setValue(0);
            if (_memoryProgressBar) _memoryProgressBar->setValue(0);
            _uiCache.messagesCount = 0;
            _uiCache.cpuUsage = 0;
            _uiCache.memoryUsage = 0;
            return;
        }

        // 服务器正在运行，获取实际数据
        qCInfo(dashboardWidget) << "ChatServer is running, getting actual data";
        
        // 获取消息数量
        int messagesCount = 0;
        try {
            messagesCount = _chatServer->getMessagesCount();
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception getting messages count:" << e.what();
            messagesCount = 0;
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception getting messages count";
            messagesCount = 0;
        }
        
        if (_messagesCountLabel) {
            _messagesCountLabel->setText(QString::number(messagesCount));
        }
        _uiCache.messagesCount = messagesCount;

        // 获取CPU使用率
        int cpuUsage = 0;
        try {
            cpuUsage = _chatServer->getCpuUsage();
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception getting CPU usage:" << e.what();
            cpuUsage = 0;
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception getting CPU usage";
            cpuUsage = 0;
        }
        
        if (_cpuUsageLabel) {
            _cpuUsageLabel->setText(QString("%1%").arg(cpuUsage));
        }
        if (_cpuProgressBar) {
            _cpuProgressBar->setValue(cpuUsage);
        }
        _uiCache.cpuUsage = cpuUsage;

        // 获取内存使用率
        int memoryUsage = 0;
        try {
            memoryUsage = _chatServer->getMemoryUsage();
        } catch (const std::exception& e) {
            qCWarning(dashboardWidget) << "Exception getting memory usage:" << e.what();
            memoryUsage = 0;
        } catch (...) {
            qCWarning(dashboardWidget) << "Unknown exception getting memory usage";
            memoryUsage = 0;
        }
        
        if (_memoryUsageLabel) {
            _memoryUsageLabel->setText(QString("%1%").arg(memoryUsage));
        }
        if (_memoryProgressBar) {
            _memoryProgressBar->setValue(memoryUsage);
        }
        _uiCache.memoryUsage = memoryUsage;

        qCInfo(dashboardWidget) << "Static data initialized - Messages:" << messagesCount 
                                << "CPU:" << cpuUsage << "% Memory:" << memoryUsage << "%";
        
    } catch (const std::exception& e) {
        qCWarning(dashboardWidget) << "Exception in initializeStaticData:" << e.what();
    } catch (...) {
        qCWarning(dashboardWidget) << "Unknown exception in initializeStaticData";
    }
}