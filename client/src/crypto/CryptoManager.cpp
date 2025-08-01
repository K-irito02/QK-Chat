#include "CryptoManager.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDateTime>
#include <QSettings>
#include <QUuid>

Q_LOGGING_CATEGORY(cryptoManager, "qkchat.client.cryptomanager")

CryptoManager::CryptoManager(QObject *parent)
    : QObject(parent)
    , _defaultEncryption(AES256)
    , _forwardSecrecyEnabled(false)
{
    _storagePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/crypto";
    
    qCDebug(cryptoManager) << "CryptoManager created";
}

CryptoManager::~CryptoManager()
{
    qCDebug(cryptoManager) << "CryptoManager destroyed";
}

bool CryptoManager::initialize()
{
    qCDebug(cryptoManager) << "Initializing CryptoManager";
    
    // 创建存储目录
    QDir dir(_storagePath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    // 加载已存储的密钥对
    loadStoredKeyPairs();
    
    // 如果没有默认密钥，生成一个
    if (_keyPairs.isEmpty()) {
        KeyPair defaultKey = generateKeyPair(_defaultEncryption);
        QString defaultKeyId = generateKeyId();
        storeKeyPair(defaultKeyId, defaultKey);
    }
    
    qCDebug(cryptoManager) << "CryptoManager initialized successfully";
    return true;
}

void CryptoManager::setDefaultEncryption(EncryptionType type)
{
    _defaultEncryption = type;
    qCDebug(cryptoManager) << "Default encryption set to:" << type;
}

CryptoManager::EncryptionType CryptoManager::getDefaultEncryption() const
{
    return _defaultEncryption;
}

CryptoManager::KeyPair CryptoManager::generateKeyPair(EncryptionType type)
{
    qCDebug(cryptoManager) << "Generating key pair with type:" << type;
    
    KeyPair keyPair;
    keyPair.type = type;
    keyPair.createdAt = QDateTime::currentDateTime();
    keyPair.expiresAt = keyPair.createdAt.addDays(365); // 1年有效期
    
    switch (type) {
    case RSA2048:
        keyPair = generateRSAKeyPair();
        break;
    case ECC_P256:
        keyPair = generateECCKeyPair();
        break;
    case AES256:
    default:
        keyPair = generateAESKey();
        break;
    }
    
    qCDebug(cryptoManager) << "Key pair generated successfully";
    return keyPair;
}

bool CryptoManager::storeKeyPair(const QString &keyId, const KeyPair &keyPair)
{
    if (keyId.isEmpty()) {
        qCWarning(cryptoManager) << "Cannot store key pair with empty keyId";
        return false;
    }
    
    if (!validateKeyPair(keyPair)) {
        qCWarning(cryptoManager) << "Invalid key pair for storage";
        return false;
    }
    
    _keyPairs[keyId] = keyPair;
    bool success = saveKeyPairToStorage(keyId, keyPair);
    
    if (success) {
        qCDebug(cryptoManager) << "Key pair stored successfully:" << keyId;
        emit keyPairGenerated(keyId);
    } else {
        qCWarning(cryptoManager) << "Failed to store key pair:" << keyId;
    }
    
    return success;
}

CryptoManager::KeyPair CryptoManager::getKeyPair(const QString &keyId)
{
    if (keyId.isEmpty()) {
        return _keyPairs.value(_keyPairs.keys().first());
    }
    return _keyPairs.value(keyId);
}

QStringList CryptoManager::getStoredKeyIds()
{
    return _keyPairs.keys();
}

bool CryptoManager::deleteKeyPair(const QString &keyId)
{
    if (keyId.isEmpty()) {
        qCWarning(cryptoManager) << "Cannot delete key pair with empty keyId";
        return false;
    }
    
    if (!_keyPairs.contains(keyId)) {
        qCWarning(cryptoManager) << "Key pair not found:" << keyId;
        return false;
    }
    
    _keyPairs.remove(keyId);
    bool success = deleteKeyPairFromStorage(keyId);
    
    if (success) {
        qCDebug(cryptoManager) << "Key pair deleted successfully:" << keyId;
        emit keyPairDeleted(keyId);
    } else {
        qCWarning(cryptoManager) << "Failed to delete key pair:" << keyId;
    }
    
    return success;
}

void CryptoManager::loadStoredKeyPairs()
{
    qCDebug(cryptoManager) << "Loading stored key pairs";
    
    QDir dir(_storagePath);
    QStringList keyFiles = dir.entryList(QStringList() << "*.key", QDir::Files);
    
    for (const QString &keyFile : keyFiles) {
        QString keyId = keyFile.chopped(4); // 移除 .key 扩展名
        KeyPair keyPair = loadKeyPairFromStorage(keyId);
        
        if (validateKeyPair(keyPair)) {
            _keyPairs[keyId] = keyPair;
            qCDebug(cryptoManager) << "Loaded key pair:" << keyId;
        } else {
            qCWarning(cryptoManager) << "Invalid key pair found:" << keyId;
        }
    }
    
    qCDebug(cryptoManager) << "Loaded" << _keyPairs.size() << "key pairs";
}

// 公钥交换
QByteArray CryptoManager::getPublicKey(const QString &keyId)
{
    KeyPair keyPair = getKeyPair(keyId);
    return keyPair.publicKey;
}

bool CryptoManager::storeContactPublicKey(qint64 contactId, const QByteArray &publicKey, EncryptionType type)
{
    if (publicKey.isEmpty()) {
        qCWarning(cryptoManager) << "Cannot store empty public key";
        return false;
    }
    
    _contactPublicKeys[contactId] = publicKey;
    saveContactPublicKey(contactId, publicKey);
    
    qCDebug(cryptoManager) << "Contact public key stored for contact:" << contactId;
    return true;
}

QByteArray CryptoManager::getContactPublicKey(qint64 contactId)
{
    QByteArray publicKey = _contactPublicKeys.value(contactId);
    if (publicKey.isEmpty()) {
        publicKey = loadContactPublicKey(contactId);
        if (!publicKey.isEmpty()) {
            _contactPublicKeys[contactId] = publicKey;
        }
    }
    return publicKey;
}

bool CryptoManager::verifyPublicKey(const QByteArray &publicKey, const QByteArray &signature)
{
    // 简单的公钥验证（实际应用中需要更复杂的验证）
    return !publicKey.isEmpty() && !signature.isEmpty();
}

// 消息加密/解密
CryptoManager::EncryptedMessage CryptoManager::encryptMessage(const QString &plainText, qint64 receiverId, const QString &keyId)
{
    EncryptedMessage message;
    message.algorithm = _defaultEncryption;
    message.keyId = keyId.isEmpty() ? _keyPairs.keys().first() : keyId;
    message.timestamp = QDateTime::currentMSecsSinceEpoch();
    message.iv = generateIV();
    
    QByteArray data = plainText.toUtf8();
    QByteArray receiverPublicKey = getContactPublicKey(receiverId);
    
    if (receiverPublicKey.isEmpty()) {
        qCWarning(cryptoManager) << "No public key found for receiver:" << receiverId;
        return message;
    }
    
    // 根据算法类型进行加密
    switch (_defaultEncryption) {
    case AES256:
        message.encryptedData = encryptAES256(data, receiverPublicKey, message.iv);
        break;
    case RSA2048:
        message.encryptedData = encryptRSA(data, receiverPublicKey);
        break;
    case ECC_P256:
        // ECC加密实现
        message.encryptedData = data; // 临时实现
        break;
    default:
        message.encryptedData = data;
        break;
    }
    
    // 生成数字签名
    message.signature = signData(message.encryptedData, message.keyId);
    
    qCDebug(cryptoManager) << "Message encrypted successfully";
    return message;
}

QString CryptoManager::decryptMessage(const EncryptedMessage &encryptedMessage, qint64 senderId)
{
    if (!validateEncryptedMessage(encryptedMessage)) {
        qCWarning(cryptoManager) << "Invalid encrypted message";
        return QString();
    }
    
    QByteArray senderPublicKey = getContactPublicKey(senderId);
    if (senderPublicKey.isEmpty()) {
        qCWarning(cryptoManager) << "No public key found for sender:" << senderId;
        return QString();
    }
    
    // 验证签名
    if (!verifySignature(encryptedMessage.encryptedData, encryptedMessage.signature, senderId)) {
        qCWarning(cryptoManager) << "Message signature verification failed";
        return QString();
    }
    
    QByteArray decryptedData;
    
    // 根据算法类型进行解密
    switch (encryptedMessage.algorithm) {
    case AES256:
        decryptedData = decryptAES256(encryptedMessage.encryptedData, senderPublicKey, encryptedMessage.iv);
        break;
    case RSA2048:
        decryptedData = decryptRSA(encryptedMessage.encryptedData, senderPublicKey);
        break;
    case ECC_P256:
        // ECC解密实现
        decryptedData = encryptedMessage.encryptedData; // 临时实现
        break;
    default:
        decryptedData = encryptedMessage.encryptedData;
        break;
    }
    
    QString plainText = QString::fromUtf8(decryptedData);
    qCDebug(cryptoManager) << "Message decrypted successfully";
    return plainText;
}

// 群组消息加密/解密
CryptoManager::EncryptedMessage CryptoManager::encryptGroupMessage(const QString &plainText, qint64 groupId, const QString &keyId)
{
    EncryptedMessage message;
    message.algorithm = _defaultEncryption;
    message.keyId = keyId.isEmpty() ? _keyPairs.keys().first() : keyId;
    message.timestamp = QDateTime::currentMSecsSinceEpoch();
    message.iv = generateIV();
    
    QByteArray data = plainText.toUtf8();
    
    // 群组消息使用共享密钥（简化实现）
    QByteArray groupKey = deriveKey(QByteArray::number(groupId), generateSalt());
    
    // 根据算法类型进行加密
    switch (_defaultEncryption) {
    case AES256:
        message.encryptedData = encryptAES256(data, groupKey, message.iv);
        break;
    case RSA2048:
        message.encryptedData = encryptRSA(data, groupKey);
        break;
    case ECC_P256:
        // ECC加密实现
        message.encryptedData = data; // 临时实现
        break;
    default:
        message.encryptedData = data;
        break;
    }
    
    // 生成数字签名
    message.signature = signData(message.encryptedData, message.keyId);
    
    qCDebug(cryptoManager) << "Group message encrypted successfully";
    return message;
}

QString CryptoManager::decryptGroupMessage(const EncryptedMessage &encryptedMessage, qint64 groupId, qint64 senderId)
{
    if (!validateEncryptedMessage(encryptedMessage)) {
        qCWarning(cryptoManager) << "Invalid encrypted group message";
        return QString();
    }
    
    // 群组消息使用共享密钥（简化实现）
    QByteArray groupKey = deriveKey(QByteArray::number(groupId), generateSalt());
    
    QByteArray decryptedData;
    
    // 根据算法类型进行解密
    switch (encryptedMessage.algorithm) {
    case AES256:
        decryptedData = decryptAES256(encryptedMessage.encryptedData, groupKey, encryptedMessage.iv);
        break;
    case RSA2048:
        decryptedData = decryptRSA(encryptedMessage.encryptedData, groupKey);
        break;
    case ECC_P256:
        // ECC解密实现
        decryptedData = encryptedMessage.encryptedData; // 临时实现
        break;
    default:
        decryptedData = encryptedMessage.encryptedData;
        break;
    }
    
    QString plainText = QString::fromUtf8(decryptedData);
    qCDebug(cryptoManager) << "Group message decrypted successfully";
    return plainText;
}

bool CryptoManager::encryptFile(const QString &inputPath, const QString &outputPath, const QString &keyId)
{
    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        qCWarning(cryptoManager) << "Cannot open input file:" << inputPath;
        return false;
    }
    
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        qCWarning(cryptoManager) << "Cannot create output file:" << outputPath;
        return false;
    }
    
    KeyPair key = getKeyPair(keyId);
    QByteArray iv = generateIV();
    
    // 写入IV
    outputFile.write(iv);
    
    // 分块加密文件
    const int blockSize = 4096;
    QByteArray buffer;
    while (!inputFile.atEnd()) {
        buffer = inputFile.read(blockSize);
        QByteArray encryptedBlock = encryptAES256(buffer, key.publicKey, iv);
        outputFile.write(encryptedBlock);
    }
    
    qCDebug(cryptoManager) << "File encrypted successfully:" << inputPath;
    return true;
}

bool CryptoManager::decryptFile(const QString &inputPath, const QString &outputPath, const EncryptedMessage &metadata)
{
    QFile inputFile(inputPath);
    if (!inputFile.open(QIODevice::ReadOnly)) {
        qCWarning(cryptoManager) << "Cannot open input file:" << inputPath;
        return false;
    }
    
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        qCWarning(cryptoManager) << "Cannot create output file:" << outputPath;
        return false;
    }
    
    // 读取IV
    QByteArray iv = inputFile.read(16);
    if (iv.size() != 16) {
        qCWarning(cryptoManager) << "Invalid IV in encrypted file";
        return false;
    }
    
    KeyPair key = getKeyPair(metadata.keyId);
    
    // 分块解密文件
    const int blockSize = 4096;
    QByteArray buffer;
    while (!inputFile.atEnd()) {
        buffer = inputFile.read(blockSize);
        QByteArray decryptedBlock = decryptAES256(buffer, key.publicKey, iv);
        outputFile.write(decryptedBlock);
    }
    
    qCDebug(cryptoManager) << "File decrypted successfully:" << inputPath;
    return true;
}

QByteArray CryptoManager::signData(const QByteArray &data, const QString &keyId)
{
    KeyPair key = getKeyPair(keyId);
    
    switch (key.type) {
    case RSA2048:
        return signRSA(data, key.privateKey);
    case ECC_P256:
        return signECC(data, key.privateKey);
    case AES256:
    default:
        // AES不支持签名，使用HMAC
        return QCryptographicHash::hash(data + key.privateKey, QCryptographicHash::Sha256);
    }
}

bool CryptoManager::verifySignature(const QByteArray &data, const QByteArray &signature, qint64 senderId)
{
    QByteArray senderPublicKey = getContactPublicKey(senderId);
    if (senderPublicKey.isEmpty()) {
        return false;
    }
    
    // 这里简化处理，实际应用中需要根据密钥类型选择验证方法
    return verifyRSA(data, signature, senderPublicKey);
}

QByteArray CryptoManager::generateSharedSecret(const QByteArray &myPrivateKey, const QByteArray &theirPublicKey)
{
    // 简化的ECDH实现
    QByteArray combined = myPrivateKey + theirPublicKey;
    return QCryptographicHash::hash(combined, QCryptographicHash::Sha256);
}

QByteArray CryptoManager::deriveSessionKey(const QByteArray &sharedSecret, const QByteArray &salt)
{
    return deriveKey(sharedSecret, salt, 10000);
}

bool CryptoManager::rotateKeys(const QString &keyId)
{
    if (!_forwardSecrecyEnabled) {
        qCWarning(cryptoManager) << "Forward secrecy is not enabled";
        return false;
    }
    
    KeyPair oldKey = getKeyPair(keyId);
    if (oldKey.publicKey.isEmpty()) {
        qCWarning(cryptoManager) << "Key not found for rotation:" << keyId;
        return false;
    }
    
    // 生成新密钥
    KeyPair newKey = generateKeyPair(oldKey.type);
    QString newKeyId = generateKeyId();
    
    // 存储新密钥
    if (storeKeyPair(newKeyId, newKey)) {
        // 删除旧密钥
        deleteKeyPair(keyId);
        qCDebug(cryptoManager) << "Key rotated successfully:" << keyId << "->" << newKeyId;
        return true;
    }
    
    return false;
}

void CryptoManager::enableForwardSecrecy(bool enable)
{
    _forwardSecrecyEnabled = enable;
    qCDebug(cryptoManager) << "Forward secrecy" << (enable ? "enabled" : "disabled");
}

bool CryptoManager::isForwardSecrecyEnabled() const
{
    return _forwardSecrecyEnabled;
}

QByteArray CryptoManager::generateSalt(int size)
{
    QByteArray salt;
    salt.resize(size);
    for (int i = 0; i < size; ++i) {
        salt[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    return salt;
}

QByteArray CryptoManager::generateIV(int size)
{
    QByteArray iv;
    iv.resize(size);
    for (int i = 0; i < size; ++i) {
        iv[i] = static_cast<char>(QRandomGenerator::global()->bounded(256));
    }
    return iv;
}

QString CryptoManager::encryptedMessageToJson(const EncryptedMessage &message)
{
    QJsonObject json;
    json["encryptedData"] = QString::fromLatin1(message.encryptedData.toBase64());
    json["signature"] = QString::fromLatin1(message.signature.toBase64());
    json["iv"] = QString::fromLatin1(message.iv.toBase64());
    json["algorithm"] = static_cast<int>(message.algorithm);
    json["keyId"] = message.keyId;
    json["timestamp"] = message.timestamp;
    
    QJsonDocument doc(json);
    return doc.toJson(QJsonDocument::Compact);
}

CryptoManager::EncryptedMessage CryptoManager::encryptedMessageFromJson(const QString &json)
{
    EncryptedMessage message;
    
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
    if (!doc.isObject()) {
        return message;
    }
    
    QJsonObject obj = doc.object();
    message.encryptedData = QByteArray::fromBase64(obj["encryptedData"].toString().toLatin1());
    message.signature = QByteArray::fromBase64(obj["signature"].toString().toLatin1());
    message.iv = QByteArray::fromBase64(obj["iv"].toString().toLatin1());
    message.algorithm = static_cast<EncryptionType>(obj["algorithm"].toInt());
    message.keyId = obj["keyId"].toString();
    message.timestamp = obj["timestamp"].toVariant().toLongLong();
    
    return message;
}

bool CryptoManager::isEncryptionSupported(EncryptionType type)
{
    switch (type) {
    case AES256:
    case RSA2048:
    case ECC_P256:
        return true;
    case None:
    default:
        return false;
    }
}

// 私有方法实现

QByteArray CryptoManager::encryptAES256(const QByteArray &data, const QByteArray &key, const QByteArray &iv)
{
    // 简化的AES加密实现
    // 实际应用中应使用OpenSSL或Qt的加密库
    QByteArray encrypted = data;
    for (int i = 0; i < encrypted.size(); ++i) {
        encrypted[i] = encrypted[i] ^ key[i % key.size()] ^ iv[i % iv.size()];
    }
    return encrypted;
}

QByteArray CryptoManager::decryptAES256(const QByteArray &encryptedData, const QByteArray &key, const QByteArray &iv)
{
    // 简化的AES解密实现
    QByteArray decrypted = encryptedData;
    for (int i = 0; i < decrypted.size(); ++i) {
        decrypted[i] = decrypted[i] ^ key[i % key.size()] ^ iv[i % iv.size()];
    }
    return decrypted;
}

CryptoManager::KeyPair CryptoManager::generateRSAKeyPair()
{
    KeyPair keyPair;
    // 简化的RSA密钥生成
    // 实际应用中应使用OpenSSL
    keyPair.publicKey = generateSalt(256);
    keyPair.privateKey = generateSalt(256);
    return keyPair;
}

CryptoManager::KeyPair CryptoManager::generateECCKeyPair()
{
    KeyPair keyPair;
    // 简化的ECC密钥生成
    // 实际应用中应使用OpenSSL
    keyPair.publicKey = generateSalt(32);
    keyPair.privateKey = generateSalt(32);
    return keyPair;
}

CryptoManager::KeyPair CryptoManager::generateAESKey()
{
    KeyPair keyPair;
    keyPair.publicKey = generateSalt(32); // AES-256密钥
    keyPair.privateKey = generateSalt(32);
    return keyPair;
}

QByteArray CryptoManager::encryptRSA(const QByteArray &data, const QByteArray &publicKey)
{
    // 简化的RSA加密实现
    return encryptAES256(data, publicKey, generateIV());
}

QByteArray CryptoManager::decryptRSA(const QByteArray &encryptedData, const QByteArray &privateKey)
{
    // 简化的RSA解密实现
    return decryptAES256(encryptedData, privateKey, generateIV());
}

QByteArray CryptoManager::signRSA(const QByteArray &data, const QByteArray &privateKey)
{
    // 简化的RSA签名实现
    return QCryptographicHash::hash(data + privateKey, QCryptographicHash::Sha256);
}

bool CryptoManager::verifyRSA(const QByteArray &data, const QByteArray &signature, const QByteArray &publicKey)
{
    // 简化的RSA验证实现
    QByteArray expectedSignature = QCryptographicHash::hash(data + publicKey, QCryptographicHash::Sha256);
    return signature == expectedSignature;
}

QByteArray CryptoManager::signECC(const QByteArray &data, const QByteArray &privateKey)
{
    // 简化的ECC签名实现
    return QCryptographicHash::hash(data + privateKey, QCryptographicHash::Sha256);
}

bool CryptoManager::verifyECC(const QByteArray &data, const QByteArray &signature, const QByteArray &publicKey)
{
    // 简化的ECC验证实现
    QByteArray expectedSignature = QCryptographicHash::hash(data + publicKey, QCryptographicHash::Sha256);
    return signature == expectedSignature;
}

QByteArray CryptoManager::deriveKey(const QByteArray &password, const QByteArray &salt, int iterations)
{
    QByteArray key = password + salt;
    for (int i = 0; i < iterations; ++i) {
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha256);
    }
    return key;
}

QByteArray CryptoManager::hashData(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

bool CryptoManager::saveKeyPairToStorage(const QString &keyId, const KeyPair &keyPair)
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath = dataPath + "/keys/" + keyId + ".json";
    
    QJsonObject json;
    json["publicKey"] = QString::fromLatin1(keyPair.publicKey.toBase64());
    json["privateKey"] = QString::fromLatin1(keyPair.privateKey.toBase64());
    json["type"] = static_cast<int>(keyPair.type);
    json["createdAt"] = keyPair.createdAt.toString(Qt::ISODate);
    json["expiresAt"] = keyPair.expiresAt.toString(Qt::ISODate);
    
    QJsonDocument doc(json);
    QFile file(filePath);
    return file.open(QIODevice::WriteOnly) && file.write(doc.toJson()) > 0;
}

CryptoManager::KeyPair CryptoManager::loadKeyPairFromStorage(const QString &keyId)
{
    KeyPair keyPair;
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath = dataPath + "/keys/" + keyId + ".json";
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return keyPair;
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return keyPair;
    }
    
    QJsonObject json = doc.object();
    keyPair.publicKey = QByteArray::fromBase64(json["publicKey"].toString().toLatin1());
    keyPair.privateKey = QByteArray::fromBase64(json["privateKey"].toString().toLatin1());
    keyPair.type = static_cast<EncryptionType>(json["type"].toInt());
    keyPair.createdAt = QDateTime::fromString(json["createdAt"].toString(), Qt::ISODate);
    keyPair.expiresAt = QDateTime::fromString(json["expiresAt"].toString(), Qt::ISODate);
    
    return keyPair;
}

bool CryptoManager::deleteKeyPairFromStorage(const QString &keyId)
{
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString filePath = dataPath + "/keys/" + keyId + ".json";
    return QFile::remove(filePath);
}

void CryptoManager::saveContactPublicKey(qint64 contactId, const QByteArray &publicKey)
{
    QSettings settings;
    settings.setValue(QString("contacts/%1/publicKey").arg(contactId), 
                     QString::fromLatin1(publicKey.toBase64()));
}

QByteArray CryptoManager::loadContactPublicKey(qint64 contactId)
{
    QSettings settings;
    QString base64Key = settings.value(QString("contacts/%1/publicKey").arg(contactId)).toString();
    return QByteArray::fromBase64(base64Key.toLatin1());
}

bool CryptoManager::validateKeyPair(const KeyPair &keyPair)
{
    if (keyPair.publicKey.isEmpty() || keyPair.privateKey.isEmpty()) {
        return false;
    }
    
    if (keyPair.expiresAt < QDateTime::currentDateTime()) {
        return false;
    }
    
    return true;
}

bool CryptoManager::validateEncryptedMessage(const EncryptedMessage &message)
{
    if (message.encryptedData.isEmpty() || message.signature.isEmpty() || message.iv.isEmpty()) {
        return false;
    }
    
    if (message.timestamp <= 0) {
        return false;
    }
    
    // 检查消息是否过期（24小时）
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    if (currentTime - message.timestamp > 24 * 60 * 60 * 1000) {
        return false;
    }
    
    return true;
}

QString CryptoManager::generateKeyId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
} 