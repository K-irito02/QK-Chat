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
    "usernameChanged",
    "",
    "emailChanged",
    "passwordChanged",
    "avatarChanged",
    "isLoggedInChanged",
    "tokenChanged",
    "clear",
    "isValid",
    "username",
    "email",
    "password",
    "avatar",
    "isLoggedIn",
    "token"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSUserModelENDCLASS_t {
    uint offsetsAndSizes[32];
    char stringdata0[10];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[13];
    char stringdata4[16];
    char stringdata5[14];
    char stringdata6[18];
    char stringdata7[13];
    char stringdata8[6];
    char stringdata9[8];
    char stringdata10[9];
    char stringdata11[6];
    char stringdata12[9];
    char stringdata13[7];
    char stringdata14[11];
    char stringdata15[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSUserModelENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSUserModelENDCLASS_t qt_meta_stringdata_CLASSUserModelENDCLASS = {
    {
        QT_MOC_LITERAL(0, 9),  // "UserModel"
        QT_MOC_LITERAL(10, 15),  // "usernameChanged"
        QT_MOC_LITERAL(26, 0),  // ""
        QT_MOC_LITERAL(27, 12),  // "emailChanged"
        QT_MOC_LITERAL(40, 15),  // "passwordChanged"
        QT_MOC_LITERAL(56, 13),  // "avatarChanged"
        QT_MOC_LITERAL(70, 17),  // "isLoggedInChanged"
        QT_MOC_LITERAL(88, 12),  // "tokenChanged"
        QT_MOC_LITERAL(101, 5),  // "clear"
        QT_MOC_LITERAL(107, 7),  // "isValid"
        QT_MOC_LITERAL(115, 8),  // "username"
        QT_MOC_LITERAL(124, 5),  // "email"
        QT_MOC_LITERAL(130, 8),  // "password"
        QT_MOC_LITERAL(139, 6),  // "avatar"
        QT_MOC_LITERAL(146, 10),  // "isLoggedIn"
        QT_MOC_LITERAL(157, 5)   // "token"
    },
    "UserModel",
    "usernameChanged",
    "",
    "emailChanged",
    "passwordChanged",
    "avatarChanged",
    "isLoggedInChanged",
    "tokenChanged",
    "clear",
    "isValid",
    "username",
    "email",
    "password",
    "avatar",
    "isLoggedIn",
    "token"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSUserModelENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       6,   70, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x06,    7 /* Public */,
       3,    0,   63,    2, 0x06,    8 /* Public */,
       4,    0,   64,    2, 0x06,    9 /* Public */,
       5,    0,   65,    2, 0x06,   10 /* Public */,
       6,    0,   66,    2, 0x06,   11 /* Public */,
       7,    0,   67,    2, 0x06,   12 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       8,    0,   68,    2, 0x02,   13 /* Public */,
       9,    0,   69,    2, 0x102,   14 /* Public | MethodIsConst  */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void,
    QMetaType::Bool,

 // properties: name, type, flags
      10, QMetaType::QString, 0x00015103, uint(0), 0,
      11, QMetaType::QString, 0x00015103, uint(1), 0,
      12, QMetaType::QString, 0x00015103, uint(2), 0,
      13, QMetaType::QUrl, 0x00015103, uint(3), 0,
      14, QMetaType::Bool, 0x00015103, uint(4), 0,
      15, QMetaType::QString, 0x00015103, uint(5), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject UserModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSUserModelENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSUserModelENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSUserModelENDCLASS_t,
        // property 'username'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'email'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'password'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'avatar'
        QtPrivate::TypeAndForceComplete<QUrl, std::true_type>,
        // property 'isLoggedIn'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'token'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<UserModel, std::true_type>,
        // method 'usernameChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'emailChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'passwordChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'avatarChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'isLoggedInChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'tokenChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'clear'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'isValid'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void UserModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UserModel *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->usernameChanged(); break;
        case 1: _t->emailChanged(); break;
        case 2: _t->passwordChanged(); break;
        case 3: _t->avatarChanged(); break;
        case 4: _t->isLoggedInChanged(); break;
        case 5: _t->tokenChanged(); break;
        case 6: _t->clear(); break;
        case 7: { bool _r = _t->isValid();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::usernameChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::emailChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::passwordChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::avatarChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::isLoggedInChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (UserModel::*)();
            if (_t _q_method = &UserModel::tokenChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<UserModel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->username(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->email(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->password(); break;
        case 3: *reinterpret_cast< QUrl*>(_v) = _t->avatar(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->isLoggedIn(); break;
        case 5: *reinterpret_cast< QString*>(_v) = _t->token(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<UserModel *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setUsername(*reinterpret_cast< QString*>(_v)); break;
        case 1: _t->setEmail(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setPassword(*reinterpret_cast< QString*>(_v)); break;
        case 3: _t->setAvatar(*reinterpret_cast< QUrl*>(_v)); break;
        case 4: _t->setIsLoggedIn(*reinterpret_cast< bool*>(_v)); break;
        case 5: _t->setToken(*reinterpret_cast< QString*>(_v)); break;
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
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 8;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void UserModel::usernameChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void UserModel::emailChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void UserModel::passwordChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void UserModel::avatarChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void UserModel::isLoggedInChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void UserModel::tokenChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
