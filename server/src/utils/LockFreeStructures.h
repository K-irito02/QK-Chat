#ifndef LOCKFREESTRUCTURES_H
#define LOCKFREESTRUCTURES_H

#include <QAtomicPointer>
#include <QAtomicInt>
#include <QSharedPointer>
#include <QWeakPointer>
#include <QHash>
#include <QMutex>
#include <QReadWriteLock>
#include <QLoggingCategory>
#include <memory>
#include <functional>

Q_DECLARE_LOGGING_CATEGORY(lockFree)

/**
 * @brief 无锁单向链表节点
 */
template<typename T>
struct LockFreeNode {
    QAtomicPointer<LockFreeNode<T>> next{nullptr};
    T data;
    QAtomicInt refCount{1};
    
    explicit LockFreeNode(const T& value) : data(value) {}
    explicit LockFreeNode(T&& value) : data(std::move(value)) {}
    
    void addRef() { refCount.fetchAndAddOrdered(1); }
    void release() {
        if (refCount.fetchAndSubOrdered(1) == 1) {
            delete this;
        }
    }
};

/**
 * @brief 无锁队列实现
 * 基于Michael & Scott算法的无锁队列
 */
template<typename T>
class LockFreeQueue
{
public:
    LockFreeQueue() {
        auto dummy = new LockFreeNode<T>(T{});
        m_head.storeRelease(dummy);
        m_tail.storeRelease(dummy);
    }
    
    ~LockFreeQueue() {
        while (auto node = m_head.loadAcquire()) {
            m_head.storeRelease(node->next.loadAcquire());
            node->release();
        }
    }
    
    void enqueue(const T& item) {
        auto newNode = new LockFreeNode<T>(item);
        
        while (true) {
            auto tail = m_tail.loadAcquire();
            auto next = tail->next.loadAcquire();
            
            if (tail == m_tail.loadAcquire()) {
                if (next == nullptr) {
                    if (tail->next.testAndSetOrdered(nullptr, newNode)) {
                        m_tail.testAndSetOrdered(tail, newNode);
                        m_size.fetchAndAddOrdered(1);
                        break;
                    }
                } else {
                    m_tail.testAndSetOrdered(tail, next);
                }
            }
        }
    }
    
    bool dequeue(T& result) {
        while (true) {
            auto head = m_head.loadAcquire();
            auto tail = m_tail.loadAcquire();
            auto next = head->next.loadAcquire();
            
            if (head == m_head.loadAcquire()) {
                if (head == tail) {
                    if (next == nullptr) {
                        return false; // 队列为空
                    }
                    m_tail.testAndSetOrdered(tail, next);
                } else {
                    if (next == nullptr) {
                        continue;
                    }
                    result = next->data;
                    if (m_head.testAndSetOrdered(head, next)) {
                        head->release();
                        m_size.fetchAndSubOrdered(1);
                        return true;
                    }
                }
            }
        }
    }
    
    bool empty() const {
        return size() == 0;
    }
    
    int size() const {
        return m_size.loadAcquire();
    }

private:
    QAtomicPointer<LockFreeNode<T>> m_head;
    QAtomicPointer<LockFreeNode<T>> m_tail;
    QAtomicInt m_size{0};
};

/**
 * @brief 原子计数器集合
 * 用于统计各种指标，避免锁竞争
 */
class AtomicCounters
{
public:
    void increment(const QString& key) {
        QMutexLocker locker(&m_mutex);
        if (!m_counters.contains(key)) {
            m_counters[key] = std::make_shared<QAtomicInt>(0);
        }
        m_counters[key]->fetchAndAddOrdered(1);
    }
    
    void add(const QString& key, int value) {
        QMutexLocker locker(&m_mutex);
        if (!m_counters.contains(key)) {
            m_counters[key] = std::make_shared<QAtomicInt>(0);
        }
        m_counters[key]->fetchAndAddOrdered(value);
    }
    
    int get(const QString& key) const {
        QMutexLocker locker(&m_mutex);
        auto it = m_counters.find(key);
        return (it != m_counters.end()) ? it.value()->loadAcquire() : 0;
    }
    
    void reset(const QString& key) {
        QMutexLocker locker(&m_mutex);
        auto it = m_counters.find(key);
        if (it != m_counters.end()) {
            it.value()->storeRelease(0);
        }
    }
    
    void resetAll() {
        QMutexLocker locker(&m_mutex);
        for (auto& counter : m_counters) {
            counter->storeRelease(0);
        }
    }
    
    QStringList keys() const {
        QMutexLocker locker(&m_mutex);
        return m_counters.keys();
    }

private:
    mutable QMutex m_mutex;
    QHash<QString, std::shared_ptr<QAtomicInt>> m_counters;
};

/**
 * @brief 读写分离的连接管理器
 * 使用读写锁优化并发访问
 */
template<typename Key, typename Value>
class ConcurrentMap
{
public:
    void insert(const Key& key, const Value& value) {
        QWriteLocker locker(&m_lock);
        m_data[key] = value;
        m_version.fetchAndAddOrdered(1);
    }
    
    bool remove(const Key& key) {
        QWriteLocker locker(&m_lock);
        bool removed = m_data.remove(key) > 0;
        if (removed) {
            m_version.fetchAndAddOrdered(1);
        }
        return removed;
    }
    
    Value value(const Key& key, const Value& defaultValue = Value{}) const {
        QReadLocker locker(&m_lock);
        return m_data.value(key, defaultValue);
    }
    
    bool contains(const Key& key) const {
        QReadLocker locker(&m_lock);
        return m_data.contains(key);
    }
    
    QList<Key> keys() const {
        QReadLocker locker(&m_lock);
        return m_data.keys();
    }
    
    QList<Value> values() const {
        QReadLocker locker(&m_lock);
        return m_data.values();
    }
    
    int size() const {
        QReadLocker locker(&m_lock);
        return m_data.size();
    }
    
    bool empty() const {
        return size() == 0;
    }
    
    void clear() {
        QWriteLocker locker(&m_lock);
        m_data.clear();
        m_version.fetchAndAddOrdered(1);
    }
    
    // 批量操作，减少锁获取次数
    void insertBatch(const QHash<Key, Value>& batch) {
        QWriteLocker locker(&m_lock);
        for (auto it = batch.begin(); it != batch.end(); ++it) {
            m_data[it.key()] = it.value();
        }
        m_version.fetchAndAddOrdered(1);
    }
    
    QHash<Key, Value> snapshot() const {
        QReadLocker locker(&m_lock);
        return m_data;
    }
    
    // 版本号，用于检测数据变化
    int version() const {
        return m_version.loadAcquire();
    }
    
    // 安全遍历，避免在遍历过程中数据被修改
    template<typename Func>
    void forEach(Func func) const {
        QReadLocker locker(&m_lock);
        for (auto it = m_data.begin(); it != m_data.end(); ++it) {
            func(it.key(), it.value());
        }
    }

private:
    mutable QReadWriteLock m_lock;
    QHash<Key, Value> m_data;
    QAtomicInt m_version{0};
};

/**
 * @brief 无锁状态机
 * 用于管理连接状态等场景
 */
template<typename StateType>
class AtomicStateMachine
{
public:
    explicit AtomicStateMachine(StateType initialState) 
        : m_state(static_cast<int>(initialState)) {}
    
    StateType currentState() const {
        return static_cast<StateType>(m_state.loadAcquire());
    }
    
    bool compareAndSwap(StateType expected, StateType desired) {
        return m_state.testAndSetOrdered(static_cast<int>(expected), 
                                        static_cast<int>(desired));
    }
    
    StateType exchange(StateType newState) {
        return static_cast<StateType>(m_state.fetchAndStoreOrdered(static_cast<int>(newState)));
    }
    
    // 条件状态转换
    bool transitionIf(StateType from, StateType to) {
        return compareAndSwap(from, to);
    }
    
    // 批量状态检查
    bool isOneOf(std::initializer_list<StateType> states) const {
        StateType current = currentState();
        for (StateType state : states) {
            if (current == state) return true;
        }
        return false;
    }

private:
    QAtomicInt m_state;
};

#endif // LOCKFREESTRUCTURES_H
