#include "DatabaseOptimizer.h"
#include "Database.h"
#include "../cache/CacheManagerV2.h"
#include <QSqlError>
#include <QSqlRecord>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimer>
#include <QThread>
#include <QDebug>
#include <algorithm>

Q_LOGGING_CATEGORY(databaseOptimizer, "qkchat.server.database.optimizer")

DatabaseOptimizer::DatabaseOptimizer(Database *database, CacheManagerV2 *cacheManager, QObject *parent)
    : QObject(parent)
    , m_database(database)
    , m_cacheManager(cacheManager)
    , m_optimizationEnabled(true)
    , m_slowQueryThreshold(1000) // 1秒
    , m_analysisInterval(3600) // 1小时
    , m_maxCachedQueries(1000)
    , m_cacheDefaultTTL(300) // 5分钟
    , m_analysisTimer(nullptr)
    , m_cleanupTimer(nullptr)
{
    qCInfo(databaseOptimizer) << "DatabaseOptimizer created";
}

DatabaseOptimizer::~DatabaseOptimizer()
{
    if (m_analysisTimer) {
        m_analysisTimer->stop();
        delete m_analysisTimer;
    }
    
    if (m_cleanupTimer) {
        m_cleanupTimer->stop();
        delete m_cleanupTimer;
    }
    
    qCInfo(databaseOptimizer) << "DatabaseOptimizer destroyed";
}

bool DatabaseOptimizer::initialize()
{
    if (!m_database) {
        qCCritical(databaseOptimizer) << "Database instance is null";
        return false;
    }
    
    // 获取数据库连接
    m_db = m_database->getDatabase();
    if (!m_db.isValid()) {
        qCCritical(databaseOptimizer) << "Invalid database connection";
        return false;
    }
    
    // 创建分析定时器
    m_analysisTimer = new QTimer(this);
    connect(m_analysisTimer, &QTimer::timeout, this, &DatabaseOptimizer::performPeriodicAnalysis);
    m_analysisTimer->start(m_analysisInterval * 1000);
    
    // 创建清理定时器
    m_cleanupTimer = new QTimer(this);
    connect(m_cleanupTimer, &QTimer::timeout, this, &DatabaseOptimizer::cleanupOldStats);
    m_cleanupTimer->start(24 * 3600 * 1000); // 每天清理一次
    
    // 收集现有索引信息
    collectIndexStats();
    
    qCInfo(databaseOptimizer) << "DatabaseOptimizer initialized successfully";
    return true;
}

void DatabaseOptimizer::enableOptimization(bool enable)
{
    m_optimizationEnabled = enable;
    qCDebug(databaseOptimizer) << "Optimization enabled:" << enable;
}

void DatabaseOptimizer::setSlowQueryThreshold(int milliseconds)
{
    m_slowQueryThreshold = milliseconds;
    qCDebug(databaseOptimizer) << "Slow query threshold set to:" << milliseconds << "ms";
}

void DatabaseOptimizer::setAnalysisInterval(int seconds)
{
    m_analysisInterval = seconds;
    if (m_analysisTimer) {
        m_analysisTimer->setInterval(seconds * 1000);
    }
    qCDebug(databaseOptimizer) << "Analysis interval set to:" << seconds << "seconds";
}

// 查询优化
QSqlQuery DatabaseOptimizer::optimizeQuery(const QString &sql, const QVariantMap &parameters)
{
    qint64 startTime = getCurrentTimestamp();
    
    // 检查缓存
    if (canUseCache(sql)) {
        QString cacheKey = generateCacheKey(sql, parameters);
        if (m_cacheManager) {
            QVariantList cachedResult = m_cacheManager->getCachedQuery(sql, parameters);
            if (!cachedResult.isEmpty()) {
                emit cacheHit(sql);
                QSqlQuery query(m_db);
                // 返回缓存的结果（这里需要特殊处理）
                return query;
            }
        }
        emit cacheMiss(sql);
    }
    
    // 重写查询以提高性能
    QString optimizedSql = rewriteQuery(sql);
    
    QSqlQuery query(m_db);
    query.prepare(optimizedSql);
    
    // 绑定参数
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        query.bindValue(it.key(), it.value());
    }
    
    // 执行查询
    bool success = executeQuery(query, optimizedSql, parameters);
    
    qint64 executionTime = getCurrentTimestamp() - startTime;
    
    // 记录查询统计
    logQuery(optimizedSql, executionTime, parameters);
    
    // 如果是慢查询，发出信号
    if (executionTime > m_slowQueryThreshold) {
        emit slowQueryDetected(optimizedSql, executionTime);
    }
    
    // 缓存结果（对于SELECT查询）
    if (canUseCache(optimizedSql) && success && isSelectQuery(optimizedSql)) {
        QVariantList result = queryToVariantList(query);
        if (m_cacheManager) {
            m_cacheManager->cacheQuery(sql, parameters, result, m_cacheDefaultTTL);
        }
    }
    
    return query;
}

QString DatabaseOptimizer::rewriteQuery(const QString &sql)
{
    if (!m_optimizationEnabled) {
        return sql;
    }
    
    QString optimized = sql;
    
    // 优化SELECT查询
    if (isSelectQuery(sql)) {
        optimized = optimizeSelectQuery(optimized);
        optimized = optimizeJoinQuery(optimized);
        optimized = optimizeWhereClause(optimized);
        optimized = optimizeOrderBy(optimized);
        optimized = addQueryHints(optimized);
    }
    
    return optimized;
}

QStringList DatabaseOptimizer::suggestIndexes(const QString &sql)
{
    QStringList suggestions;
    
    if (!isSelectQuery(sql)) {
        return suggestions;
    }
    
    // 提取表和列信息
    QStringList tables = extractTables(sql);
    QStringList whereColumns = extractWhereColumns(sql);
    
    for (const QString &table : tables) {
        for (const QString &column : whereColumns) {
            if (shouldCreateIndex(table, QStringList() << column)) {
                QString indexName = generateIndexName(table, QStringList() << column);
                suggestions.append(QString("CREATE INDEX %1 ON %2 (%3)")
                                  .arg(indexName, table, column));
            }
        }
    }
    
    return suggestions;
}

bool DatabaseOptimizer::canUseCache(const QString &sql)
{
    // 只缓存SELECT查询
    if (!isSelectQuery(sql)) {
        return false;
    }
    
    // 不缓存包含NOW()、RAND()等函数的查询
    QStringList nonCacheableFunctions = {"NOW()", "RAND()", "CURRENT_TIMESTAMP", "UUID()"};
    for (const QString &func : nonCacheableFunctions) {
        if (sql.toUpper().contains(func)) {
            return false;
        }
    }
    
    return true;
}

// 索引管理
bool DatabaseOptimizer::createIndex(const QString &table, const QStringList &columns, IndexType type, bool unique)
{
    if (columns.isEmpty()) {
        return false;
    }
    
    QString indexName = generateIndexName(table, columns);
    QString indexType;
    
    switch (type) {
    case BTreeIndex:
        indexType = "BTREE";
        break;
    case HashIndex:
        indexType = "HASH";
        break;
    case FullTextIndex:
        indexType = "FULLTEXT";
        break;
    default:
        indexType = "BTREE";
        break;
    }
    
    QString sql = QString("CREATE %1 INDEX %2 ON %3 (%4) USING %5")
                  .arg(unique ? "UNIQUE" : "")
                  .arg(indexName)
                  .arg(table)
                  .arg(columns.join(", "))
                  .arg(indexType);
    
    QSqlQuery query(m_db);
    if (query.exec(sql)) {
        // 记录索引信息
        IndexInfo info;
        info.name = indexName;
        info.table = table;
        info.columns = columns;
        info.type = type;
        info.isUnique = unique;
        info.createdAt = QDateTime::currentDateTime();
        info.usage = 0;
        
        m_indexes[indexName] = info;
        
        emit indexCreated(indexName, table);
        qCInfo(databaseOptimizer) << "Index created:" << indexName << "on table" << table;
        return true;
    } else {
        qCWarning(databaseOptimizer) << "Failed to create index:" << query.lastError().text();
        return false;
    }
}

bool DatabaseOptimizer::dropIndex(const QString &indexName)
{
    QString sql = QString("DROP INDEX %1").arg(indexName);
    
    QSqlQuery query(m_db);
    if (query.exec(sql)) {
        m_indexes.remove(indexName);
        emit indexDropped(indexName);
        qCInfo(databaseOptimizer) << "Index dropped:" << indexName;
        return true;
    } else {
        qCWarning(databaseOptimizer) << "Failed to drop index:" << query.lastError().text();
        return false;
    }
}

QList<DatabaseOptimizer::IndexInfo> DatabaseOptimizer::analyzeIndexUsage()
{
    collectIndexStats();
    return m_indexes.values();
}

QStringList DatabaseOptimizer::suggestDropIndexes()
{
    QStringList suggestions;
    
    for (const IndexInfo &info : m_indexes) {
        if (shouldDropIndex(info)) {
            suggestions.append(info.name);
        }
    }
    
    return suggestions;
}

bool DatabaseOptimizer::optimizeIndexes()
{
    // 分析索引使用情况
    analyzeIndexUsage();
    
    // 删除未使用的索引
    QStringList dropCandidates = suggestDropIndexes();
    for (const QString &indexName : dropCandidates) {
        dropIndex(indexName);
    }
    
    // TODO: 根据查询模式创建推荐的索引
    
    emit optimizationCompleted();
    return true;
}

// 查询缓存
bool DatabaseOptimizer::cacheQuery(const QString &sql, const QVariantMap &parameters, const QVariantList &result, int ttl)
{
    if (!m_cacheManager || !canUseCache(sql)) {
        return false;
    }
    
    QString cacheKey = generateCacheKey(sql, parameters);
    return m_cacheManager->set(cacheKey, result, ttl, "query_cache");
}

QVariantList DatabaseOptimizer::getCachedQuery(const QString &sql, const QVariantMap &parameters)
{
    if (!m_cacheManager || !canUseCache(sql)) {
        return QVariantList();
    }
    
    QString cacheKey = generateCacheKey(sql, parameters);
    return m_cacheManager->get(cacheKey).toList();
}

void DatabaseOptimizer::invalidateQueryCache(const QString &pattern)
{
    if (!m_cacheManager) {
        return;
    }
    
    if (pattern.isEmpty()) {
        m_cacheManager->clearCategory("query_cache");
    } else {
        // TODO: 实现模式匹配的缓存失效
        qCDebug(databaseOptimizer) << "Pattern-based cache invalidation not implemented:" << pattern;
    }
}

// 性能监控
void DatabaseOptimizer::logQuery(const QString &sql, qint64 executionTime, const QVariantMap &parameters)
{
    Q_UNUSED(parameters) // TODO: 实现参数记录
    
    QueryStats stats;
    stats.query = normalizeQuery(sql);
    stats.type = isSelectQuery(sql) ? Select : 
                 (sql.toUpper().startsWith("INSERT") ? Insert :
                  (sql.toUpper().startsWith("UPDATE") ? Update : Delete));
    stats.executionTime = executionTime;
    stats.timestamp = QDateTime::currentDateTime();
    stats.useIndex = false; // TODO: 从查询计划中提取
    stats.cpuTime = 0; // TODO: 实现CPU时间测量
    stats.memoryUsed = 0; // TODO: 实现内存使用测量
    
    m_queryStats.append(stats);
    
    // 限制统计数据大小
    if (m_queryStats.size() > 10000) {
        m_queryStats.removeFirst();
    }
    
    qCDebug(databaseOptimizer) << "Query logged:" << stats.query << "Time:" << executionTime << "ms";
}

QList<DatabaseOptimizer::QueryStats> DatabaseOptimizer::getSlowQueries(int limit)
{
    QList<QueryStats> slowQueries;
    
    for (const QueryStats &stats : m_queryStats) {
        if (stats.executionTime > m_slowQueryThreshold) {
            slowQueries.append(stats);
        }
    }
    
    // 按执行时间排序
    std::sort(slowQueries.begin(), slowQueries.end(), 
              [](const QueryStats &a, const QueryStats &b) {
                  return a.executionTime > b.executionTime;
              });
    
    if (limit > 0 && slowQueries.size() > limit) {
        slowQueries = slowQueries.mid(0, limit);
    }
    
    return slowQueries;
}

QVariantMap DatabaseOptimizer::getPerformanceMetrics()
{
    QVariantMap metrics;
    
    if (m_queryStats.isEmpty()) {
        return metrics;
    }
    
    qint64 totalTime = 0;
    int selectCount = 0, insertCount = 0, updateCount = 0, deleteCount = 0;
    int slowQueryCount = 0;
    
    for (const QueryStats &stats : m_queryStats) {
        totalTime += stats.executionTime;
        
        switch (stats.type) {
        case Select: selectCount++; break;
        case Insert: insertCount++; break;
        case Update: updateCount++; break;
        case Delete: deleteCount++; break;
        }
        
        if (stats.executionTime > m_slowQueryThreshold) {
            slowQueryCount++;
        }
    }
    
    int totalQueries = m_queryStats.size();
    double avgTime = static_cast<double>(totalTime) / totalQueries;
    double slowQueryRate = static_cast<double>(slowQueryCount) / totalQueries * 100;
    
    metrics["total_queries"] = totalQueries;
    metrics["total_time"] = totalTime;
    metrics["average_time"] = avgTime;
    metrics["slow_query_count"] = slowQueryCount;
    metrics["slow_query_rate"] = slowQueryRate;
    metrics["select_count"] = selectCount;
    metrics["insert_count"] = insertCount;
    metrics["update_count"] = updateCount;
    metrics["delete_count"] = deleteCount;
    metrics["index_count"] = m_indexes.size();
    
    return metrics;
}

void DatabaseOptimizer::analyzeTableStats()
{
    QStringList tables = {"users", "messages", "groups", "group_members", "friendships", "user_sessions"};
    
    for (const QString &table : tables) {
        collectTableStats(table);
    }
}

// 数据库维护
bool DatabaseOptimizer::vacuum()
{
    // MySQL不支持VACUUM，使用OPTIMIZE TABLE
    QStringList tables = {"users", "messages", "groups", "group_members", "friendships", "user_sessions"};
    
    bool allSuccess = true;
    for (const QString &table : tables) {
        QSqlQuery query(m_db);
        QString sql = QString("OPTIMIZE TABLE %1").arg(table);
        if (!query.exec(sql)) {
            qCWarning(databaseOptimizer) << "Failed to optimize table" << table << ":" << query.lastError().text();
            allSuccess = false;
        }
    }
    
    qCInfo(databaseOptimizer) << "Database vacuum/optimize completed";
    return allSuccess;
}

bool DatabaseOptimizer::analyze()
{
    QStringList tables = {"users", "messages", "groups", "group_members", "friendships", "user_sessions"};
    
    bool allSuccess = true;
    for (const QString &table : tables) {
        QSqlQuery query(m_db);
        QString sql = QString("ANALYZE TABLE %1").arg(table);
        if (!query.exec(sql)) {
            qCWarning(databaseOptimizer) << "Failed to analyze table" << table << ":" << query.lastError().text();
            allSuccess = false;
        }
    }
    
    qCInfo(databaseOptimizer) << "Database analyze completed";
    return allSuccess;
}

bool DatabaseOptimizer::reindex()
{
    // 重建所有索引
    for (const IndexInfo &info : m_indexes) {
        QString sql = QString("ALTER TABLE %1 DROP INDEX %2").arg(info.table, info.name);
        QSqlQuery dropQuery(m_db);
        dropQuery.exec(sql);
        
        // 重新创建索引
        createIndex(info.table, info.columns, info.type, info.isUnique);
    }
    
    qCInfo(databaseOptimizer) << "Database reindex completed";
    return true;
}

bool DatabaseOptimizer::optimizeTables()
{
    return vacuum() && analyze();
}

qint64 DatabaseOptimizer::getDatabaseSize()
{
    QSqlQuery query(m_db);
    query.prepare("SELECT ROUND(SUM(data_length + index_length) / 1024 / 1024, 1) AS 'DB Size in MB' "
                  "FROM information_schema.tables WHERE table_schema = ?");
    query.addBindValue(m_db.databaseName());
    
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong() * 1024 * 1024; // 返回字节
    }
    
    return 0;
}

QVariantMap DatabaseOptimizer::getTableSizes()
{
    QVariantMap sizes;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT table_name, ROUND(((data_length + index_length) / 1024 / 1024), 2) AS 'size_mb' "
                  "FROM information_schema.tables WHERE table_schema = ? ORDER BY size_mb DESC");
    query.addBindValue(m_db.databaseName());
    
    if (query.exec()) {
        while (query.next()) {
            QString tableName = query.value(0).toString();
            double sizeMB = query.value(1).toDouble();
            sizes[tableName] = sizeMB;
        }
    }
    
    return sizes;
}

// 分区管理
bool DatabaseOptimizer::createPartition(const QString &table, const QString &column, const QString &partitionType)
{
    // MySQL分区示例
    QString sql = QString("ALTER TABLE %1 PARTITION BY %2 (%3) PARTITIONS 4")
                  .arg(table, partitionType, column);
    
    QSqlQuery query(m_db);
    if (query.exec(sql)) {
        qCInfo(databaseOptimizer) << "Partition created for table:" << table;
        return true;
    } else {
        qCWarning(databaseOptimizer) << "Failed to create partition:" << query.lastError().text();
        return false;
    }
}

bool DatabaseOptimizer::dropPartition(const QString &table, const QString &partitionName)
{
    QString sql = QString("ALTER TABLE %1 DROP PARTITION %2").arg(table, partitionName);
    
    QSqlQuery query(m_db);
    if (query.exec(sql)) {
        qCInfo(databaseOptimizer) << "Partition dropped:" << partitionName;
        return true;
    } else {
        qCWarning(databaseOptimizer) << "Failed to drop partition:" << query.lastError().text();
        return false;
    }
}

QStringList DatabaseOptimizer::getPartitions(const QString &table)
{
    QStringList partitions;
    
    QSqlQuery query(m_db);
    query.prepare("SELECT partition_name FROM information_schema.partitions "
                  "WHERE table_schema = ? AND table_name = ? AND partition_name IS NOT NULL");
    query.addBindValue(m_db.databaseName());
    query.addBindValue(table);
    
    if (query.exec()) {
        while (query.next()) {
            partitions.append(query.value(0).toString());
        }
    }
    
    return partitions;
}

// 连接池优化
void DatabaseOptimizer::optimizeConnections()
{
    // TODO: 实现连接池优化逻辑
    qCDebug(databaseOptimizer) << "Connection pool optimization not implemented";
}

int DatabaseOptimizer::getOptimalConnectionCount()
{
    // 基于CPU核心数和预期负载计算最优连接数
    int cpuCores = QThread::idealThreadCount();
    return qMax(10, cpuCores * 2);
}

void DatabaseOptimizer::setConnectionPoolSize(int size)
{
    // TODO: 实现连接池大小设置
    qCDebug(databaseOptimizer) << "Connection pool size setting not implemented:" << size;
}

// 私有槽
void DatabaseOptimizer::performPeriodicAnalysis()
{
    qCDebug(databaseOptimizer) << "Performing periodic analysis";
    
    analyzeTableStats();
    collectIndexStats();
    
    // 清理旧的查询统计
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-7);
    m_queryStats.erase(
        std::remove_if(m_queryStats.begin(), m_queryStats.end(),
                       [cutoff](const QueryStats &stats) {
                           return stats.timestamp < cutoff;
                       }),
        m_queryStats.end());
    
    qCDebug(databaseOptimizer) << "Periodic analysis completed";
}

void DatabaseOptimizer::cleanupOldStats()
{
    // 清理超过30天的统计数据
    QDateTime cutoff = QDateTime::currentDateTime().addDays(-30);
    
    int oldSize = m_queryStats.size();
    m_queryStats.erase(
        std::remove_if(m_queryStats.begin(), m_queryStats.end(),
                       [cutoff](const QueryStats &stats) {
                           return stats.timestamp < cutoff;
                       }),
        m_queryStats.end());
    
    int removed = oldSize - m_queryStats.size();
    if (removed > 0) {
        qCInfo(databaseOptimizer) << "Cleaned up" << removed << "old query statistics";
    }
}

// 私有方法
QString DatabaseOptimizer::normalizeQuery(const QString &sql)
{
    QString normalized = sql.simplified().toUpper();
    
    // 移除具体的数值，用占位符替代
    normalized.replace(QRegularExpression("\\b\\d+\\b"), "?");
    normalized.replace(QRegularExpression("'[^']*'"), "?");
    normalized.replace(QRegularExpression("\"[^\"]*\""), "?");
    
    return normalized;
}

QString DatabaseOptimizer::generateCacheKey(const QString &sql, const QVariantMap &parameters)
{
    QString combined = sql;
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        combined += QString(":%1=%2").arg(it.key(), it.value().toString());
    }
    
    QCryptographicHash hash(QCryptographicHash::Md5);
    hash.addData(combined.toUtf8());
    return QString(hash.result().toHex());
}

bool DatabaseOptimizer::isSelectQuery(const QString &sql)
{
    return sql.trimmed().toUpper().startsWith("SELECT");
}

bool DatabaseOptimizer::isModifyingQuery(const QString &sql)
{
    QString upper = sql.trimmed().toUpper();
    return upper.startsWith("INSERT") || upper.startsWith("UPDATE") || 
           upper.startsWith("DELETE") || upper.startsWith("REPLACE");
}

QStringList DatabaseOptimizer::extractTables(const QString &sql)
{
    QStringList tables;
    QRegularExpression fromRegex("FROM\\s+(\\w+)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpression joinRegex("JOIN\\s+(\\w+)", QRegularExpression::CaseInsensitiveOption);
    
    QRegularExpressionMatchIterator fromIt = fromRegex.globalMatch(sql);
    while (fromIt.hasNext()) {
        QRegularExpressionMatch match = fromIt.next();
        tables.append(match.captured(1));
    }
    
    QRegularExpressionMatchIterator joinIt = joinRegex.globalMatch(sql);
    while (joinIt.hasNext()) {
        QRegularExpressionMatch match = joinIt.next();
        tables.append(match.captured(1));
    }
    
    tables.removeDuplicates();
    return tables;
}

QStringList DatabaseOptimizer::extractColumns(const QString &sql)
{
    QStringList columns;
    QRegularExpression selectRegex("SELECT\\s+(.+?)\\s+FROM", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = selectRegex.match(sql);
    
    if (match.hasMatch()) {
        QString selectClause = match.captured(1);
        QStringList parts = selectClause.split(',');
        
        for (QString part : parts) {
            part = part.trimmed();
            if (part != "*" && !part.contains('(')) {
                // 移除表别名
                if (part.contains('.')) {
                    part = part.split('.').last();
                }
                columns.append(part);
            }
        }
    }
    
    return columns;
}

QStringList DatabaseOptimizer::extractWhereColumns(const QString &sql)
{
    QStringList columns;
    QRegularExpression whereRegex("WHERE\\s+(.+?)(?:\\s+(?:GROUP|ORDER|LIMIT|$))", 
                                  QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = whereRegex.match(sql);
    
    if (match.hasMatch()) {
        QString whereClause = match.captured(1);
        QRegularExpression columnRegex("(\\w+)\\s*[=<>!]", QRegularExpression::CaseInsensitiveOption);
        
        QRegularExpressionMatchIterator it = columnRegex.globalMatch(whereClause);
        while (it.hasNext()) {
            QRegularExpressionMatch colMatch = it.next();
            QString column = colMatch.captured(1);
            if (column.toUpper() != "AND" && column.toUpper() != "OR") {
                columns.append(column);
            }
        }
    }
    
    columns.removeDuplicates();
    return columns;
}

double DatabaseOptimizer::calculateSelectivity(const QString &table, const QString &column)
{
    QSqlQuery query(m_db);
    query.prepare(QString("SELECT COUNT(DISTINCT %1) / COUNT(*) FROM %2").arg(column, table));
    
    if (query.exec() && query.next()) {
        return query.value(0).toDouble();
    }
    
    return 0.0;
}

bool DatabaseOptimizer::shouldCreateIndex(const QString &table, const QStringList &columns)
{
    if (columns.isEmpty()) {
        return false;
    }
    
    // 检查是否已经存在索引
    QString indexName = generateIndexName(table, columns);
    if (m_indexes.contains(indexName)) {
        return false;
    }
    
    // 计算选择性
    for (const QString &column : columns) {
        double selectivity = calculateSelectivity(table, column);
        if (selectivity < 0.1) { // 选择性太低
            return false;
        }
    }
    
    return true;
}

bool DatabaseOptimizer::shouldDropIndex(const IndexInfo &index)
{
    // 如果索引很久没有使用，建议删除
    if (index.lastUsed.isValid()) {
        QDateTime cutoff = QDateTime::currentDateTime().addDays(-30);
        if (index.lastUsed < cutoff && index.usage < 10) {
            return true;
        }
    }
    
    return false;
}

QString DatabaseOptimizer::generateIndexName(const QString &table, const QStringList &columns)
{
    return QString("idx_%1_%2").arg(table, columns.join("_"));
}

QString DatabaseOptimizer::optimizeSelectQuery(const QString &sql)
{
    QString optimized = sql;
    
    // 避免SELECT *
    if (optimized.contains("SELECT *")) {
        qCDebug(databaseOptimizer) << "Query uses SELECT *, consider specifying columns";
    }
    
    return optimized;
}

QString DatabaseOptimizer::optimizeJoinQuery(const QString &sql)
{
    QString optimized = sql;
    
    // TODO: 优化JOIN查询
    // 1. 确保JOIN条件使用索引
    // 2. 重新排列JOIN顺序
    // 3. 使用合适的JOIN类型
    
    return optimized;
}

QString DatabaseOptimizer::optimizeWhereClause(const QString &sql)
{
    QString optimized = sql;
    
    // TODO: 优化WHERE子句
    // 1. 重新排列条件顺序
    // 2. 使用索引友好的条件
    // 3. 避免函数调用在WHERE中
    
    return optimized;
}

QString DatabaseOptimizer::optimizeOrderBy(const QString &sql)
{
    QString optimized = sql;
    
    // TODO: 优化ORDER BY子句
    // 1. 确保ORDER BY列有索引
    // 2. 考虑使用LIMIT优化
    
    return optimized;
}

QString DatabaseOptimizer::addQueryHints(const QString &sql)
{
    QString hinted = sql;
    
    // TODO: 添加MySQL查询提示
    // 例如: USE INDEX, FORCE INDEX等
    
    return hinted;
}

void DatabaseOptimizer::updateQueryCacheStats()
{
    // TODO: 更新查询缓存统计信息
}

void DatabaseOptimizer::evictLeastUsedCache()
{
    if (m_cacheManager) {
        // 清理最少使用的查询缓存
        m_cacheManager->clearCategory("query_cache");
    }
}

bool DatabaseOptimizer::shouldCacheQuery(const QString &sql, qint64 executionTime)
{
    // 缓存执行时间较长的SELECT查询
    return isSelectQuery(sql) && executionTime > 100; // 100ms以上的查询
}

void DatabaseOptimizer::collectTableStats(const QString &table)
{
    QSqlQuery query(m_db);
    query.prepare("SELECT COUNT(*) FROM " + table);
    
    if (query.exec() && query.next()) {
        int rowCount = query.value(0).toInt();
        qCDebug(databaseOptimizer) << "Table" << table << "has" << rowCount << "rows";
    }
}

void DatabaseOptimizer::collectIndexStats()
{
    QSqlQuery query(m_db);
    query.prepare("SELECT DISTINCT table_name, index_name, column_name, non_unique "
                  "FROM information_schema.statistics "
                  "WHERE table_schema = ? AND index_name != 'PRIMARY'");
    query.addBindValue(m_db.databaseName());
    
    if (query.exec()) {
        QHash<QString, IndexInfo> newIndexes;
        
        while (query.next()) {
            QString tableName = query.value(0).toString();
            QString indexName = query.value(1).toString();
            QString columnName = query.value(2).toString();
            bool isUnique = !query.value(3).toBool();
            
            if (!newIndexes.contains(indexName)) {
                IndexInfo info;
                info.name = indexName;
                info.table = tableName;
                info.type = BTreeIndex; // 默认类型
                info.isUnique = isUnique;
                info.createdAt = QDateTime::currentDateTime();
                info.usage = 0;
                newIndexes[indexName] = info;
            }
            
            newIndexes[indexName].columns.append(columnName);
        }
        
        m_indexes = newIndexes;
        qCDebug(databaseOptimizer) << "Collected" << m_indexes.size() << "index statistics";
    }
}

void DatabaseOptimizer::collectQueryPlanStats(const QString &sql)
{
    QSqlQuery query(m_db);
    query.prepare("EXPLAIN " + sql);
    
    if (query.exec()) {
        while (query.next()) {
            // TODO: 分析查询执行计划
            qCDebug(databaseOptimizer) << "Query plan:" << query.record();
        }
    }
}

QVariantMap DatabaseOptimizer::analyzeQueryPlan(const QString &sql)
{
    QVariantMap plan;
    
    QSqlQuery query(m_db);
    query.prepare("EXPLAIN FORMAT=JSON " + sql);
    
    if (query.exec() && query.next()) {
        QString jsonPlan = query.value(0).toString();
        QJsonDocument doc = QJsonDocument::fromJson(jsonPlan.toUtf8());
        plan = doc.object().toVariantMap();
    }
    
    return plan;
}

QStringList DatabaseOptimizer::identifyBottlenecks()
{
    QStringList bottlenecks;
    
    // 分析慢查询
    QList<QueryStats> slowQueries = getSlowQueries(10);
    for (const QueryStats &stats : slowQueries) {
        bottlenecks.append(QString("Slow query: %1 (%2ms)")
                          .arg(stats.query.left(50))
                          .arg(stats.executionTime));
    }
    
    // 分析表大小
    QVariantMap tableSizes = getTableSizes();
    for (auto it = tableSizes.begin(); it != tableSizes.end(); ++it) {
        if (it.value().toDouble() > 1000) { // 大于1GB的表
            bottlenecks.append(QString("Large table: %1 (%2 MB)")
                              .arg(it.key())
                              .arg(it.value().toDouble()));
        }
    }
    
    return bottlenecks;
}

QVariantMap DatabaseOptimizer::generateOptimizationReport()
{
    QVariantMap report;
    
    report["performance_metrics"] = getPerformanceMetrics();
    report["slow_queries"] = QVariant::fromValue(getSlowQueries(10));
    report["index_suggestions"] = suggestIndexes("SELECT * FROM users WHERE username = ?");
    report["drop_suggestions"] = suggestDropIndexes();
    report["bottlenecks"] = identifyBottlenecks();
    report["database_size"] = getDatabaseSize();
    report["table_sizes"] = getTableSizes();
    
    return report;
}

void DatabaseOptimizer::updateStatistics()
{
    analyze();
}

void DatabaseOptimizer::rebuildIndexes()
{
    reindex();
}

void DatabaseOptimizer::defragmentTables()
{
    vacuum();
}

void DatabaseOptimizer::purgeOldData()
{
    // TODO: 实现旧数据清理逻辑
    qCDebug(databaseOptimizer) << "Old data purging not implemented";
}

bool DatabaseOptimizer::executeQuery(QSqlQuery &query, const QString &sql, const QVariantMap &parameters)
{
    Q_UNUSED(sql)
    Q_UNUSED(parameters)
    
    return query.exec();
}

QVariantList DatabaseOptimizer::queryToVariantList(QSqlQuery &query)
{
    QVariantList result;
    
    while (query.next()) {
        QVariantMap row;
        QSqlRecord record = query.record();
        
        for (int i = 0; i < record.count(); ++i) {
            row[record.fieldName(i)] = query.value(i);
        }
        
        result.append(row);
    }
    
    return result;
}

QString DatabaseOptimizer::formatExecutionTime(qint64 milliseconds)
{
    if (milliseconds < 1000) {
        return QString("%1ms").arg(milliseconds);
    } else if (milliseconds < 60000) {
        return QString("%1.%2s").arg(milliseconds / 1000).arg((milliseconds % 1000) / 100);
    } else {
        int minutes = milliseconds / 60000;
        int seconds = (milliseconds % 60000) / 1000;
        return QString("%1m %2s").arg(minutes).arg(seconds);
    }
}

qint64 DatabaseOptimizer::getCurrentTimestamp()
{
    return QDateTime::currentMSecsSinceEpoch();
} 