/****************************************************************************
** Meta object code from reading C++ file 'ChatServer.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/core/ChatServer.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ChatServer.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSChatServerENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSChatServerENDCLASS = QtMocHelpers::stringData(
    "ChatServer",
    "serverStarted",
    "",
    "serverStopped",
    "serverError",
    "error",
    "clientConnected",
    "clientId",
    "address",
    "clientDisconnected",
    "clientAuthenticated",
    "userId",
    "userOnline",
    "userOffline",
    "messageReceived",
    "fromUserId",
    "toUserId",
    "message",
    "messageProcessed",
    "messageId",
    "messageFailed",
    "performanceAlert",
    "systemOverloaded",
    "healthStatusChanged",
    "healthy",
    "databaseError",
    "databaseConnected",
    "performSystemMaintenance",
    "performCleanup",
    "updateSystemStats",
    "checkSystemHealth",
    "onConnectionManagerEvent",
    "onMessageEngineEvent",
    "onThreadManagerEvent",
    "onClientConnected",
    "QSslSocket*",
    "socket",
    "onClientDisconnected",
    "handleClientData",
    "handleClientDisconnected",
    "handleSocketError",
    "QAbstractSocket::SocketError"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSChatServerENDCLASS_t {
    uint offsetsAndSizes[84];
    char stringdata0[11];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[14];
    char stringdata4[12];
    char stringdata5[6];
    char stringdata6[16];
    char stringdata7[9];
    char stringdata8[8];
    char stringdata9[19];
    char stringdata10[20];
    char stringdata11[7];
    char stringdata12[11];
    char stringdata13[12];
    char stringdata14[16];
    char stringdata15[11];
    char stringdata16[9];
    char stringdata17[8];
    char stringdata18[17];
    char stringdata19[10];
    char stringdata20[14];
    char stringdata21[17];
    char stringdata22[17];
    char stringdata23[20];
    char stringdata24[8];
    char stringdata25[14];
    char stringdata26[18];
    char stringdata27[25];
    char stringdata28[15];
    char stringdata29[18];
    char stringdata30[18];
    char stringdata31[25];
    char stringdata32[21];
    char stringdata33[21];
    char stringdata34[18];
    char stringdata35[12];
    char stringdata36[7];
    char stringdata37[21];
    char stringdata38[17];
    char stringdata39[25];
    char stringdata40[18];
    char stringdata41[29];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSChatServerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSChatServerENDCLASS_t qt_meta_stringdata_CLASSChatServerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 10),  // "ChatServer"
        QT_MOC_LITERAL(11, 13),  // "serverStarted"
        QT_MOC_LITERAL(25, 0),  // ""
        QT_MOC_LITERAL(26, 13),  // "serverStopped"
        QT_MOC_LITERAL(40, 11),  // "serverError"
        QT_MOC_LITERAL(52, 5),  // "error"
        QT_MOC_LITERAL(58, 15),  // "clientConnected"
        QT_MOC_LITERAL(74, 8),  // "clientId"
        QT_MOC_LITERAL(83, 7),  // "address"
        QT_MOC_LITERAL(91, 18),  // "clientDisconnected"
        QT_MOC_LITERAL(110, 19),  // "clientAuthenticated"
        QT_MOC_LITERAL(130, 6),  // "userId"
        QT_MOC_LITERAL(137, 10),  // "userOnline"
        QT_MOC_LITERAL(148, 11),  // "userOffline"
        QT_MOC_LITERAL(160, 15),  // "messageReceived"
        QT_MOC_LITERAL(176, 10),  // "fromUserId"
        QT_MOC_LITERAL(187, 8),  // "toUserId"
        QT_MOC_LITERAL(196, 7),  // "message"
        QT_MOC_LITERAL(204, 16),  // "messageProcessed"
        QT_MOC_LITERAL(221, 9),  // "messageId"
        QT_MOC_LITERAL(231, 13),  // "messageFailed"
        QT_MOC_LITERAL(245, 16),  // "performanceAlert"
        QT_MOC_LITERAL(262, 16),  // "systemOverloaded"
        QT_MOC_LITERAL(279, 19),  // "healthStatusChanged"
        QT_MOC_LITERAL(299, 7),  // "healthy"
        QT_MOC_LITERAL(307, 13),  // "databaseError"
        QT_MOC_LITERAL(321, 17),  // "databaseConnected"
        QT_MOC_LITERAL(339, 24),  // "performSystemMaintenance"
        QT_MOC_LITERAL(364, 14),  // "performCleanup"
        QT_MOC_LITERAL(379, 17),  // "updateSystemStats"
        QT_MOC_LITERAL(397, 17),  // "checkSystemHealth"
        QT_MOC_LITERAL(415, 24),  // "onConnectionManagerEvent"
        QT_MOC_LITERAL(440, 20),  // "onMessageEngineEvent"
        QT_MOC_LITERAL(461, 20),  // "onThreadManagerEvent"
        QT_MOC_LITERAL(482, 17),  // "onClientConnected"
        QT_MOC_LITERAL(500, 11),  // "QSslSocket*"
        QT_MOC_LITERAL(512, 6),  // "socket"
        QT_MOC_LITERAL(519, 20),  // "onClientDisconnected"
        QT_MOC_LITERAL(540, 16),  // "handleClientData"
        QT_MOC_LITERAL(557, 24),  // "handleClientDisconnected"
        QT_MOC_LITERAL(582, 17),  // "handleSocketError"
        QT_MOC_LITERAL(600, 28)   // "QAbstractSocket::SocketError"
    },
    "ChatServer",
    "serverStarted",
    "",
    "serverStopped",
    "serverError",
    "error",
    "clientConnected",
    "clientId",
    "address",
    "clientDisconnected",
    "clientAuthenticated",
    "userId",
    "userOnline",
    "userOffline",
    "messageReceived",
    "fromUserId",
    "toUserId",
    "message",
    "messageProcessed",
    "messageId",
    "messageFailed",
    "performanceAlert",
    "systemOverloaded",
    "healthStatusChanged",
    "healthy",
    "databaseError",
    "databaseConnected",
    "performSystemMaintenance",
    "performCleanup",
    "updateSystemStats",
    "checkSystemHealth",
    "onConnectionManagerEvent",
    "onMessageEngineEvent",
    "onThreadManagerEvent",
    "onClientConnected",
    "QSslSocket*",
    "socket",
    "onClientDisconnected",
    "handleClientData",
    "handleClientDisconnected",
    "handleSocketError",
    "QAbstractSocket::SocketError"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSChatServerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      28,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      16,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  182,    2, 0x06,    1 /* Public */,
       3,    0,  183,    2, 0x06,    2 /* Public */,
       4,    1,  184,    2, 0x06,    3 /* Public */,
       6,    2,  187,    2, 0x06,    5 /* Public */,
       9,    2,  192,    2, 0x06,    8 /* Public */,
      10,    1,  197,    2, 0x06,   11 /* Public */,
      12,    1,  200,    2, 0x06,   13 /* Public */,
      13,    1,  203,    2, 0x06,   15 /* Public */,
      14,    3,  206,    2, 0x06,   17 /* Public */,
      18,    1,  213,    2, 0x06,   21 /* Public */,
      20,    2,  216,    2, 0x06,   23 /* Public */,
      21,    1,  221,    2, 0x06,   26 /* Public */,
      22,    0,  224,    2, 0x06,   28 /* Public */,
      23,    1,  225,    2, 0x06,   29 /* Public */,
      25,    1,  228,    2, 0x06,   31 /* Public */,
      26,    0,  231,    2, 0x06,   33 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      27,    0,  232,    2, 0x08,   34 /* Private */,
      28,    0,  233,    2, 0x08,   35 /* Private */,
      29,    0,  234,    2, 0x08,   36 /* Private */,
      30,    0,  235,    2, 0x08,   37 /* Private */,
      31,    0,  236,    2, 0x08,   38 /* Private */,
      32,    0,  237,    2, 0x08,   39 /* Private */,
      33,    0,  238,    2, 0x08,   40 /* Private */,
      34,    1,  239,    2, 0x08,   41 /* Private */,
      37,    1,  242,    2, 0x08,   43 /* Private */,
      38,    1,  245,    2, 0x08,   45 /* Private */,
      39,    1,  248,    2, 0x08,   47 /* Private */,
      40,    2,  251,    2, 0x08,   49 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,    7,    8,
    QMetaType::Void, QMetaType::LongLong,   11,
    QMetaType::Void, QMetaType::LongLong,   11,
    QMetaType::Void, QMetaType::LongLong,   11,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong, QMetaType::QJsonObject,   15,   16,   17,
    QMetaType::Void, QMetaType::QString,   19,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   19,    5,
    QMetaType::Void, QMetaType::QString,   17,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   24,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 35,   36,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString,    7,
    QMetaType::Void, QMetaType::QString, 0x80000000 | 41,    7,    5,

       0        // eod
};

Q_CONSTINIT const QMetaObject ChatServer::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSChatServerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSChatServerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSChatServerENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ChatServer, std::true_type>,
        // method 'serverStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'serverStopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'serverError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clientConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clientDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'clientAuthenticated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'userOnline'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'userOffline'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'messageReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QJsonObject &, std::false_type>,
        // method 'messageProcessed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'messageFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'performanceAlert'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'systemOverloaded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'healthStatusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'databaseError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'databaseConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'performSystemMaintenance'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'performCleanup'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateSystemStats'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'checkSystemHealth'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onConnectionManagerEvent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onMessageEngineEvent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onThreadManagerEvent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onClientConnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QSslSocket *, std::false_type>,
        // method 'onClientDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'handleClientData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'handleClientDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'handleSocketError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<QAbstractSocket::SocketError, std::false_type>
    >,
    nullptr
} };

void ChatServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ChatServer *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->serverStarted(); break;
        case 1: _t->serverStopped(); break;
        case 2: _t->serverError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 3: _t->clientConnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->clientDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 5: _t->clientAuthenticated((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 6: _t->userOnline((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 7: _t->userOffline((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 8: _t->messageReceived((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QJsonObject>>(_a[3]))); break;
        case 9: _t->messageProcessed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 10: _t->messageFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 11: _t->performanceAlert((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 12: _t->systemOverloaded(); break;
        case 13: _t->healthStatusChanged((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 14: _t->databaseError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: _t->databaseConnected(); break;
        case 16: _t->performSystemMaintenance(); break;
        case 17: _t->performCleanup(); break;
        case 18: _t->updateSystemStats(); break;
        case 19: _t->checkSystemHealth(); break;
        case 20: _t->onConnectionManagerEvent(); break;
        case 21: _t->onMessageEngineEvent(); break;
        case 22: _t->onThreadManagerEvent(); break;
        case 23: _t->onClientConnected((*reinterpret_cast< std::add_pointer_t<QSslSocket*>>(_a[1]))); break;
        case 24: _t->onClientDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 25: _t->handleClientData((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 26: _t->handleClientDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 27: _t->handleSocketError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QAbstractSocket::SocketError>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 23:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QSslSocket* >(); break;
            }
            break;
        case 27:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ChatServer::*)();
            if (_t _q_method = &ChatServer::serverStarted; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)();
            if (_t _q_method = &ChatServer::serverStopped; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(const QString & );
            if (_t _q_method = &ChatServer::serverError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(const QString & , const QString & );
            if (_t _q_method = &ChatServer::clientConnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(const QString & , const QString & );
            if (_t _q_method = &ChatServer::clientDisconnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(qint64 );
            if (_t _q_method = &ChatServer::clientAuthenticated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(qint64 );
            if (_t _q_method = &ChatServer::userOnline; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(qint64 );
            if (_t _q_method = &ChatServer::userOffline; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(qint64 , qint64 , const QJsonObject & );
            if (_t _q_method = &ChatServer::messageReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(const QString & );
            if (_t _q_method = &ChatServer::messageProcessed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(const QString & , const QString & );
            if (_t _q_method = &ChatServer::messageFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(const QString & );
            if (_t _q_method = &ChatServer::performanceAlert; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)();
            if (_t _q_method = &ChatServer::systemOverloaded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(bool );
            if (_t _q_method = &ChatServer::healthStatusChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(const QString & );
            if (_t _q_method = &ChatServer::databaseError; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)();
            if (_t _q_method = &ChatServer::databaseConnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
    }
}

const QMetaObject *ChatServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ChatServer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSChatServerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ChatServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 28)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 28;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 28)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 28;
    }
    return _id;
}

// SIGNAL 0
void ChatServer::serverStarted()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ChatServer::serverStopped()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ChatServer::serverError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void ChatServer::clientConnected(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ChatServer::clientDisconnected(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ChatServer::clientAuthenticated(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void ChatServer::userOnline(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void ChatServer::userOffline(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
}

// SIGNAL 8
void ChatServer::messageReceived(qint64 _t1, qint64 _t2, const QJsonObject & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void ChatServer::messageProcessed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 9, _a);
}

// SIGNAL 10
void ChatServer::messageFailed(const QString & _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void ChatServer::performanceAlert(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void ChatServer::systemOverloaded()
{
    QMetaObject::activate(this, &staticMetaObject, 12, nullptr);
}

// SIGNAL 13
void ChatServer::healthStatusChanged(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void ChatServer::databaseError(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}

// SIGNAL 15
void ChatServer::databaseConnected()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}
QT_WARNING_POP
