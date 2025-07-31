/****************************************************************************
** Meta object code from reading C++ file 'Validator.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/utils/Validator.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'Validator.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSValidatorENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSValidatorENDCLASS = QtMocHelpers::stringData(
    "Validator",
    "isValidUsername",
    "",
    "username",
    "getUsernameError",
    "isValidEmail",
    "email",
    "getEmailError",
    "isValidPassword",
    "password",
    "getPasswordError",
    "isPasswordMatched",
    "confirmPassword",
    "isValidVerificationCode",
    "code",
    "isValidImageFile",
    "filePath",
    "isValidImageSize",
    "maxSizeMB"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSValidatorENDCLASS_t {
    uint offsetsAndSizes[38];
    char stringdata0[10];
    char stringdata1[16];
    char stringdata2[1];
    char stringdata3[9];
    char stringdata4[17];
    char stringdata5[13];
    char stringdata6[6];
    char stringdata7[14];
    char stringdata8[16];
    char stringdata9[9];
    char stringdata10[17];
    char stringdata11[18];
    char stringdata12[16];
    char stringdata13[24];
    char stringdata14[5];
    char stringdata15[17];
    char stringdata16[9];
    char stringdata17[17];
    char stringdata18[10];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSValidatorENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSValidatorENDCLASS_t qt_meta_stringdata_CLASSValidatorENDCLASS = {
    {
        QT_MOC_LITERAL(0, 9),  // "Validator"
        QT_MOC_LITERAL(10, 15),  // "isValidUsername"
        QT_MOC_LITERAL(26, 0),  // ""
        QT_MOC_LITERAL(27, 8),  // "username"
        QT_MOC_LITERAL(36, 16),  // "getUsernameError"
        QT_MOC_LITERAL(53, 12),  // "isValidEmail"
        QT_MOC_LITERAL(66, 5),  // "email"
        QT_MOC_LITERAL(72, 13),  // "getEmailError"
        QT_MOC_LITERAL(86, 15),  // "isValidPassword"
        QT_MOC_LITERAL(102, 8),  // "password"
        QT_MOC_LITERAL(111, 16),  // "getPasswordError"
        QT_MOC_LITERAL(128, 17),  // "isPasswordMatched"
        QT_MOC_LITERAL(146, 15),  // "confirmPassword"
        QT_MOC_LITERAL(162, 23),  // "isValidVerificationCode"
        QT_MOC_LITERAL(186, 4),  // "code"
        QT_MOC_LITERAL(191, 16),  // "isValidImageFile"
        QT_MOC_LITERAL(208, 8),  // "filePath"
        QT_MOC_LITERAL(217, 16),  // "isValidImageSize"
        QT_MOC_LITERAL(234, 9)   // "maxSizeMB"
    },
    "Validator",
    "isValidUsername",
    "",
    "username",
    "getUsernameError",
    "isValidEmail",
    "email",
    "getEmailError",
    "isValidPassword",
    "password",
    "getPasswordError",
    "isPasswordMatched",
    "confirmPassword",
    "isValidVerificationCode",
    "code",
    "isValidImageFile",
    "filePath",
    "isValidImageSize",
    "maxSizeMB"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSValidatorENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      11,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   80,    2, 0x102,    1 /* Public | MethodIsConst  */,
       4,    1,   83,    2, 0x102,    3 /* Public | MethodIsConst  */,
       5,    1,   86,    2, 0x102,    5 /* Public | MethodIsConst  */,
       7,    1,   89,    2, 0x102,    7 /* Public | MethodIsConst  */,
       8,    1,   92,    2, 0x102,    9 /* Public | MethodIsConst  */,
      10,    1,   95,    2, 0x102,   11 /* Public | MethodIsConst  */,
      11,    2,   98,    2, 0x102,   13 /* Public | MethodIsConst  */,
      13,    1,  103,    2, 0x102,   16 /* Public | MethodIsConst  */,
      15,    1,  106,    2, 0x102,   18 /* Public | MethodIsConst  */,
      17,    2,  109,    2, 0x102,   20 /* Public | MethodIsConst  */,
      17,    1,  114,    2, 0x122,   23 /* Public | MethodCloned | MethodIsConst  */,

 // methods: parameters
    QMetaType::Bool, QMetaType::QString,    3,
    QMetaType::QString, QMetaType::QString,    3,
    QMetaType::Bool, QMetaType::QString,    6,
    QMetaType::QString, QMetaType::QString,    6,
    QMetaType::Bool, QMetaType::QString,    9,
    QMetaType::QString, QMetaType::QString,    9,
    QMetaType::Bool, QMetaType::QString, QMetaType::QString,    9,   12,
    QMetaType::Bool, QMetaType::QString,   14,
    QMetaType::Bool, QMetaType::QString,   16,
    QMetaType::Bool, QMetaType::QString, QMetaType::Int,   16,   18,
    QMetaType::Bool, QMetaType::QString,   16,

       0        // eod
};

Q_CONSTINIT const QMetaObject Validator::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSValidatorENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSValidatorENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSValidatorENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<Validator, std::true_type>,
        // method 'isValidUsername'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'getUsernameError'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'isValidEmail'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'getEmailError'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'isValidPassword'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'getPasswordError'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'isPasswordMatched'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'isValidVerificationCode'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'isValidImageFile'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'isValidImageSize'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'isValidImageSize'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>
    >,
    nullptr
} };

void Validator::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Validator *>(_o);
        (void)_t;
        switch (_id) {
        case 0: { bool _r = _t->isValidUsername((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 1: { QString _r = _t->getUsernameError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 2: { bool _r = _t->isValidEmail((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 3: { QString _r = _t->getEmailError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 4: { bool _r = _t->isValidPassword((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 5: { QString _r = _t->getPasswordError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 6: { bool _r = _t->isPasswordMatched((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 7: { bool _r = _t->isValidVerificationCode((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 8: { bool _r = _t->isValidImageFile((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 9: { bool _r = _t->isValidImageSize((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 10: { bool _r = _t->isValidImageSize((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
}

const QMetaObject *Validator::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Validator::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSValidatorENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Validator::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    return _id;
}
QT_WARNING_POP
