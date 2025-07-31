/****************************************************************************
** Meta object code from reading C++ file 'ConfigManager.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/config/ConfigManager.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ConfigManager.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSConfigManagerENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSConfigManagerENDCLASS = QtMocHelpers::stringData(
    "ConfigManager",
    "isDarkThemeChanged",
    "",
    "primaryColorChanged",
    "accentColorChanged",
    "languageChanged",
    "rememberPasswordChanged",
    "autoLoginChanged",
    "configLoaded",
    "configSaved",
    "loadConfig",
    "saveConfig",
    "resetToDefault",
    "getServerHost",
    "getServerPort",
    "setServerConfig",
    "host",
    "port",
    "saveUserCredentials",
    "username",
    "password",
    "loadUsername",
    "loadPassword",
    "clearUserCredentials",
    "isDarkTheme",
    "primaryColor",
    "accentColor",
    "language",
    "rememberPassword",
    "autoLogin"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSConfigManagerENDCLASS_t {
    uint offsetsAndSizes[60];
    char stringdata0[14];
    char stringdata1[19];
    char stringdata2[1];
    char stringdata3[20];
    char stringdata4[19];
    char stringdata5[16];
    char stringdata6[24];
    char stringdata7[17];
    char stringdata8[13];
    char stringdata9[12];
    char stringdata10[11];
    char stringdata11[11];
    char stringdata12[15];
    char stringdata13[14];
    char stringdata14[14];
    char stringdata15[16];
    char stringdata16[5];
    char stringdata17[5];
    char stringdata18[20];
    char stringdata19[9];
    char stringdata20[9];
    char stringdata21[13];
    char stringdata22[13];
    char stringdata23[21];
    char stringdata24[12];
    char stringdata25[13];
    char stringdata26[12];
    char stringdata27[9];
    char stringdata28[17];
    char stringdata29[10];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSConfigManagerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSConfigManagerENDCLASS_t qt_meta_stringdata_CLASSConfigManagerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 13),  // "ConfigManager"
        QT_MOC_LITERAL(14, 18),  // "isDarkThemeChanged"
        QT_MOC_LITERAL(33, 0),  // ""
        QT_MOC_LITERAL(34, 19),  // "primaryColorChanged"
        QT_MOC_LITERAL(54, 18),  // "accentColorChanged"
        QT_MOC_LITERAL(73, 15),  // "languageChanged"
        QT_MOC_LITERAL(89, 23),  // "rememberPasswordChanged"
        QT_MOC_LITERAL(113, 16),  // "autoLoginChanged"
        QT_MOC_LITERAL(130, 12),  // "configLoaded"
        QT_MOC_LITERAL(143, 11),  // "configSaved"
        QT_MOC_LITERAL(155, 10),  // "loadConfig"
        QT_MOC_LITERAL(166, 10),  // "saveConfig"
        QT_MOC_LITERAL(177, 14),  // "resetToDefault"
        QT_MOC_LITERAL(192, 13),  // "getServerHost"
        QT_MOC_LITERAL(206, 13),  // "getServerPort"
        QT_MOC_LITERAL(220, 15),  // "setServerConfig"
        QT_MOC_LITERAL(236, 4),  // "host"
        QT_MOC_LITERAL(241, 4),  // "port"
        QT_MOC_LITERAL(246, 19),  // "saveUserCredentials"
        QT_MOC_LITERAL(266, 8),  // "username"
        QT_MOC_LITERAL(275, 8),  // "password"
        QT_MOC_LITERAL(284, 12),  // "loadUsername"
        QT_MOC_LITERAL(297, 12),  // "loadPassword"
        QT_MOC_LITERAL(310, 20),  // "clearUserCredentials"
        QT_MOC_LITERAL(331, 11),  // "isDarkTheme"
        QT_MOC_LITERAL(343, 12),  // "primaryColor"
        QT_MOC_LITERAL(356, 11),  // "accentColor"
        QT_MOC_LITERAL(368, 8),  // "language"
        QT_MOC_LITERAL(377, 16),  // "rememberPassword"
        QT_MOC_LITERAL(394, 9)   // "autoLogin"
    },
    "ConfigManager",
    "isDarkThemeChanged",
    "",
    "primaryColorChanged",
    "accentColorChanged",
    "languageChanged",
    "rememberPasswordChanged",
    "autoLoginChanged",
    "configLoaded",
    "configSaved",
    "loadConfig",
    "saveConfig",
    "resetToDefault",
    "getServerHost",
    "getServerPort",
    "setServerConfig",
    "host",
    "port",
    "saveUserCredentials",
    "username",
    "password",
    "loadUsername",
    "loadPassword",
    "clearUserCredentials",
    "isDarkTheme",
    "primaryColor",
    "accentColor",
    "language",
    "rememberPassword",
    "autoLogin"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSConfigManagerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      18,   14, // methods
       6,  148, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       8,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  122,    2, 0x06,    7 /* Public */,
       3,    0,  123,    2, 0x06,    8 /* Public */,
       4,    0,  124,    2, 0x06,    9 /* Public */,
       5,    0,  125,    2, 0x06,   10 /* Public */,
       6,    0,  126,    2, 0x06,   11 /* Public */,
       7,    0,  127,    2, 0x06,   12 /* Public */,
       8,    0,  128,    2, 0x06,   13 /* Public */,
       9,    0,  129,    2, 0x06,   14 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      10,    0,  130,    2, 0x02,   15 /* Public */,
      11,    0,  131,    2, 0x02,   16 /* Public */,
      12,    0,  132,    2, 0x02,   17 /* Public */,
      13,    0,  133,    2, 0x102,   18 /* Public | MethodIsConst  */,
      14,    0,  134,    2, 0x102,   19 /* Public | MethodIsConst  */,
      15,    2,  135,    2, 0x02,   20 /* Public */,
      18,    2,  140,    2, 0x02,   23 /* Public */,
      21,    0,  145,    2, 0x02,   26 /* Public */,
      22,    0,  146,    2, 0x02,   27 /* Public */,
      23,    0,  147,    2, 0x02,   28 /* Public */,

 // signals: parameters
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
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::QString,
    QMetaType::Int,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   16,   17,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   19,   20,
    QMetaType::QString,
    QMetaType::QString,
    QMetaType::Void,

 // properties: name, type, flags
      24, QMetaType::Bool, 0x00015103, uint(0), 0,
      25, QMetaType::QString, 0x00015103, uint(1), 0,
      26, QMetaType::QString, 0x00015103, uint(2), 0,
      27, QMetaType::QString, 0x00015103, uint(3), 0,
      28, QMetaType::Bool, 0x00015103, uint(4), 0,
      29, QMetaType::Bool, 0x00015103, uint(5), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject ConfigManager::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSConfigManagerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSConfigManagerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSConfigManagerENDCLASS_t,
        // property 'isDarkTheme'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'primaryColor'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'accentColor'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'language'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'rememberPassword'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'autoLogin'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<ConfigManager, std::true_type>,
        // method 'isDarkThemeChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'primaryColorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'accentColorChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'languageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'rememberPasswordChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'autoLoginChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'configLoaded'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'configSaved'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loadConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'resetToDefault'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'getServerHost'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'getServerPort'
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'setServerConfig'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'saveUserCredentials'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'loadUsername'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'loadPassword'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
        // method 'clearUserCredentials'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void ConfigManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<ConfigManager *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->isDarkThemeChanged(); break;
        case 1: _t->primaryColorChanged(); break;
        case 2: _t->accentColorChanged(); break;
        case 3: _t->languageChanged(); break;
        case 4: _t->rememberPasswordChanged(); break;
        case 5: _t->autoLoginChanged(); break;
        case 6: _t->configLoaded(); break;
        case 7: _t->configSaved(); break;
        case 8: _t->loadConfig(); break;
        case 9: _t->saveConfig(); break;
        case 10: _t->resetToDefault(); break;
        case 11: { QString _r = _t->getServerHost();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 12: { int _r = _t->getServerPort();
            if (_a[0]) *reinterpret_cast< int*>(_a[0]) = std::move(_r); }  break;
        case 13: _t->setServerConfig((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 14: _t->saveUserCredentials((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 15: { QString _r = _t->loadUsername();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 16: { QString _r = _t->loadPassword();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 17: _t->clearUserCredentials(); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (ConfigManager::*)();
            if (_t _q_method = &ConfigManager::isDarkThemeChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (ConfigManager::*)();
            if (_t _q_method = &ConfigManager::primaryColorChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (ConfigManager::*)();
            if (_t _q_method = &ConfigManager::accentColorChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (ConfigManager::*)();
            if (_t _q_method = &ConfigManager::languageChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (ConfigManager::*)();
            if (_t _q_method = &ConfigManager::rememberPasswordChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (ConfigManager::*)();
            if (_t _q_method = &ConfigManager::autoLoginChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (ConfigManager::*)();
            if (_t _q_method = &ConfigManager::configLoaded; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (ConfigManager::*)();
            if (_t _q_method = &ConfigManager::configSaved; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<ConfigManager *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isDarkTheme(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->primaryColor(); break;
        case 2: *reinterpret_cast< QString*>(_v) = _t->accentColor(); break;
        case 3: *reinterpret_cast< QString*>(_v) = _t->language(); break;
        case 4: *reinterpret_cast< bool*>(_v) = _t->rememberPassword(); break;
        case 5: *reinterpret_cast< bool*>(_v) = _t->autoLogin(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<ConfigManager *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setIsDarkTheme(*reinterpret_cast< bool*>(_v)); break;
        case 1: _t->setPrimaryColor(*reinterpret_cast< QString*>(_v)); break;
        case 2: _t->setAccentColor(*reinterpret_cast< QString*>(_v)); break;
        case 3: _t->setLanguage(*reinterpret_cast< QString*>(_v)); break;
        case 4: _t->setRememberPassword(*reinterpret_cast< bool*>(_v)); break;
        case 5: _t->setAutoLogin(*reinterpret_cast< bool*>(_v)); break;
        default: break;
        }
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *ConfigManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ConfigManager::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSConfigManagerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int ConfigManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 18)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 18;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 18)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 18;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void ConfigManager::isDarkThemeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void ConfigManager::primaryColorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void ConfigManager::accentColorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void ConfigManager::languageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void ConfigManager::rememberPasswordChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void ConfigManager::autoLoginChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void ConfigManager::configLoaded()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void ConfigManager::configSaved()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP
