#ifndef DASHBOARDWIDGET_H
#define DASHBOARDWIDGET_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QGridLayout;
class QLabel;
class QProgressBar;
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
    
    void setChatServer(ChatServer *server);
    void refreshData();
    void updateTheme(bool isDark);
    
private slots:
    void updateStatistics();
    
private:
    void setupUI();
    void setupStatisticsCards();
    void setupCharts();
    
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
    
    ChatServer *_chatServer;
    bool _isDarkTheme;
    QTimer *_updateTimer;
};

#endif // DASHBOARDWIDGET_H 