/****************************************************************************
** Meta object code from reading C++ file 'ChatServer.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/core/ChatServer.h"
#include <QtCore/qmetatype.h>
#include <QtCore/QList>

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
    "socketId",
    "clientDisconnected",
    "messageReceived",
    "fromUserId",
    "toUserId",
    "message",
    "userOnline",
    "userId",
    "userOffline",
    "onNewConnection",
    "onClientDisconnected",
    "onClientDataReceived",
    "onSslErrors",
    "QList<QSslError>",
    "errors",
    "cleanupConnections"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSChatServerENDCLASS_t {
    uint offsetsAndSizes[46];
    char stringdata0[11];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[14];
    char stringdata4[12];
    char stringdata5[6];
    char stringdata6[16];
    char stringdata7[9];
    char stringdata8[19];
    char stringdata9[16];
    char stringdata10[11];
    char stringdata11[9];
    char stringdata12[8];
    char stringdata13[11];
    char stringdata14[7];
    char stringdata15[12];
    char stringdata16[16];
    char stringdata17[21];
    char stringdata18[21];
    char stringdata19[12];
    char stringdata20[17];
    char stringdata21[7];
    char stringdata22[19];
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
        QT_MOC_LITERAL(74, 8),  // "socketId"
        QT_MOC_LITERAL(83, 18),  // "clientDisconnected"
        QT_MOC_LITERAL(102, 15),  // "messageReceived"
        QT_MOC_LITERAL(118, 10),  // "fromUserId"
        QT_MOC_LITERAL(129, 8),  // "toUserId"
        QT_MOC_LITERAL(138, 7),  // "message"
        QT_MOC_LITERAL(146, 10),  // "userOnline"
        QT_MOC_LITERAL(157, 6),  // "userId"
        QT_MOC_LITERAL(164, 11),  // "userOffline"
        QT_MOC_LITERAL(176, 15),  // "onNewConnection"
        QT_MOC_LITERAL(192, 20),  // "onClientDisconnected"
        QT_MOC_LITERAL(213, 20),  // "onClientDataReceived"
        QT_MOC_LITERAL(234, 11),  // "onSslErrors"
        QT_MOC_LITERAL(246, 16),  // "QList<QSslError>"
        QT_MOC_LITERAL(263, 6),  // "errors"
        QT_MOC_LITERAL(270, 18)   // "cleanupConnections"
    },
    "ChatServer",
    "serverStarted",
    "",
    "serverStopped",
    "serverError",
    "error",
    "clientConnected",
    "socketId",
    "clientDisconnected",
    "messageReceived",
    "fromUserId",
    "toUserId",
    "message",
    "userOnline",
    "userId",
    "userOffline",
    "onNewConnection",
    "onClientDisconnected",
    "onClientDataReceived",
    "onSslErrors",
    "QList<QSslError>",
    "errors",
    "cleanupConnections"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSChatServerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   92,    2, 0x06,    1 /* Public */,
       3,    0,   93,    2, 0x06,    2 /* Public */,
       4,    1,   94,    2, 0x06,    3 /* Public */,
       6,    1,   97,    2, 0x06,    5 /* Public */,
       8,    1,  100,    2, 0x06,    7 /* Public */,
       9,    3,  103,    2, 0x06,    9 /* Public */,
      13,    1,  110,    2, 0x06,   13 /* Public */,
      15,    1,  113,    2, 0x06,   15 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      16,    0,  116,    2, 0x08,   17 /* Private */,
      17,    0,  117,    2, 0x08,   18 /* Private */,
      18,    0,  118,    2, 0x08,   19 /* Private */,
      19,    1,  119,    2, 0x08,   20 /* Private */,
      22,    0,  122,    2, 0x08,   22 /* Private */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::LongLong,    7,
    QMetaType::Void, QMetaType::LongLong,    7,
    QMetaType::Void, QMetaType::LongLong, QMetaType::LongLong, QMetaType::QString,   10,   11,   12,
    QMetaType::Void, QMetaType::LongLong,   14,
    QMetaType::Void, QMetaType::LongLong,   14,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 20,   21,
    QMetaType::Void,

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
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'clientDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'messageReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'userOnline'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'userOffline'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'onNewConnection'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onClientDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onClientDataReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSslErrors'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QList<QSslError> &, std::false_type>,
        // method 'cleanupConnections'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
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
        case 3: _t->clientConnected((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 4: _t->clientDisconnected((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 5: _t->messageReceived((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 6: _t->userOnline((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 7: _t->userOffline((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1]))); break;
        case 8: _t->onNewConnection(); break;
        case 9: _t->onClientDisconnected(); break;
        case 10: _t->onClientDataReceived(); break;
        case 11: _t->onSslErrors((*reinterpret_cast< std::add_pointer_t<QList<QSslError>>>(_a[1]))); break;
        case 12: _t->cleanupConnections(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 11:
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
            using _t = void (ChatServer::*)(qint64 );
            if (_t _q_method = &ChatServer::clientConnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(qint64 );
            if (_t _q_method = &ChatServer::clientDisconnected; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ChatServer::*)(qint64 , qint64 , const QString & );
            if (_t _q_method = &ChatServer::messageReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
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
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
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
void ChatServer::clientConnected(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void ChatServer::clientDisconnected(qint64 _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void ChatServer::messageReceived(qint64 _t1, qint64 _t2, const QString & _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
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
QT_WARNING_POP
