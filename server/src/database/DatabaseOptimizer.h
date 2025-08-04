#ifndef DATABASEOPTIMIZER_H
#define DATABASEOPTIMIZER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariantMap>
#include <QLoggingCategory>
#include <QTimer>
#include <QHash>
#include <QDateTime>

Q_DECLARE_LOGGING_CATEGORY(databaseOptimizer)

class Database;
class CacheManagerV2;

class DatabaseOptimizer : public QObject
{
    Q_OBJECT

public:
    // 查询类型
    enum QueryType {
        Select = 0,
        Insert = 1,
        Update = 2,
        Delete = 3
    };
    Q_ENUM(QueryType)

    // 索引类型
    enum IndexType {
        BTreeIndex = 0,      // B-Tree索引
        HashIndex = 1,       // 哈希索引
        FullTextIndex = 2,   // 全文索引
        CompositeIndex = 3   // 复合索引
    };
    Q_ENUM(IndexType)

    // 查询性能统计
    struct QueryStats {
        QString query;
        QueryType type;
        qint64 executionTime;
        qint64 rowsAffected;
        qint64 rowsExamined;
        QDateTime timestamp;
        bool useIndex;
        QString indexUsed;
        double cpuTime;
        qint64 memoryUsed;
    };

    // 索引信息
    struct IndexInfo {
        QString name;
        QString table;
        QStringList columns;
        IndexType type;
        bool isUnique;
        qint64 size;
        double selectivity;
        qint64 usage;
        QDateTime createdAt;
        QDateTime lastUsed;
    };

    explicit DatabaseOptimizer(Database *database, CacheManagerV2 *cacheManager, QObject *parent = nullptr);
    ~DatabaseOptimizer();

    // 初始化和配置
    bool initialize();
    void enableOptimization(bool enable);
    void setSlowQueryThreshold(int milliseconds);
    void setAnalysisInterval(int seconds);

    // 查询优化
    QSqlQuery optimizeQuery(const QString &sql, const QVariantMap &parameters = QVariantMap());
    QString rewriteQuery(const QString &sql);
    QStringList suggestIndexes(const QString &sql);
    bool canUseCache(const QString &sql);

    // 索引管理
    bool createIndex(const QString &table, const QStringList &columns, IndexType type = BTreeIndex, bool unique = false);
    bool dropIndex(const QString &indexName);
    QList<IndexInfo> analyzeIndexUsage();
    QStringList suggestDropIndexes();
    bool optimizeIndexes();

    // 查询缓存
    bool cacheQuery(const QString &sql, const QVariantMap &parameters, const QVariantList &result, int ttl = 300);
    QVariantList getCachedQuery(const QString &sql, const QVariantMap &parameters);
    void invalidateQueryCache(const QString &pattern = QString());

    // 性能监控
    void logQuery(const QString &sql, qint64 executionTime, const QVariantMap &parameters = QVariantMap());
    QList<QueryStats> getSlowQueries(int limit = 100);
    QVariantMap getPerformanceMetrics();
    void analyzeTableStats();

    // 数据库维护
    bool vacuum();
    bool analyze();
    bool reindex();
    bool optimizeTables();
    qint64 getDatabaseSize();
    QVariantMap getTableSizes();

    // 分区管理
    bool createPartition(const QString &table, const QString &column, const QString &partitionType);
    bool dropPartition(const QString &table, const QString &partitionName);
    QStringList getPartitions(const QString &table);

    // 连接池优化
    void optimizeConnections();
    int getOptimalConnectionCount();
    void setConnectionPoolSize(int size);

signals:
    void slowQueryDetected(const QString &sql, qint64 executionTime);
    void indexCreated(const QString &indexName, const QString &table);
    void indexDropped(const QString &indexName);
    void cacheHit(const QString &sql);
    void cacheMiss(const QString &sql);
    void optimizationCompleted();

private slots:
    void performPeriodicAnalysis();
    void cleanupOldStats();

private:
    Database *m_database;
    CacheManagerV2 *m_cacheManager;
    QSqlDatabase m_db;

    // 配置
    bool m_optimizationEnabled;
    int m_slowQueryThreshold;
    int m_analysisInterval;
    int m_maxCachedQueries;
    int m_cacheDefaultTTL;

    // 统计数据
    QList<QueryStats> m_queryStats;
    QHash<QString, IndexInfo> m_indexes;
    QHash<QString, qint64> m_queryCache;
    
    // 定时器
    QTimer *m_analysisTimer;
    QTimer *m_cleanupTimer; // 清理定时器

    // 查询分析
    QString normalizeQuery(const QString &sql);
    QString generateCacheKey(const QString &sql, const QVariantMap &parameters);
    bool isSelectQuery(const QString &sql);
    bool isModifyingQuery(const QString &sql);
    QStringList extractTables(const QString &sql);
    QStringList extractColumns(const QString &sql);
    QStringList extractWhereColumns(const QString &sql);

    // 索引分析
    double calculateSelectivity(const QString &table, const QString &column);
    bool shouldCreateIndex(const QString &table, const QStringList &columns);
    bool shouldDropIndex(const IndexInfo &index);
    QString generateIndexName(const QString &table, const QStringList &columns);

    // 查询重写
    QString optimizeSelectQuery(const QString &sql);
    QString optimizeJoinQuery(const QString &sql);
    QString optimizeWhereClause(const QString &sql);
    QString optimizeOrderBy(const QString &sql);
    QString addQueryHints(const QString &sql);

    // 缓存管理
    void updateQueryCacheStats();
    void evictLeastUsedCache();
    bool shouldCacheQuery(const QString &sql, qint64 executionTime);

    // 统计收集
    void collectTableStats(const QString &table);
    void collectIndexStats();
    void collectQueryPlanStats(const QString &sql);

    // 性能调优
    QVariantMap analyzeQueryPlan(const QString &sql);
    QStringList identifyBottlenecks();
    QVariantMap generateOptimizationReport();

    // 维护任务
    void updateStatistics();
    void rebuildIndexes();
    void defragmentTables();
    void purgeOldData();

    // 辅助方法
    bool executeQuery(QSqlQuery &query, const QString &sql, const QVariantMap &parameters = QVariantMap());
    QVariantList queryToVariantList(QSqlQuery &query);
    QString formatExecutionTime(qint64 milliseconds);
    qint64 getCurrentTimestamp();
};

Q_DECLARE_METATYPE(DatabaseOptimizer::QueryType)
Q_DECLARE_METATYPE(DatabaseOptimizer::IndexType)
Q_DECLARE_METATYPE(DatabaseOptimizer::QueryStats)
Q_DECLARE_METATYPE(DatabaseOptimizer::IndexInfo)

#endif // DATABASEOPTIMIZER_H