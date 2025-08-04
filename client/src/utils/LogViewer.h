#ifndef LOGVIEWER_H
#define LOGVIEWER_H

#include <QObject>
#include <QWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QFile>
#include <QFileSystemWatcher>
#include <QDateTime>
#include <QRegExp>
#include <QLabel>
#include <QLineEdit>

/**
 * @brief 日志查看器类
 * 
 * 提供实时日志查看功能，包括：
 * - 实时日志显示
 * - 日志过滤
 * - 日志搜索
 * - 日志级别过滤
 */
class LogViewer : public QWidget
{
    Q_OBJECT
    
public:
    explicit LogViewer(QWidget *parent = nullptr);
    ~LogViewer();
    
    // 日志控制
    void startLogging();
    void stopLogging();
    void clearLogs();
    void refreshLogs();
    
    // 过滤设置
    void setLogLevelFilter(const QString &level);
    void setLogTypeFilter(const QString &type);
    void setTextFilter(const QString &text);
    void setTimeRange(const QDateTime &start, const QDateTime &end);
    
    // 搜索功能
    void searchText(const QString &text, bool caseSensitive = false);
    void searchNext();
    void searchPrevious();
    
    // 导出功能
    void exportLogs(const QString &filePath);
    void exportFilteredLogs(const QString &filePath);
    
    // 统计信息
    int getTotalLogLines() const;
    int getFilteredLogLines() const;
    QHash<QString, int> getLogLevelStatistics() const;
    QHash<QString, int> getLogTypeStatistics() const;
    
signals:
    void logLineAdded(const QString &line);
    void logFiltered(int totalLines, int filteredLines);
    void searchResultFound(int lineNumber, const QString &line);
    
private slots:
    void onLogFileChanged(const QString &path);
    void onLogDirectoryChanged(const QString &path);
    void onFilterChanged();
    void onSearchTextChanged();
    void onRefreshTimer();
    void onExportClicked();
    void onClearClicked();
    
private:
    void setupUI();
    void setupConnections();
    void loadLogFiles();
    void parseLogLine(const QString &line);
    void applyFilters();
    void highlightSearchResults();
    void updateStatistics();
    
    // UI组件
    QTextEdit *_logDisplay;
    QComboBox *_levelFilter;
    QComboBox *_typeFilter;
    QLineEdit *_textFilter;
    QPushButton *_refreshButton;
    QPushButton *_clearButton;
    QPushButton *_exportButton;
    QPushButton *_searchButton;
    QLineEdit *_searchInput;
    QLabel *_statusLabel;
    
    // 数据
    QStringList _logLines;
    QStringList _filteredLines;
    QHash<QString, int> _levelStatistics;
    QHash<QString, int> _typeStatistics;
    
    // 过滤条件
    QString _currentLevelFilter;
    QString _currentTypeFilter;
    QString _currentTextFilter;
    QDateTime _startTime;
    QDateTime _endTime;
    
    // 搜索
    QString _searchText;
    bool _caseSensitive;
    QList<int> _searchResults;
    int _currentSearchIndex;
    
    // 文件监控
    QFileSystemWatcher *_fileWatcher;
    QTimer *_refreshTimer;
    QString _logDirectory;
    
    // 配置
    bool _autoRefresh;
    int _maxLines;
    bool _showTimestamp;
    bool _showLogLevel;
    bool _showLogType;
    
    static const int DEFAULT_MAX_LINES = 10000;
    static const int REFRESH_INTERVAL = 1000; // 1秒
};

#endif // LOGVIEWER_H 