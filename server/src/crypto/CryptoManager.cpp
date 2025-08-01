#include "CryptoManager.h"

#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>

// 静态实例初始化
CryptoManager* CryptoManager::m_instance = nullptr;

CryptoManager::CryptoManager(QObject *parent)
    : QObject(parent)
{
    // 初始化加密相关资源
}

CryptoManager::~CryptoManager()
{
    // 清理加密相关资源
}

CryptoManager* CryptoManager::instance()
{
    if (!m_instance) {
        m_instance = new CryptoManager();
    }
    return m_instance;
}

QByteArray CryptoManager::encryptAES(const QByteArray &data, const QByteArray &key)
{
    // 实现AES加密
    // 注意：这里只是一个简单的实现，实际应用中应该使用更安全的方法
    // 可以使用Qt的QCA库或OpenSSL库实现完整的AES加密
    
    // 简单异或加密作为示例
    QByteArray result = data;
    for (int i = 0; i < result.size(); ++i) {
        result[i] = result[i] ^ key[i % key.size()];
    }
    return result;
}

QByteArray CryptoManager::decryptAES(const QByteArray &data, const QByteArray &key)
{
    // 对于简单的异或加密，解密操作与加密相同
    return encryptAES(data, key);
}

QByteArray CryptoManager::generateRandomKey(int length)
{
    QByteArray key;
    key.resize(length);
    
    // 使用Qt的随机数生成器填充随机字节
    for (int i = 0; i < length; ++i) {
        key[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    
    return key;
}

QByteArray CryptoManager::hash(const QByteArray &data, const QString &algorithm)
{
    QCryptographicHash::Algorithm hashAlgorithm;
    
    // 选择哈希算法
    if (algorithm == "MD5") {
        hashAlgorithm = QCryptographicHash::Md5;
    } else if (algorithm == "SHA-1") {
        hashAlgorithm = QCryptographicHash::Sha1;
    } else if (algorithm == "SHA-224") {
        hashAlgorithm = QCryptographicHash::Sha224;
    } else if (algorithm == "SHA-384") {
        hashAlgorithm = QCryptographicHash::Sha384;
    } else if (algorithm == "SHA-512") {
        hashAlgorithm = QCryptographicHash::Sha512;
    } else {
        // 默认使用SHA-256
        hashAlgorithm = QCryptographicHash::Sha256;
    }
    
    return QCryptographicHash::hash(data, hashAlgorithm);
}

bool CryptoManager::verifyPassword(const QString &password, const QString &hashedPassword, const QString &salt)
{
    // 使用相同的盐值和哈希算法处理输入的密码
    QByteArray passwordData = password.toUtf8();
    QByteArray saltData = salt.toUtf8();
    
    // 将密码和盐值组合
    QByteArray combined = passwordData + saltData;
    
    // 计算哈希
    QByteArray hashedInput = hash(combined);
    
    // 比较哈希值
    return (hashedInput.toHex() == hashedPassword);
}

QPair<QString, QString> CryptoManager::hashPassword(const QString &password, const QString &salt)
{
    QString actualSalt = salt;
    
    // 如果没有提供盐值，则生成一个新的盐值
    if (actualSalt.isEmpty()) {
        // 使用当前时间和随机数生成盐值
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        int random = QRandomGenerator::global()->bounded(1000000);
        actualSalt = QString::number(now) + QString::number(random);
        actualSalt = QString::fromUtf8(hash(actualSalt.toUtf8()).toHex());
    }
    
    // 将密码和盐值组合
    QByteArray combined = password.toUtf8() + actualSalt.toUtf8();
    
    // 计算哈希
    QByteArray hashedPassword = hash(combined);
    
    // 返回哈希后的密码和盐值
    return qMakePair(QString::fromUtf8(hashedPassword.toHex()), actualSalt);
}