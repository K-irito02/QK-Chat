/****************************************************************************
** Meta object code from reading C++ file 'SessionManager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/core/SessionManager.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SessionManager.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSSessionManagerENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSSessionManagerENDCLASS = QtMocHelpers::stringData(
    "SessionManager",
    "sessionCreated",
    "",
    "userId",
    "sessionToken",
    "sessionRemoved",
    "sessionExpired",
    "onUserDisconnected",
    "identifier",
    "createSession",
    "ipAddress",
    "expirationHours",
    "deviceInfo",
    "validateSession",
    "qint64&",
    "updateSessionLastActive",
    "removeSession",
    "removeUserSessions",
    "cleanExpiredSessions",
    "getUserIdBySession",
    "getDeviceInfo",
    "getIpAddress",
    "getSessionExpiry",
    "getActiveSessionCount",
    "getUserSessionCount"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSSessionManagerENDCLASS_t {
    uint offsetsAndSizes[50];
    char stringdata0[15];
    char stringdata1[15];
    char stringdata2[1];
    char stringdata3[7];
    char stringdata4[13];
    char stringdata5[15];
    char stringdata6[15];
    char stringdata7[19];
    char stringdata8[11];
    char stringdata9[14];
    char stringdata10[10];
    char stringdata11[16];
    char stringdata12[11];
    char stringdata13[16];
    char stringdata14[8];
    char stringdata15[24];
    char stringdata16[14];
    char stringdata17[19];
    char stringdata18[21];
    char stringdata19[19];
    char stringdata20[14];
    char stringdata21[13];
    char stringdata22[17];
    char stringdata23[22];
    char stringdata24[20];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSSessionManagerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSSessionManagerENDCLASS_t qt_meta_stringdata_CLASSSessionManagerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 14),  // "SessionManager"
        QT_MOC_LITERAL(15, 14),  // "sessionCreated"
        QT_MOC_LITERAL(30, 0),  // ""
        QT_MOC_LITERAL(31, 6),  // "userId"
        QT_MOC_LITERAL(38, 12),  // "sessionToken"
        QT_MOC_LITERAL(51, 14),  // "sessionRemoved"
        QT_MOC_LITERAL(66, 14),  // "sessionExpired"
        QT_MOC_LITERAL(81, 18),  // "onUserDisconnected"
        QT_MOC_LITERAL(100, 10),  // "identifier"
        QT_MOC_LITERAL(111, 13),  // "createSession"
        QT_MOC_LITERAL(125, 9),  // "ipAddress"
        QT_MOC_LITERAL(135, 15),  // "expirationHours"
        QT_MOC_LITERAL(151, 10),  // "deviceInfo"
        QT_MOC_LITERAL(162, 15),  // "validateSession"
        QT_MOC_LITERAL(178, 7),  // "qint64&"
        QT_MOC_LITERAL(186, 23),  // "updateSessionLastActive"
        QT_MOC_LITERAL(210, 13),  // "removeSession"
        QT_MOC_LITERAL(224, 18),  // "removeUserSessions"
        QT_MOC_LITERAL(243, 20),  // "cleanExpiredSessions"
        QT_MOC_LITERAL(264, 18),  // "getUserIdBySession"
        QT_MOC_LITERAL(283, 13),  // "getDeviceInfo"
        QT_MOC_LITERAL(297, 12),  // "getIpAddress"
        QT_MOC_LITERAL(310, 16),  // "getSessionExpiry"
        QT_MOC_LITERAL(327, 21),  // "getActiveSessionCount"
        QT_MOC_LITERAL(349, 19)   // "getUserSessionCount"
    },
    "SessionManager",
    "sessionCreated",
    "",
    "userId",
    "sessionToken",
    "sessionRemoved",
    "sessionExpired",
    "onUserDisconnected",
    "identifier",
    "createSession",
    "ipAddress",
    "expirationHours",
    "deviceInfo",
    "validateSession",
    "qint64&",
    "updateSessionLastActive",
    "removeSession",
    "removeUserSessions",
    "cleanExpiredSessions",
    "getUserIdBySession",
    "getDeviceInfo",
    "getIpAddress",
    "getSessionExpiry",
    "getActiveSessionCount",
    "getUserSessionCount"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSSessionManagerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      19,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    2,  128,    2, 0x06,    1 /* Public */,
       5,    1,  133,    2, 0x06,    4 /* Public */,
       6,    2,  136,    2, 0x06,    6 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    1,  141,    2, 0x0a,    9 /* Public */,
       9,    3,  144,    2, 0x0a,   11 /* Public */,
       9,    2,  151,    2, 0x2a,   15 /* Public | MethodCloned */,
       9,    4,  156,    2, 0x0a,   18 /* Public */,
       9,    3,  165,    2, 0x2a,   23 /* Public | MethodCloned */,
      13,    2,  172,    2, 0x0a,   27 /* Public */,
      15,    1,  177,    2, 0x0a,   30 /* Public */,
      16,    1,  180,    2, 0x0a,   32 /* Public */,
      17,    1,  183,    2, 0x0a,   34 /* Public */,
      18,    0,  186,    2, 0x0a,   36 /* Public */,
      19,    1,  187,    2, 0x0a,   37 /* Public */,
      20,    1,  190,    2, 0x0a,   39 /* Public */,
      21,    1,  193,    2, 0x0a,   41 /* Public */,
      22,    1,  196,    2, 0x0a,   43 /* Public */,
      23,    0,  199,    2, 0x10a,   45 /* Public | MethodIsConst  */,
      24,    1,  200,    2, 0x10a,   46 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,    3,    4,
    QMetaType::Void, QMetaType::QString,    4,
    QMetaType::Void, QMetaType::LongLong, QMetaType::QString,    3,    4,

 // slots: parameters
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::QString, QMetaType::LongLong, QMetaType::QString, QMetaType::Int,    3,   10,   11,
    QMetaType::QString, QMetaType::LongLong, QMetaType::QString,    3,   10,
    QMetaType::QString, QMetaType::LongLong, QMetaType::QString, QMetaType::QString, QMetaType::Int,    3,   12,   10,   11,
    QMetaType::QString, QMetaType::LongLong, QMetaType::QString, QMetaType::QString,    3,   12,   10,
    QMetaType::Bool, QMetaType::QString, 0x80000000 | 14,    4,    3,
    QMetaType::Bool, QMetaType::QString,    4,
    QMetaType::Bool, QMetaType::QString,    4,
    QMetaType::Bool, QMetaType::LongLong,    3,
    QMetaType::Void,
    QMetaType::LongLong, QMetaType::QString,    4,
    QMetaType::QString, QMetaType::QString,    4,
    QMetaType::QString, QMetaType::QString,    4,
    QMetaType::QDateTime, QMetaType::QString,    4,
    QMetaType::Int,
    QMetaType::Int, QMetaType::LongLong,    3,

       0        // eod
};

Q_CONSTINIT const QMetaObject SessionManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSSessionManagerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSSessionManagerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSSessionManagerENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<SessionManager, std::true_type>,
        // method 'sessionCreated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sessionRemoved'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'sessionExpired'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onUserDisconnected'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'createSession'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'createSession'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'createSession'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'createSession'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'validateSession'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64 &, std::false_type>,
        // method 'updateSessionLastActive'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'removeSession'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'removeUserSessions'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'cleanExpiredSessions'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'getUserIdBySession'
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'getDeviceInfo'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'getIpAddress'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'getSessionExpiry'
        QtPrivate::TypeAndForceComplete<QDateTime, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'getActiveSessionCount'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'getUserSessionCount'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>
    >,
    nullptr
} };

void SessionManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<SessionManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->sessionCreated((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 1: _t->sessionRemoved((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->sessionExpired((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 3: _t->onUserDisconnected((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 4: { QString _r = _t->createSession((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 5: { QString _r = _t->createSession((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 6: { QString _r = _t->createSession((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[4])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 7: { QString _r = _t->createSession((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 8: { bool _r = _t->validateSession((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64&>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 9: { bool _r = _t->updateSessionLastActive((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 10: { bool _r = _t->removeSession((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 11: { bool _r = _t->removeUserSessions((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 12: _t->cleanExpiredSessions(); break;
        case 13: { qint64 _r = _t->getUserIdBySession((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< qint64*>(_a[0]) = std::move(_r); }  break;
        case 14: { QString _r = _t->getDeviceInfo((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 15: { QString _r = _t->getIpAddress((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 16: { QDateTime _r = _t->getSessionExpiry((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QDateTime*>(_a[0]) = std::move(_r); }  break;
        case 17: { int _r = _t->getActiveSessionCount();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 18: { int _r = _t->getUserSessionCount((*reinterpret_cast< std::add_pointer_t<qint64>>(_a[1])));
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (SessionManager::*)(qint64 , const QString & );
            if (_t _q_method = &SessionManager::sessionCreated; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (SessionManager::*)(const QString & );
            if (_t _q_method = &SessionManager::sessionRemoved; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (SessionManager::*)(qint64 , const QString & );
            if (_t _q_method = &SessionManager::sessionExpired; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject *SessionManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SessionManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSSessionManagerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SessionManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 19)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 19;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 19)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 19;
    }
    return _id;
}

// SIGNAL 0
void SessionManager::sessionCreated(qint64 _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SessionManager::sessionRemoved(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SessionManager::sessionExpired(qint64 _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
