/****************************************************************************
** Meta object code from reading C++ file 'UserModel.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/models/UserModel.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'UserModel.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSUserModelENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSUserModelENDCLASS = QtMocHelpers::stringData(
    "UserModel",
    "userIdChanged",
    "",
    "usernameChanged",
    "emailChanged",
    "passwordChanged",
    "displayNameChanged",
    "avatarChanged",
    "statusChanged",
    "isLoggedInChanged",
    "tokenChanged",
    "lastOnlineChanged",
    "userInfoChanged",
    "clear",
    "isValid",
    "updateUserInfo",
    "userInfo",
    "toVariantMap",
    "userId",
    "username",
    "email",
    "password",
    "displayName",
    "avatar",
    "status",
    "token",
    "isLoggedIn",
    "lastOnline"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSUserModelENDCLASS_t {
    uint offsetsAndSizes[56];
    char stringdata0[10];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[16];
    char stringdata4[13];
    char stringdata5[16];
    char stringdata6[19];
    char stringdata7[14];
    char stringdata8[14];
    char stringdata9[18];
    char stringdata10[13];
    char stringdata11[18];
    char stringdata12[16];
    char stringdata13[6];
    char stringdata14[8];
    char stringdata15[15];
    char stringdata16[9];
    char stringdata17[13];
    char stringdata18[7];
    char stringdata19[9];
    char stringdata20[6];
    char stringdata21[9];
    char stringdata22[12];
    char stringdata23[7];
    char stringdata24[7];
    char stringdata25[6];
    char stringdata26[11];
    char stringdata27[11];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSUserModelENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSUserModelENDCLASS_t qt_meta_stringdata_CLASSUserModelENDCLASS = {
    {
        QT_MOC_LITERAL(0, 9),  // "UserModel"
        QT_MOC_LITERAL(10, 13),  // "userIdChanged"
        QT_MOC_LITERAL(24, 0),  // ""
        QT_MOC_LITERAL(25, 15),  // "usernameChanged"
        QT_MOC_LITERAL(41, 12),  // "emailChanged"
        QT_MOC_LITERAL(54, 15),  // "passwordChanged"
        QT_MOC_LITERAL(70, 18),  // "displayNameChanged"
        QT_MOC_LITERAL(89, 13),  // "avatarChanged"
        QT_MOC_LITERAL(103, 13),  // "statusChanged"
        QT_MOC_LITERAL(117, 17),  // "isLoggedInChanged"
        QT_MOC_LITERAL(135, 12),  // "tokenChanged"
        QT_MOC_LITERAL(148, 17),  // "lastOnlineChanged"
        QT_MOC_LITERAL(166, 15),  // "userInfoChanged"
        QT_MOC_LITERAL(182, 5),  // "clear"
        QT_MOC_LITERAL(188, 7),  // "isValid"
        QT_MOC_LITERAL(196, 14),  // "updateUserInfo"
        QT_MOC_LITERAL(211, 8),  // "userInfo"
        QT_MOC_LITERAL(220, 12),  // "toVariantMap"
        QT_MOC_LITERAL(233, 6),  // "userId"
        QT_MOC_LITERAL(240, 8),  // "username"
        QT_MOC_LITERAL(249, 5),  // "email"
        QT_MOC_LITERAL(255, 8),  // "password"
        QT_MOC_LITERAL(264, 11),  // "displayName"
        QT_MOC_LITERAL(276, 6),  // "avatar"
        QT_MOC_LITERAL(283, 6),  // "status"
        QT_MOC_LITERAL(290, 5),  // "token"
        QT_MOC_LITERAL(296, 10),  // "isLoggedIn"
        QT_MOC_LITERAL(307, 10)   // "lastOnline"
    },
    "UserModel",
    "userIdChanged",
    "",
    "usernameChanged",
    "emailChanged",
    "passwordChanged",
    "displayNameChanged",
    "avatarChanged",
    "statusChanged",
    "isLoggedInChanged",
    "tokenChanged",
    "lastOnlineChanged",
    "userInfoChanged",
    "clear",
    "isValid",
    "updateUserInfo",
    "userInfo",
    "toVariantMap",
    "userId",
    "username",
    "email",
    "password",
    "displayName",
    "avatar",
    "status",
    "token",
    "isLoggedIn",
    "lastOnline"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSUserModelENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      15,   14, // methods
      10,  121, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      11,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  104,    2, 0x06,   11 /* Public */,
       3,    0,  105,    2, 0x06,   12 /* Public */,
       4,    0,  106,    2, 0x06,   13 /* Public */,
       5,    0,  107,    2, 0x06,   14 /* Public */,
       6,    0,  108,    2, 0x06,   15 /* Public */,
       7,    0,  109,    2, 0x06,   16 /* Public */,
       8,    0,  110,    2, 0x06,   17 /* Public */,
       9,    0,  111,    2, 0x06,   18 /* Public */,
      10,    0,  112,    2, 0x06,   19 /* Public */,
      11,    0,  113,    2, 0x06,   20 /* Public */,
      12,    0,  114,    2, 0x06,   21 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      13,    0,  115,    2, 0x02,   22 /* Public */,
      14,    0,  116,    2, 0x102,   23 /* Public | MethodIsConst  */,
      15,    1,  117,    2, 0x02,   24 /* Public */,
      17,    0,  120,    2, 0x102,   26 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Bool,
    QMetaType::Void, QMetaType::QVariantMap,   16,
    QMetaType::QVariantMap,

 // properties: name, type, flags
      18, QMetaType::LongLong, 0x00015103, uint(0), 0,
      19, QMetaType::QString, 0x00015103, uint(1), 0,
      20, QMetaType::QString, 0x00015103, uint(2), 0,
      21, QMetaType::QString, 0x00015103, uint(3), 0,
      22, QMetaType::QString, 0x00015103, uint(4), 0,
      23, QMetaType::QUrl, 0x00015103, uint(5), 0,
      24, QMetaType::QString, 0x00015103, uint(6), 0,
      25, QMetaType::QString, 0x00015103, uint(8), 0,
      26, QMetaType::Bool, 0x00015103, uint(7), 0,
      27, QMetaType::QDateTime, 0x00015103, uint(9), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject UserModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSUserModelENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSUserModelENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSUserModelENDCLASS_t,
        // property 'userId'
        QtPrivate::TypeAndForceComplete<qint64, std::true_type>,
        // property 'username'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'email'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'password'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'displayName'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'avatar'
        QtPrivate::TypeAndForceComplete<QUrl, std::true_type>,
        // property 'status'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'token'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'isLoggedIn'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'lastOnline'
        QtPrivate::TypeAndForceComplete<QDateTime, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<UserModel, std::true_type>,
        // method 'userIdChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'usernameChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'emailChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'passwordChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'displayNameChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'avatarChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'statusChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'isLoggedInChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'tokenChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'lastOnlineChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'userInfoChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clear'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'isValid'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'updateUserInfo'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QVariantMap &, std::false_type>,
        // method 'toVariantMap'
        QtPrivate::TypeAndForceComplete<QVariantMap, std::false_type>
    >,
    nullptr
} };

void UserModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UserModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->userIdChanged(); break;
        case 1: _t->usernameChanged(); break;
        case 2: _t->emailChanged(); break;
        case 3: _t->passwordChanged(); break;
        case 4: _t->displayNameChanged(); break;
        case 5: _t->avatarChanged(); break;
        case 6: _t->statusChanged(); break;
        case 7: _t->isLoggedInChanged(); break;
        case 8: _t->tokenChanged(); break;
        case 9: _t->lastOnlineChanged(); break;
        case 10: _t->userInfoChanged(); break;
        case 11: _t->clear(); break;
        case 12: { bool _r = _t->isValid();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 13: _t->updateUserInfo((*reinterpret_cast< std::add_pointer_t<QVariantMap>>(_a[1]))); break;
        case 14: { QVariantMap _r = _t->toVariantMap();
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::userIdChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::usernameChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::emailChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::passwordChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::displayNameChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::avatarChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::statusChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::isLoggedInChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::tokenChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::lastOnlineChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::userInfoChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<UserModel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< qint64*>(_v) = _t->userId(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->username(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->email(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->password(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->displayName(); break;
        case 5: *reinterpret_cast< QUrl*>(_v) = _t->avatar(); break;
        case 6: *reinterpret_cast< QString*>(_v) = _t->status(); break;
        case 7: *reinterpret_cast< QString*>(_v) = _t->token(); break;
        case 8: *reinterpret_cast< bool*>(_v) = _t->isLoggedIn(); break;
        case 9: *reinterpret_cast< QDateTime*>(_v) = _t->lastOnline(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<UserModel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setUserId(*reinterpret_cast< qint64*>(_v)); break;
        case 1: _t->setUsername(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setEmail(*reinterpret_cast< QString*>(_v)); break;
        case 3: _t->setPassword(*reinterpret_cast< QString*>(_v)); break;
        case 4: _t->setDisplayName(*reinterpret_cast< QString*>(_v)); break;
        case 5: _t->setAvatar(*reinterpret_cast< QUrl*>(_v)); break;
        case 6: _t->setStatus(*reinterpret_cast< QString*>(_v)); break;
        case 7: _t->setToken(*reinterpret_cast< QString*>(_v)); break;
        case 8: _t->setIsLoggedIn(*reinterpret_cast< bool*>(_v)); break;
        case 9: _t->setLastOnline(*reinterpret_cast< QDateTime*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *UserModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UserModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSUserModelENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UserModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void UserModel::userIdChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void UserModel::usernameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void UserModel::emailChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void UserModel::passwordChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void UserModel::displayNameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void UserModel::avatarChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void UserModel::statusChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void UserModel::isLoggedInChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void UserModel::tokenChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void UserModel::lastOnlineChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void UserModel::userInfoChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}
QT_WARNING_POP
