/****************************************************************************
** Meta object code from reading C++ file 'NetworkClient.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/network/NetworkClient.h"
#include <QtNetwork/QSslError>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NetworkClient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.5.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {

#ifdef QT_MOC_HAS_STRINGDATA
struct qt_meta_stringdata_CLASSNetworkClientENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSNetworkClientENDCLASS = QtMocHelpers::stringData(
    "NetworkClient",
    "connected",
    "",
    "disconnected",
    "connectionError",
    "error",
    "loginResponse",
    "success",
    "message",
    "registerResponse",
    "verifyEmailResponse",
    "sendVerificationResponse",
    "resendVerificationResponse",
    "emailCodeVerificationResponse",
    "logoutResponse",
    "captchaReceived",
    "captchaImage",
    "usernameAvailability",
    "available",
    "emailAvailability",
    "avatarUploaded",
    "avatarUrl",
    "messageReceived",
    "sender",
    "content",
    "messageType",
    "timestamp",
    "messageSent",
    "messageId",
    "messageDelivered",
    "networkError",
    "emailCodeVerified",
    "onConnected",
    "onDisconnected",
    "onSocketReadyRead",
    "onSocketError",
    "QAbstractSocket::SocketError",
    "socketError",
    "onSslErrors",
    "QList<QSslError>",
    "errors",
    "onHeartbeatTimeout",
    "onNetworkReplyFinished"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSNetworkClientENDCLASS_t {
    uint offsetsAndSizes[86];
    char stringdata0[14];
    char stringdata1[10];
    char stringdata2[1];
    char stringdata3[13];
    char stringdata4[16];
    char stringdata5[6];
    char stringdata6[14];
    char stringdata7[8];
    char stringdata8[8];
    char stringdata9[17];
    char stringdata10[20];
    char stringdata11[25];
    char stringdata12[27];
    char stringdata13[30];
    char stringdata14[15];
    char stringdata15[16];
    char stringdata16[13];
    char stringdata17[21];
    char stringdata18[10];
    char stringdata19[18];
    char stringdata20[15];
    char stringdata21[10];
    char stringdata22[16];
    char stringdata23[7];
    char stringdata24[8];
    char stringdata25[12];
    char stringdata26[10];
    char stringdata27[12];
    char stringdata28[10];
    char stringdata29[17];
    char stringdata30[13];
    char stringdata31[18];
    char stringdata32[12];
    char stringdata33[15];
    char stringdata34[18];
    char stringdata35[14];
    char stringdata36[29];
    char stringdata37[12];
    char stringdata38[12];
    char stringdata39[17];
    char stringdata40[7];
    char stringdata41[19];
    char stringdata42[23];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSNetworkClientENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSNetworkClientENDCLASS_t qt_meta_stringdata_CLASSNetworkClientENDCLASS = {
    {
        QT_MOC_LITERAL(0, 13),  // "NetworkClient"
        QT_MOC_LITERAL(14, 9),  // "connected"
        QT_MOC_LITERAL(24, 0),  // ""
        QT_MOC_LITERAL(25, 12),  // "disconnected"
        QT_MOC_LITERAL(38, 15),  // "connectionError"
        QT_MOC_LITERAL(54, 5),  // "error"
        QT_MOC_LITERAL(60, 13),  // "loginResponse"
        QT_MOC_LITERAL(74, 7),  // "success"
        QT_MOC_LITERAL(82, 7),  // "message"
        QT_MOC_LITERAL(90, 16),  // "registerResponse"
        QT_MOC_LITERAL(107, 19),  // "verifyEmailResponse"
        QT_MOC_LITERAL(127, 24),  // "sendVerificationResponse"
        QT_MOC_LITERAL(152, 26),  // "resendVerificationResponse"
        QT_MOC_LITERAL(179, 29),  // "emailCodeVerificationResponse"
        QT_MOC_LITERAL(209, 14),  // "logoutResponse"
        QT_MOC_LITERAL(224, 15),  // "captchaReceived"
        QT_MOC_LITERAL(240, 12),  // "captchaImage"
        QT_MOC_LITERAL(253, 20),  // "usernameAvailability"
        QT_MOC_LITERAL(274, 9),  // "available"
        QT_MOC_LITERAL(284, 17),  // "emailAvailability"
        QT_MOC_LITERAL(302, 14),  // "avatarUploaded"
        QT_MOC_LITERAL(317, 9),  // "avatarUrl"
        QT_MOC_LITERAL(327, 15),  // "messageReceived"
        QT_MOC_LITERAL(343, 6),  // "sender"
        QT_MOC_LITERAL(350, 7),  // "content"
        QT_MOC_LITERAL(358, 11),  // "messageType"
        QT_MOC_LITERAL(370, 9),  // "timestamp"
        QT_MOC_LITERAL(380, 11),  // "messageSent"
        QT_MOC_LITERAL(392, 9),  // "messageId"
        QT_MOC_LITERAL(402, 16),  // "messageDelivered"
        QT_MOC_LITERAL(419, 12),  // "networkError"
        QT_MOC_LITERAL(432, 17),  // "emailCodeVerified"
        QT_MOC_LITERAL(450, 11),  // "onConnected"
        QT_MOC_LITERAL(462, 14),  // "onDisconnected"
        QT_MOC_LITERAL(477, 17),  // "onSocketReadyRead"
        QT_MOC_LITERAL(495, 13),  // "onSocketError"
        QT_MOC_LITERAL(509, 28),  // "QAbstractSocket::SocketError"
        QT_MOC_LITERAL(538, 11),  // "socketError"
        QT_MOC_LITERAL(550, 11),  // "onSslErrors"
        QT_MOC_LITERAL(562, 16),  // "QList<QSslError>"
        QT_MOC_LITERAL(579, 6),  // "errors"
        QT_MOC_LITERAL(586, 18),  // "onHeartbeatTimeout"
        QT_MOC_LITERAL(605, 22)   // "onNetworkReplyFinished"
    },
    "NetworkClient",
    "connected",
    "",
    "disconnected",
    "connectionError",
    "error",
    "loginResponse",
    "success",
    "message",
    "registerResponse",
    "verifyEmailResponse",
    "sendVerificationResponse",
    "resendVerificationResponse",
    "emailCodeVerificationResponse",
    "logoutResponse",
    "captchaReceived",
    "captchaImage",
    "usernameAvailability",
    "available",
    "emailAvailability",
    "avatarUploaded",
    "avatarUrl",
    "messageReceived",
    "sender",
    "content",
    "messageType",
    "timestamp",
    "messageSent",
    "messageId",
    "messageDelivered",
    "networkError",
    "emailCodeVerified",
    "onConnected",
    "onDisconnected",
    "onSocketReadyRead",
    "onSocketError",
    "QAbstractSocket::SocketError",
    "socketError",
    "onSslErrors",
    "QList<QSslError>",
    "errors",
    "onHeartbeatTimeout",
    "onNetworkReplyFinished"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSNetworkClientENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      26,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      19,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  170,    2, 0x06,    1 /* Public */,
       3,    0,  171,    2, 0x06,    2 /* Public */,
       4,    1,  172,    2, 0x06,    3 /* Public */,
       6,    2,  175,    2, 0x06,    5 /* Public */,
       9,    2,  180,    2, 0x06,    8 /* Public */,
      10,    2,  185,    2, 0x06,   11 /* Public */,
      11,    2,  190,    2, 0x06,   14 /* Public */,
      12,    2,  195,    2, 0x06,   17 /* Public */,
      13,    2,  200,    2, 0x06,   20 /* Public */,
      14,    1,  205,    2, 0x06,   23 /* Public */,
      15,    1,  208,    2, 0x06,   25 /* Public */,
      17,    1,  211,    2, 0x06,   27 /* Public */,
      19,    1,  214,    2, 0x06,   29 /* Public */,
      20,    2,  217,    2, 0x06,   31 /* Public */,
      22,    4,  222,    2, 0x06,   34 /* Public */,
      27,    1,  231,    2, 0x06,   39 /* Public */,
      29,    1,  234,    2, 0x06,   41 /* Public */,
      30,    1,  237,    2, 0x06,   43 /* Public */,
      31,    2,  240,    2, 0x06,   45 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      32,    0,  245,    2, 0x08,   48 /* Private */,
      33,    0,  246,    2, 0x08,   49 /* Private */,
      34,    0,  247,    2, 0x08,   50 /* Private */,
      35,    1,  248,    2, 0x08,   51 /* Private */,
      38,    1,  251,    2, 0x08,   53 /* Private */,
      41,    0,  254,    2, 0x08,   55 /* Private */,
      42,    0,  255,    2, 0x08,   56 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::QString,   16,
    QMetaType::Void, QMetaType::Bool,   18,
    QMetaType::Void, QMetaType::Bool,   18,
    QMetaType::Void, QMetaType::Bool, QMetaType::QUrl,    7,   21,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::LongLong,   23,   24,   25,   26,
    QMetaType::Void, QMetaType::QString,   28,
    QMetaType::Void, QMetaType::QString,   28,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 36,   37,
    QMetaType::Void, 0x80000000 | 39,   40,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject NetworkClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSNetworkClientENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSNetworkClientENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSNetworkClientENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<NetworkClient, std::true_type>,
        // method 'connected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'disconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'connectionError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'loginResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'registerResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'verifyEmailResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sendVerificationResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resendVerificationResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'emailCodeVerificationResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'logoutResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'captchaReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'usernameAvailability'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'emailAvailability'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'avatarUploaded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'messageReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'messageSent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'messageDelivered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'networkError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'emailCodeVerified'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSocketReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSocketError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QAbstractSocket::SocketError, std::false_type>,
        // method 'onSslErrors'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<QSslError> &, std::false_type>,
        // method 'onHeartbeatTimeout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onNetworkReplyFinished'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void NetworkClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<NetworkClient *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->connected(); break;
        case 1: _t->disconnected(); break;
        case 2: _t->connectionError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->loginResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->registerResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 5: _t->verifyEmailResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 6: _t->sendVerificationResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 7: _t->resendVerificationResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 8: _t->emailCodeVerificationResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 9: _t->logoutResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 10: _t->captchaReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 11: _t->usernameAvailability((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 12: _t->emailAvailability((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 13: _t->avatarUploaded((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[2]))); break;
        case 14: _t->messageReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[4]))); break;
        case 15: _t->messageSent((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 16: _t->messageDelivered((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 17: _t->networkError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 18: _t->emailCodeVerified((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 19: _t->onConnected(); break;
        case 20: _t->onDisconnected(); break;
        case 21: _t->onSocketReadyRead(); break;
        case 22: _t->onSocketError((*reinterpret_cast< std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        case 23: _t->onSslErrors((*reinterpret_cast< std::add_pointer_t<QList<QSslError>>>(_a[1]))); break;
        case 24: _t->onHeartbeatTimeout(); break;
        case 25: _t->onNetworkReplyFinished(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 22:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        case 23:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<QSslError> >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (NetworkClient::*)();
            if (_t _q_method = &NetworkClient::connected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)();
            if (_t _q_method = &NetworkClient::disconnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & );
            if (_t _q_method = &NetworkClient::connectionError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QString & );
            if (_t _q_method = &NetworkClient::loginResponse; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QString & );
            if (_t _q_method = &NetworkClient::registerResponse; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QString & );
            if (_t _q_method = &NetworkClient::verifyEmailResponse; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QString & );
            if (_t _q_method = &NetworkClient::sendVerificationResponse; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QString & );
            if (_t _q_method = &NetworkClient::resendVerificationResponse; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QString & );
            if (_t _q_method = &NetworkClient::emailCodeVerificationResponse; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool );
            if (_t _q_method = &NetworkClient::logoutResponse; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & );
            if (_t _q_method = &NetworkClient::captchaReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool );
            if (_t _q_method = &NetworkClient::usernameAvailability; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool );
            if (_t _q_method = &NetworkClient::emailAvailability; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QUrl & );
            if (_t _q_method = &NetworkClient::avatarUploaded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & , const QString & , const QString & , qint64 );
            if (_t _q_method = &NetworkClient::messageReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & );
            if (_t _q_method = &NetworkClient::messageSent; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & );
            if (_t _q_method = &NetworkClient::messageDelivered; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & );
            if (_t _q_method = &NetworkClient::networkError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 17;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QString & );
            if (_t _q_method = &NetworkClient::emailCodeVerified; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 18;
                return;
            }
        }
    }
}

const QMetaObject *NetworkClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NetworkClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSNetworkClientENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int NetworkClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 26)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 26;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 26)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 26;
    }
    return _id;
}

// SIGNAL 0
void NetworkClient::connected()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void NetworkClient::disconnected()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void NetworkClient::connectionError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void NetworkClient::loginResponse(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void NetworkClient::registerResponse(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void NetworkClient::verifyEmailResponse(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void NetworkClient::sendVerificationResponse(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void NetworkClient::resendVerificationResponse(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void NetworkClient::emailCodeVerificationResponse(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void NetworkClient::logoutResponse(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void NetworkClient::captchaReceived(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void NetworkClient::usernameAvailability(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void NetworkClient::emailAvailability(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void NetworkClient::avatarUploaded(bool _t1, const QUrl & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void NetworkClient::messageReceived(const QString & _t1, const QString & _t2, const QString & _t3, qint64 _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void NetworkClient::messageSent(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}

// SIGNAL 16
void NetworkClient::messageDelivered(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void NetworkClient::networkError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 17, _a);
}

// SIGNAL 18
void NetworkClient::emailCodeVerified(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}
QT_WARNING_POP
