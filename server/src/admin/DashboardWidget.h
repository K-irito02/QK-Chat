#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QGridLayout;
class QLabel;
class QProgressBar;
class QPushButton;
class QChart;
class QChartView;
QT_END_NAMESPACE

class ChatServer;

/**
 * @brief 仪表板组件
 * 
 * 显示服务器实时状态和统计信息
 */
class DashboardWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit DashboardWidget(QWidget *parent = nullptr);
    ~DashboardWidget();

    void setChatServer(ChatServer *server);
    void refreshData();
    void updateTheme(bool isDark);
    void manualRefresh(); // 手动刷新统计信息
    
private slots:
    void updateStatistics();
    void startStatisticsUpdate();
    void updateUIElements();
    void performSafeStatisticsUpdate();
    void onSafetyTimeout();
    void updateUptime(); // 更新运行时间
    void updateUserData(); // 更新用户数据
    void initializeStaticData(); // 初始化静态数据
    
private:
    void setupUI();
    void setupStatisticsCards();
    void setupCharts();
    void setupRefreshButton();
    
    QGridLayout *_mainLayout;
    
    // 统计卡片
    QLabel *_onlineUsersLabel;
    QLabel *_totalUsersLabel;
    QLabel *_messagesCountLabel;
    QLabel *_uptimeLabel;
    QLabel *_cpuUsageLabel;
    QLabel *_memoryUsageLabel;
    
    QProgressBar *_cpuProgressBar;
    QProgressBar *_memoryProgressBar;
    
    // 手动刷新按钮
    QPushButton *_refreshButton;
    
    ChatServer *_chatServer;
    bool _isDarkTheme;
    QTimer *_updateTimer;
    QTimer *_uiUpdateTimer;
    QTimer *_uptimeTimer; // 运行时间更新定时器
    QTimer *_userDataTimer; // 用户数据更新定时器
    
    // UI更新缓存
    struct UIUpdateCache {
        int onlineUsers = 0;
        int totalUsers = 0;
        int messagesCount = 0;
        QString uptime = "00:00:00";
        int cpuUsage = 0;
        int memoryUsage = 0;
        bool needsUpdate = false;
        QDateTime lastUpdateTime;
    };
    UIUpdateCache _uiCache;
    
    // 统计更新状态
    QAtomicInt _isUpdating{0};
    QAtomicInt _updateRequested{0};
    QTimer *_safetyTimer;
};

#endif // DASHBOARDWIDGET_H 