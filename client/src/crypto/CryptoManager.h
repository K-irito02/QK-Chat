#ifndef CRYPTOMANAGER_H
#define CRYPTOMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVariantMap>
#include <QDateTime>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(cryptoManager)

class CryptoManager : public QObject
{
    Q_OBJECT

public:
    // 加密算法类型
    enum EncryptionType {
        None = 0,           // 无加密
        AES256 = 1,         // AES-256加密
        RSA2048 = 2,        // RSA-2048加密
        ECC_P256 = 3        // ECC P-256加密
    };
    Q_ENUM(EncryptionType)

    // 密钥对结构
    struct KeyPair {
        QByteArray publicKey;
        QByteArray privateKey;
        EncryptionType type;
        QDateTime createdAt;
        QDateTime expiresAt;
    };

    // 加密消息结构
    struct EncryptedMessage {
        QByteArray encryptedData;    // 加密后的数据
        QByteArray signature;        // 数字签名
        QByteArray iv;               // 初始化向量
        EncryptionType algorithm;    // 使用的算法
        QString keyId;               // 密钥ID
        qint64 timestamp;            // 时间戳
    };

    explicit CryptoManager(QObject *parent = nullptr);
    ~CryptoManager();

    // 初始化和配置
    bool initialize();
    void setDefaultEncryption(EncryptionType type);
    EncryptionType getDefaultEncryption() const;

    // 密钥管理
    KeyPair generateKeyPair(EncryptionType type = AES256);
    bool storeKeyPair(const QString &keyId, const KeyPair &keyPair);
    KeyPair getKeyPair(const QString &keyId);
    QStringList getStoredKeyIds();
    bool deleteKeyPair(const QString &keyId);
    void loadStoredKeyPairs();
    
    // 公钥交换
    QByteArray getPublicKey(const QString &keyId = QString());
    bool storeContactPublicKey(qint64 contactId, const QByteArray &publicKey, EncryptionType type);
    QByteArray getContactPublicKey(qint64 contactId);
    bool verifyPublicKey(const QByteArray &publicKey, const QByteArray &signature);

    // 消息加密/解密
    EncryptedMessage encryptMessage(const QString &plainText, qint64 receiverId, const QString &keyId = QString());
    QString decryptMessage(const EncryptedMessage &encryptedMessage, qint64 senderId);
    
    // 群组消息加密/解密
    EncryptedMessage encryptGroupMessage(const QString &plainText, qint64 groupId, const QString &keyId = QString());
    QString decryptGroupMessage(const EncryptedMessage &encryptedMessage, qint64 groupId, qint64 senderId);
    
    // 文件加密/解密
    bool encryptFile(const QString &inputPath, const QString &outputPath, const QString &keyId = QString());
    bool decryptFile(const QString &inputPath, const QString &outputPath, const EncryptedMessage &metadata);
    
    // 数字签名
    QByteArray signData(const QByteArray &data, const QString &keyId = QString());
    bool verifySignature(const QByteArray &data, const QByteArray &signature, qint64 senderId);
    
    // 密钥交换协议（ECDH）
    QByteArray generateSharedSecret(const QByteArray &myPrivateKey, const QByteArray &theirPublicKey);
    QByteArray deriveSessionKey(const QByteArray &sharedSecret, const QByteArray &salt);
    
    // 前向保密
    bool rotateKeys(const QString &keyId);
    void enableForwardSecrecy(bool enable);
    bool isForwardSecrecyEnabled() const;
    
    // 工具函数
    QByteArray generateSalt(int size = 32);
    QByteArray generateIV(int size = 16);
    QString encryptedMessageToJson(const EncryptedMessage &message);
    EncryptedMessage encryptedMessageFromJson(const QString &json);
    bool isEncryptionSupported(EncryptionType type);

signals:
    void keyPairGenerated(const QString &keyId);
    void keyPairDeleted(const QString &keyId);
    void encryptionError(const QString &error);

private:
    // 加密算法实现
    QByteArray encryptAES256(const QByteArray &data, const QByteArray &key, const QByteArray &iv);
    QByteArray decryptAES256(const QByteArray &encryptedData, const QByteArray &key, const QByteArray &iv);
    
    // 密钥生成
    KeyPair generateRSAKeyPair();
    KeyPair generateECCKeyPair();
    KeyPair generateAESKey();
    
    // RSA加密/解密
    QByteArray encryptRSA(const QByteArray &data, const QByteArray &publicKey);
    QByteArray decryptRSA(const QByteArray &encryptedData, const QByteArray &privateKey);
    
    // 数字签名
    QByteArray signRSA(const QByteArray &data, const QByteArray &privateKey);
    bool verifyRSA(const QByteArray &data, const QByteArray &signature, const QByteArray &publicKey);
    
    QByteArray signECC(const QByteArray &data, const QByteArray &privateKey);
    bool verifyECC(const QByteArray &data, const QByteArray &signature, const QByteArray &publicKey);
    
    // 密钥派生
    QByteArray deriveKey(const QByteArray &password, const QByteArray &salt, int iterations = 10000);
    QByteArray hashData(const QByteArray &data);
    
    // 存储管理
    bool saveKeyPairToStorage(const QString &keyId, const KeyPair &keyPair);
    KeyPair loadKeyPairFromStorage(const QString &keyId);
    bool deleteKeyPairFromStorage(const QString &keyId);
    
    // 联系人公钥管理
    void saveContactPublicKey(qint64 contactId, const QByteArray &publicKey);
    QByteArray loadContactPublicKey(qint64 contactId);
    
    // 验证函数
    bool validateKeyPair(const KeyPair &keyPair);
    bool validateEncryptedMessage(const EncryptedMessage &message);
    QString generateKeyId();
    
    // 成员变量
    QMap<QString, KeyPair> _keyPairs;
    QMap<qint64, QByteArray> _contactPublicKeys;
    EncryptionType _defaultEncryption;
    bool _forwardSecrecyEnabled;
    QString _storagePath;
};

Q_DECLARE_METATYPE(CryptoManager::KeyPair)
Q_DECLARE_METATYPE(CryptoManager::EncryptedMessage)

#endif // CRYPTOMANAGER_H 