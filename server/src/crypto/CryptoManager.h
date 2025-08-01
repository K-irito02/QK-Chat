#ifndef CRYPTOMANAGER_H
#define CRYPTOMANAGER_H

#include <QObject>
#include <QString>
#include <QByteArray>

/**
 * @brief 加密管理器类，负责处理服务器端的加密和解密操作
 */
class CryptoManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取CryptoManager的单例实例
     * @return CryptoManager的单例实例
     */
    static CryptoManager *instance();

    /**
     * @brief 使用AES加密数据
     * @param data 要加密的数据
     * @param key 加密密钥
     * @return 加密后的数据
     */
    QByteArray encryptAES(const QByteArray &data, const QByteArray &key);

    /**
     * @brief 使用AES解密数据
     * @param data 要解密的数据
     * @param key 解密密钥
     * @return 解密后的数据
     */
    QByteArray decryptAES(const QByteArray &data, const QByteArray &key);

    /**
     * @brief 生成随机密钥
     * @param length 密钥长度
     * @return 生成的随机密钥
     */
    QByteArray generateRandomKey(int length = 32);

    /**
     * @brief 计算字符串的哈希值
     * @param data 要计算哈希的数据
     * @param algorithm 哈希算法（默认为SHA-256）
     * @return 计算得到的哈希值
     */
    QByteArray hash(const QByteArray &data, const QString &algorithm = "SHA-256");

    /**
     * @brief 验证密码
     * @param password 输入的密码
     * @param hashedPassword 存储的哈希密码
     * @param salt 盐值
     * @return 密码是否匹配
     */
    bool verifyPassword(const QString &password, const QString &hashedPassword, const QString &salt);

    /**
     * @brief 生成密码哈希
     * @param password 原始密码
     * @param salt 盐值（如果为空则自动生成）
     * @return 哈希后的密码和盐值的组合
     */
    QPair<QString, QString> hashPassword(const QString &password, const QString &salt = QString());

private:
    explicit CryptoManager(QObject *parent = nullptr);
    ~CryptoManager();

    static CryptoManager *m_instance;
};

#endif // CRYPTOMANAGER_H