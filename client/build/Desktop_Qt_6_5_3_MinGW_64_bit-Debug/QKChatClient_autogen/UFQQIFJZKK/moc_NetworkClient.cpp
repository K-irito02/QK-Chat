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
    "logoutResponse",
    "captchaReceived",
    "captchaImage",
    "usernameAvailability",
    "available",
    "emailAvailability",
    "emailVerificationCodeSent",
    "emailVerificationCodeVerified",
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
    uint offsetsAndSizes[80];
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
    char stringdata10[15];
    char stringdata11[16];
    char stringdata12[13];
    char stringdata13[21];
    char stringdata14[10];
    char stringdata15[18];
    char stringdata16[26];
    char stringdata17[30];
    char stringdata18[15];
    char stringdata19[10];
    char stringdata20[16];
    char stringdata21[7];
    char stringdata22[8];
    char stringdata23[12];
    char stringdata24[10];
    char stringdata25[12];
    char stringdata26[10];
    char stringdata27[17];
    char stringdata28[13];
    char stringdata29[12];
    char stringdata30[15];
    char stringdata31[18];
    char stringdata32[14];
    char stringdata33[29];
    char stringdata34[12];
    char stringdata35[12];
    char stringdata36[17];
    char stringdata37[7];
    char stringdata38[19];
    char stringdata39[23];
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
        QT_MOC_LITERAL(107, 14),  // "logoutResponse"
        QT_MOC_LITERAL(122, 15),  // "captchaReceived"
        QT_MOC_LITERAL(138, 12),  // "captchaImage"
        QT_MOC_LITERAL(151, 20),  // "usernameAvailability"
        QT_MOC_LITERAL(172, 9),  // "available"
        QT_MOC_LITERAL(182, 17),  // "emailAvailability"
        QT_MOC_LITERAL(200, 25),  // "emailVerificationCodeSent"
        QT_MOC_LITERAL(226, 29),  // "emailVerificationCodeVerified"
        QT_MOC_LITERAL(256, 14),  // "avatarUploaded"
        QT_MOC_LITERAL(271, 9),  // "avatarUrl"
        QT_MOC_LITERAL(281, 15),  // "messageReceived"
        QT_MOC_LITERAL(297, 6),  // "sender"
        QT_MOC_LITERAL(304, 7),  // "content"
        QT_MOC_LITERAL(312, 11),  // "messageType"
        QT_MOC_LITERAL(324, 9),  // "timestamp"
        QT_MOC_LITERAL(334, 11),  // "messageSent"
        QT_MOC_LITERAL(346, 9),  // "messageId"
        QT_MOC_LITERAL(356, 16),  // "messageDelivered"
        QT_MOC_LITERAL(373, 12),  // "networkError"
        QT_MOC_LITERAL(386, 11),  // "onConnected"
        QT_MOC_LITERAL(398, 14),  // "onDisconnected"
        QT_MOC_LITERAL(413, 17),  // "onSocketReadyRead"
        QT_MOC_LITERAL(431, 13),  // "onSocketError"
        QT_MOC_LITERAL(445, 28),  // "QAbstractSocket::SocketError"
        QT_MOC_LITERAL(474, 11),  // "socketError"
        QT_MOC_LITERAL(486, 11),  // "onSslErrors"
        QT_MOC_LITERAL(498, 16),  // "QList<QSslError>"
        QT_MOC_LITERAL(515, 6),  // "errors"
        QT_MOC_LITERAL(522, 18),  // "onHeartbeatTimeout"
        QT_MOC_LITERAL(541, 22)   // "onNetworkReplyFinished"
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
    "logoutResponse",
    "captchaReceived",
    "captchaImage",
    "usernameAvailability",
    "available",
    "emailAvailability",
    "emailVerificationCodeSent",
    "emailVerificationCodeVerified",
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
      23,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      16,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  152,    2, 0x06,    1 /* Public */,
       3,    0,  153,    2, 0x06,    2 /* Public */,
       4,    1,  154,    2, 0x06,    3 /* Public */,
       6,    2,  157,    2, 0x06,    5 /* Public */,
       9,    2,  162,    2, 0x06,    8 /* Public */,
      10,    1,  167,    2, 0x06,   11 /* Public */,
      11,    1,  170,    2, 0x06,   13 /* Public */,
      13,    1,  173,    2, 0x06,   15 /* Public */,
      15,    1,  176,    2, 0x06,   17 /* Public */,
      16,    2,  179,    2, 0x06,   19 /* Public */,
      17,    2,  184,    2, 0x06,   22 /* Public */,
      18,    2,  189,    2, 0x06,   25 /* Public */,
      20,    4,  194,    2, 0x06,   28 /* Public */,
      25,    1,  203,    2, 0x06,   33 /* Public */,
      27,    1,  206,    2, 0x06,   35 /* Public */,
      28,    1,  209,    2, 0x06,   37 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      29,    0,  212,    2, 0x08,   39 /* Private */,
      30,    0,  213,    2, 0x08,   40 /* Private */,
      31,    0,  214,    2, 0x08,   41 /* Private */,
      32,    1,  215,    2, 0x08,   42 /* Private */,
      35,    1,  218,    2, 0x08,   44 /* Private */,
      38,    0,  221,    2, 0x08,   46 /* Private */,
      39,    0,  222,    2, 0x08,   47 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::Bool,    7,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::Bool,   14,
    QMetaType::Void, QMetaType::Bool,   14,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::Bool, QMetaType::QUrl,    7,   19,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::LongLong,   21,   22,   23,   24,
    QMetaType::Void, QMetaType::QString,   26,
    QMetaType::Void, QMetaType::QString,   26,
    QMetaType::Void, QMetaType::QString,    5,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 33,   34,
    QMetaType::Void, 0x80000000 | 36,   37,
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
        // method 'emailVerificationCodeSent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'emailVerificationCodeVerified'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
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
        case 5: _t->logoutResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: _t->captchaReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->usernameAvailability((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 8: _t->emailAvailability((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 9: _t->emailVerificationCodeSent((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 10: _t->emailVerificationCodeVerified((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 11: _t->avatarUploaded((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[2]))); break;
        case 12: _t->messageReceived((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[4]))); break;
        case 13: _t->messageSent((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 14: _t->messageDelivered((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: _t->networkError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 16: _t->onConnected(); break;
        case 17: _t->onDisconnected(); break;
        case 18: _t->onSocketReadyRead(); break;
        case 19: _t->onSocketError((*reinterpret_cast< std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        case 20: _t->onSslErrors((*reinterpret_cast< std::add_pointer_t<QList<QSslError>>>(_a[1]))); break;
        case 21: _t->onHeartbeatTimeout(); break;
        case 22: _t->onNetworkReplyFinished(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 19:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        case 20:
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
            using _t = void (NetworkClient::*)(bool );
            if (_t _q_method = &NetworkClient::logoutResponse; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & );
            if (_t _q_method = &NetworkClient::captchaReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool );
            if (_t _q_method = &NetworkClient::usernameAvailability; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool );
            if (_t _q_method = &NetworkClient::emailAvailability; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QString & );
            if (_t _q_method = &NetworkClient::emailVerificationCodeSent; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QString & );
            if (_t _q_method = &NetworkClient::emailVerificationCodeVerified; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(bool , const QUrl & );
            if (_t _q_method = &NetworkClient::avatarUploaded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & , const QString & , const QString & , qint64 );
            if (_t _q_method = &NetworkClient::messageReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & );
            if (_t _q_method = &NetworkClient::messageSent; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & );
            if (_t _q_method = &NetworkClient::messageDelivered; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (NetworkClient::*)(const QString & );
            if (_t _q_method = &NetworkClient::networkError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 15;
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
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
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
void NetworkClient::logoutResponse(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void NetworkClient::captchaReceived(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void NetworkClient::usernameAvailability(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void NetworkClient::emailAvailability(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void NetworkClient::emailVerificationCodeSent(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void NetworkClient::emailVerificationCodeVerified(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void NetworkClient::avatarUploaded(bool _t1, const QUrl & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void NetworkClient::messageReceived(const QString & _t1, const QString & _t2, const QString & _t3, qint64 _t4)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t4))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void NetworkClient::messageSent(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void NetworkClient::messageDelivered(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void NetworkClient::networkError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}
QT_WARNING_POP
