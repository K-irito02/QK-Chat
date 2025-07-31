/****************************************************************************
** Meta object code from reading C++ file 'UserController.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/controllers/UserController.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'UserController.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSUserControllerENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSUserControllerENDCLASS = QtMocHelpers::stringData(
    "UserController",
    "isLoadingChanged",
    "",
    "errorMessageChanged",
    "loginAttemptsChanged",
    "needCaptchaChanged",
    "captchaImageChanged",
    "loginSuccess",
    "loginFailed",
    "error",
    "registerSuccess",
    "registerFailed",
    "logoutSuccess",
    "usernameValidationResult",
    "isValid",
    "emailValidationResult",
    "passwordValidationResult",
    "usernameAvailabilityResult",
    "isAvailable",
    "emailAvailabilityResult",
    "onLoginResponse",
    "success",
    "message",
    "token",
    "onRegisterResponse",
    "onNetworkError",
    "resetLoginAttempts",
    "login",
    "usernameOrEmail",
    "password",
    "captcha",
    "registerUser",
    "username",
    "email",
    "avatar",
    "logout",
    "refreshCaptcha",
    "validateUsername",
    "validateEmail",
    "validatePassword",
    "checkUsernameAvailability",
    "checkEmailAvailability",
    "getDefaultAvatars",
    "uploadCustomAvatar",
    "filePath",
    "tryAutoLogin",
    "saveLoginCredentials",
    "remember",
    "isLoading",
    "errorMessage",
    "loginAttempts",
    "needCaptcha",
    "captchaImage"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSUserControllerENDCLASS_t {
    uint offsetsAndSizes[106];
    char stringdata0[15];
    char stringdata1[17];
    char stringdata2[1];
    char stringdata3[20];
    char stringdata4[21];
    char stringdata5[19];
    char stringdata6[20];
    char stringdata7[13];
    char stringdata8[12];
    char stringdata9[6];
    char stringdata10[16];
    char stringdata11[15];
    char stringdata12[14];
    char stringdata13[25];
    char stringdata14[8];
    char stringdata15[22];
    char stringdata16[25];
    char stringdata17[27];
    char stringdata18[12];
    char stringdata19[24];
    char stringdata20[16];
    char stringdata21[8];
    char stringdata22[8];
    char stringdata23[6];
    char stringdata24[19];
    char stringdata25[15];
    char stringdata26[19];
    char stringdata27[6];
    char stringdata28[16];
    char stringdata29[9];
    char stringdata30[8];
    char stringdata31[13];
    char stringdata32[9];
    char stringdata33[6];
    char stringdata34[7];
    char stringdata35[7];
    char stringdata36[15];
    char stringdata37[17];
    char stringdata38[14];
    char stringdata39[17];
    char stringdata40[26];
    char stringdata41[23];
    char stringdata42[18];
    char stringdata43[19];
    char stringdata44[9];
    char stringdata45[13];
    char stringdata46[21];
    char stringdata47[9];
    char stringdata48[10];
    char stringdata49[13];
    char stringdata50[14];
    char stringdata51[12];
    char stringdata52[13];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSUserControllerENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSUserControllerENDCLASS_t qt_meta_stringdata_CLASSUserControllerENDCLASS = {
    {
        QT_MOC_LITERAL(0, 14),  // "UserController"
        QT_MOC_LITERAL(15, 16),  // "isLoadingChanged"
        QT_MOC_LITERAL(32, 0),  // ""
        QT_MOC_LITERAL(33, 19),  // "errorMessageChanged"
        QT_MOC_LITERAL(53, 20),  // "loginAttemptsChanged"
        QT_MOC_LITERAL(74, 18),  // "needCaptchaChanged"
        QT_MOC_LITERAL(93, 19),  // "captchaImageChanged"
        QT_MOC_LITERAL(113, 12),  // "loginSuccess"
        QT_MOC_LITERAL(126, 11),  // "loginFailed"
        QT_MOC_LITERAL(138, 5),  // "error"
        QT_MOC_LITERAL(144, 15),  // "registerSuccess"
        QT_MOC_LITERAL(160, 14),  // "registerFailed"
        QT_MOC_LITERAL(175, 13),  // "logoutSuccess"
        QT_MOC_LITERAL(189, 24),  // "usernameValidationResult"
        QT_MOC_LITERAL(214, 7),  // "isValid"
        QT_MOC_LITERAL(222, 21),  // "emailValidationResult"
        QT_MOC_LITERAL(244, 24),  // "passwordValidationResult"
        QT_MOC_LITERAL(269, 26),  // "usernameAvailabilityResult"
        QT_MOC_LITERAL(296, 11),  // "isAvailable"
        QT_MOC_LITERAL(308, 23),  // "emailAvailabilityResult"
        QT_MOC_LITERAL(332, 15),  // "onLoginResponse"
        QT_MOC_LITERAL(348, 7),  // "success"
        QT_MOC_LITERAL(356, 7),  // "message"
        QT_MOC_LITERAL(364, 5),  // "token"
        QT_MOC_LITERAL(370, 18),  // "onRegisterResponse"
        QT_MOC_LITERAL(389, 14),  // "onNetworkError"
        QT_MOC_LITERAL(404, 18),  // "resetLoginAttempts"
        QT_MOC_LITERAL(423, 5),  // "login"
        QT_MOC_LITERAL(429, 15),  // "usernameOrEmail"
        QT_MOC_LITERAL(445, 8),  // "password"
        QT_MOC_LITERAL(454, 7),  // "captcha"
        QT_MOC_LITERAL(462, 12),  // "registerUser"
        QT_MOC_LITERAL(475, 8),  // "username"
        QT_MOC_LITERAL(484, 5),  // "email"
        QT_MOC_LITERAL(490, 6),  // "avatar"
        QT_MOC_LITERAL(497, 6),  // "logout"
        QT_MOC_LITERAL(504, 14),  // "refreshCaptcha"
        QT_MOC_LITERAL(519, 16),  // "validateUsername"
        QT_MOC_LITERAL(536, 13),  // "validateEmail"
        QT_MOC_LITERAL(550, 16),  // "validatePassword"
        QT_MOC_LITERAL(567, 25),  // "checkUsernameAvailability"
        QT_MOC_LITERAL(593, 22),  // "checkEmailAvailability"
        QT_MOC_LITERAL(616, 17),  // "getDefaultAvatars"
        QT_MOC_LITERAL(634, 18),  // "uploadCustomAvatar"
        QT_MOC_LITERAL(653, 8),  // "filePath"
        QT_MOC_LITERAL(662, 12),  // "tryAutoLogin"
        QT_MOC_LITERAL(675, 20),  // "saveLoginCredentials"
        QT_MOC_LITERAL(696, 8),  // "remember"
        QT_MOC_LITERAL(705, 9),  // "isLoading"
        QT_MOC_LITERAL(715, 12),  // "errorMessage"
        QT_MOC_LITERAL(728, 13),  // "loginAttempts"
        QT_MOC_LITERAL(742, 11),  // "needCaptcha"
        QT_MOC_LITERAL(754, 12)   // "captchaImage"
    },
    "UserController",
    "isLoadingChanged",
    "",
    "errorMessageChanged",
    "loginAttemptsChanged",
    "needCaptchaChanged",
    "captchaImageChanged",
    "loginSuccess",
    "loginFailed",
    "error",
    "registerSuccess",
    "registerFailed",
    "logoutSuccess",
    "usernameValidationResult",
    "isValid",
    "emailValidationResult",
    "passwordValidationResult",
    "usernameAvailabilityResult",
    "isAvailable",
    "emailAvailabilityResult",
    "onLoginResponse",
    "success",
    "message",
    "token",
    "onRegisterResponse",
    "onNetworkError",
    "resetLoginAttempts",
    "login",
    "usernameOrEmail",
    "password",
    "captcha",
    "registerUser",
    "username",
    "email",
    "avatar",
    "logout",
    "refreshCaptcha",
    "validateUsername",
    "validateEmail",
    "validatePassword",
    "checkUsernameAvailability",
    "checkEmailAvailability",
    "getDefaultAvatars",
    "uploadCustomAvatar",
    "filePath",
    "tryAutoLogin",
    "saveLoginCredentials",
    "remember",
    "isLoading",
    "errorMessage",
    "loginAttempts",
    "needCaptcha",
    "captchaImage"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSUserControllerENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
      33,   14, // methods
       5,  313, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      15,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  212,    2, 0x06,    6 /* Public */,
       3,    0,  213,    2, 0x06,    7 /* Public */,
       4,    0,  214,    2, 0x06,    8 /* Public */,
       5,    0,  215,    2, 0x06,    9 /* Public */,
       6,    0,  216,    2, 0x06,   10 /* Public */,
       7,    0,  217,    2, 0x06,   11 /* Public */,
       8,    1,  218,    2, 0x06,   12 /* Public */,
      10,    0,  221,    2, 0x06,   14 /* Public */,
      11,    1,  222,    2, 0x06,   15 /* Public */,
      12,    0,  225,    2, 0x06,   17 /* Public */,
      13,    2,  226,    2, 0x06,   18 /* Public */,
      15,    2,  231,    2, 0x06,   21 /* Public */,
      16,    2,  236,    2, 0x06,   24 /* Public */,
      17,    1,  241,    2, 0x06,   27 /* Public */,
      19,    1,  244,    2, 0x06,   29 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      20,    3,  247,    2, 0x08,   31 /* Private */,
      24,    2,  254,    2, 0x08,   35 /* Private */,
      25,    1,  259,    2, 0x08,   38 /* Private */,
      26,    0,  262,    2, 0x08,   40 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      27,    3,  263,    2, 0x02,   41 /* Public */,
      27,    2,  270,    2, 0x22,   45 /* Public | MethodCloned */,
      31,    4,  275,    2, 0x02,   48 /* Public */,
      35,    0,  284,    2, 0x02,   53 /* Public */,
      36,    0,  285,    2, 0x02,   54 /* Public */,
      37,    1,  286,    2, 0x02,   55 /* Public */,
      38,    1,  289,    2, 0x02,   57 /* Public */,
      39,    1,  292,    2, 0x02,   59 /* Public */,
      40,    1,  295,    2, 0x02,   61 /* Public */,
      41,    1,  298,    2, 0x02,   63 /* Public */,
      42,    0,  301,    2, 0x102,   65 /* Public | MethodIsConst  */,
      43,    1,  302,    2, 0x02,   66 /* Public */,
      45,    0,  305,    2, 0x02,   68 /* Public */,
      46,    3,  306,    2, 0x02,   69 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   14,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   14,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   14,    9,
    QMetaType::Void, QMetaType::Bool,   18,
    QMetaType::Void, QMetaType::Bool,   18,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool, QMetaType::QString, QMetaType::QString,   21,   22,   23,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   21,   22,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,   28,   29,   30,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   28,   29,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QUrl,   32,   33,   29,   34,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QString,   32,
    QMetaType::Bool, QMetaType::QString,   33,
    QMetaType::Bool, QMetaType::QString,   29,
    QMetaType::Bool, QMetaType::QString,   32,
    QMetaType::Bool, QMetaType::QString,   33,
    QMetaType::QStringList,
    QMetaType::Bool, QMetaType::QUrl,   44,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::Bool,   32,   29,   47,

 // properties: name, type, flags
      48, QMetaType::Bool, 0x00015001, uint(0), 0,
      49, QMetaType::QString, 0x00015001, uint(1), 0,
      50, QMetaType::Int, 0x00015001, uint(2), 0,
      51, QMetaType::Bool, 0x00015001, uint(3), 0,
      52, QMetaType::QString, 0x00015001, uint(4), 0,

       0        // eod
};

Q_CONSTINIT const QMetaObject UserController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_CLASSUserControllerENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSUserControllerENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSUserControllerENDCLASS_t,
        // property 'isLoading'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'errorMessage'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // property 'loginAttempts'
        QtPrivate::TypeAndForceComplete<int, std::true_type>,
        // property 'needCaptcha'
        QtPrivate::TypeAndForceComplete<bool, std::true_type>,
        // property 'captchaImage'
        QtPrivate::TypeAndForceComplete<QString, std::true_type>,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<UserController, std::true_type>,
        // method 'isLoadingChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'errorMessageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loginAttemptsChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'needCaptchaChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'captchaImageChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loginSuccess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'loginFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'registerSuccess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'registerFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'logoutSuccess'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'usernameValidationResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'emailValidationResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'passwordValidationResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'usernameAvailabilityResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'emailAvailabilityResult'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onLoginResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onRegisterResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onNetworkError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resetLoginAttempts'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'login'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'login'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'registerUser'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'logout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshCaptcha'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'validateUsername'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'validateEmail'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'validatePassword'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'checkUsernameAvailability'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'checkEmailAvailability'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'getDefaultAvatars'
        QtPrivate::TypeAndForceComplete<QStringList, std::false_type>,
        // method 'uploadCustomAvatar'
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'tryAutoLogin'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'saveLoginCredentials'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>
    >,
    nullptr
} };

void UserController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UserController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->isLoadingChanged(); break;
        case 1: _t->errorMessageChanged(); break;
        case 2: _t->loginAttemptsChanged(); break;
        case 3: _t->needCaptchaChanged(); break;
        case 4: _t->captchaImageChanged(); break;
        case 5: _t->loginSuccess(); break;
        case 6: _t->loginFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->registerSuccess(); break;
        case 8: _t->registerFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->logoutSuccess(); break;
        case 10: _t->usernameValidationResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 11: _t->emailValidationResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->passwordValidationResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 13: _t->usernameAvailabilityResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 14: _t->emailAvailabilityResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 15: _t->onLoginResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 16: _t->onRegisterResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 17: _t->onNetworkError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 18: _t->resetLoginAttempts(); break;
        case 19: _t->login((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 20: _t->login((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 21: _t->registerUser((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[4]))); break;
        case 22: _t->logout(); break;
        case 23: _t->refreshCaptcha(); break;
        case 24: { bool _r = _t->validateUsername((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 25: { bool _r = _t->validateEmail((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 26: { bool _r = _t->validatePassword((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 27: { bool _r = _t->checkUsernameAvailability((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 28: { bool _r = _t->checkEmailAvailability((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 29: { QStringList _r = _t->getDefaultAvatars();
            if (_a[0]) *reinterpret_cast< QStringList*>(_a[0]) = std::move(_r); }  break;
        case 30: { bool _r = _t->uploadCustomAvatar((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 31: _t->tryAutoLogin(); break;
        case 32: _t->saveLoginCredentials((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::isLoadingChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::errorMessageChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::loginAttemptsChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::needCaptchaChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::captchaImageChanged; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::loginSuccess; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (UserController::*)(const QString & );
            if (_t _q_method = &UserController::loginFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 6;
                return;
            }
        }
        {
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::registerSuccess; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 7;
                return;
            }
        }
        {
            using _t = void (UserController::*)(const QString & );
            if (_t _q_method = &UserController::registerFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 8;
                return;
            }
        }
        {
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::logoutSuccess; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 9;
                return;
            }
        }
        {
            using _t = void (UserController::*)(bool , const QString & );
            if (_t _q_method = &UserController::usernameValidationResult; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 10;
                return;
            }
        }
        {
            using _t = void (UserController::*)(bool , const QString & );
            if (_t _q_method = &UserController::emailValidationResult; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 11;
                return;
            }
        }
        {
            using _t = void (UserController::*)(bool , const QString & );
            if (_t _q_method = &UserController::passwordValidationResult; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 12;
                return;
            }
        }
        {
            using _t = void (UserController::*)(bool );
            if (_t _q_method = &UserController::usernameAvailabilityResult; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 13;
                return;
            }
        }
        {
            using _t = void (UserController::*)(bool );
            if (_t _q_method = &UserController::emailAvailabilityResult; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 14;
                return;
            }
        }
    }else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<UserController *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< bool*>(_v) = _t->isLoading(); break;
        case 1: *reinterpret_cast< QString*>(_v) = _t->errorMessage(); break;
        case 2: *reinterpret_cast< int*>(_v) = _t->loginAttempts(); break;
        case 3: *reinterpret_cast< bool*>(_v) = _t->needCaptcha(); break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->captchaImage(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
}

const QMetaObject *UserController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *UserController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSUserControllerENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int UserController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 33)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 33;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 33)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 33;
    }else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void UserController::isLoadingChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void UserController::errorMessageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void UserController::loginAttemptsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void UserController::needCaptchaChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void UserController::captchaImageChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void UserController::loginSuccess()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void UserController::loginFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}

// SIGNAL 7
void UserController::registerSuccess()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void UserController::registerFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 8, _a);
}

// SIGNAL 9
void UserController::logoutSuccess()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void UserController::usernameValidationResult(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 10, _a);
}

// SIGNAL 11
void UserController::emailValidationResult(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 11, _a);
}

// SIGNAL 12
void UserController::passwordValidationResult(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 12, _a);
}

// SIGNAL 13
void UserController::usernameAvailabilityResult(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 13, _a);
}

// SIGNAL 14
void UserController::emailAvailabilityResult(bool _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 14, _a);
}
QT_WARNING_POP
