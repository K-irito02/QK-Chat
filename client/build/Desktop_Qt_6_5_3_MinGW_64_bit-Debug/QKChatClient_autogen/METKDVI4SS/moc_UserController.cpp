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
    "username",
    "userId",
    "registerFailed",
    "logoutSuccess",
    "usernameValidationResult",
    "isValid",
    "emailValidationResult",
    "passwordValidationResult",
    "usernameAvailabilityResult",
    "isAvailable",
    "emailAvailabilityResult",
    "emailVerificationCodeSent",
    "success",
    "message",
    "emailVerificationCodeVerified",
    "onLoginResponse",
    "onRegisterResponse",
    "onNetworkError",
    "resetLoginAttempts",
    "onUsernameAvailability",
    "available",
    "onEmailAvailability",
    "onEmailVerificationCodeSent",
    "onEmailVerificationCodeVerified",
    "login",
    "usernameOrEmail",
    "password",
    "captcha",
    "registerUser",
    "email",
    "verificationCode",
    "avatar",
    "logout",
    "refreshCaptcha",
    "connectToServer",
    "host",
    "port",
    "validateUsername",
    "validateEmail",
    "validatePassword",
    "checkUsernameAvailability",
    "checkEmailAvailability",
    "sendEmailVerificationCode",
    "verifyEmailCode",
    "code",
    "testMethod",
    "input",
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
    uint offsetsAndSizes[138];
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
    char stringdata11[9];
    char stringdata12[7];
    char stringdata13[15];
    char stringdata14[14];
    char stringdata15[25];
    char stringdata16[8];
    char stringdata17[22];
    char stringdata18[25];
    char stringdata19[27];
    char stringdata20[12];
    char stringdata21[24];
    char stringdata22[26];
    char stringdata23[8];
    char stringdata24[8];
    char stringdata25[30];
    char stringdata26[16];
    char stringdata27[19];
    char stringdata28[15];
    char stringdata29[19];
    char stringdata30[23];
    char stringdata31[10];
    char stringdata32[20];
    char stringdata33[28];
    char stringdata34[32];
    char stringdata35[6];
    char stringdata36[16];
    char stringdata37[9];
    char stringdata38[8];
    char stringdata39[13];
    char stringdata40[6];
    char stringdata41[17];
    char stringdata42[7];
    char stringdata43[7];
    char stringdata44[15];
    char stringdata45[16];
    char stringdata46[5];
    char stringdata47[5];
    char stringdata48[17];
    char stringdata49[14];
    char stringdata50[17];
    char stringdata51[26];
    char stringdata52[23];
    char stringdata53[26];
    char stringdata54[16];
    char stringdata55[5];
    char stringdata56[11];
    char stringdata57[6];
    char stringdata58[18];
    char stringdata59[19];
    char stringdata60[9];
    char stringdata61[13];
    char stringdata62[21];
    char stringdata63[9];
    char stringdata64[10];
    char stringdata65[13];
    char stringdata66[14];
    char stringdata67[12];
    char stringdata68[13];
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
        QT_MOC_LITERAL(160, 8),  // "username"
        QT_MOC_LITERAL(169, 6),  // "userId"
        QT_MOC_LITERAL(176, 14),  // "registerFailed"
        QT_MOC_LITERAL(191, 13),  // "logoutSuccess"
        QT_MOC_LITERAL(205, 24),  // "usernameValidationResult"
        QT_MOC_LITERAL(230, 7),  // "isValid"
        QT_MOC_LITERAL(238, 21),  // "emailValidationResult"
        QT_MOC_LITERAL(260, 24),  // "passwordValidationResult"
        QT_MOC_LITERAL(285, 26),  // "usernameAvailabilityResult"
        QT_MOC_LITERAL(312, 11),  // "isAvailable"
        QT_MOC_LITERAL(324, 23),  // "emailAvailabilityResult"
        QT_MOC_LITERAL(348, 25),  // "emailVerificationCodeSent"
        QT_MOC_LITERAL(374, 7),  // "success"
        QT_MOC_LITERAL(382, 7),  // "message"
        QT_MOC_LITERAL(390, 29),  // "emailVerificationCodeVerified"
        QT_MOC_LITERAL(420, 15),  // "onLoginResponse"
        QT_MOC_LITERAL(436, 18),  // "onRegisterResponse"
        QT_MOC_LITERAL(455, 14),  // "onNetworkError"
        QT_MOC_LITERAL(470, 18),  // "resetLoginAttempts"
        QT_MOC_LITERAL(489, 22),  // "onUsernameAvailability"
        QT_MOC_LITERAL(512, 9),  // "available"
        QT_MOC_LITERAL(522, 19),  // "onEmailAvailability"
        QT_MOC_LITERAL(542, 27),  // "onEmailVerificationCodeSent"
        QT_MOC_LITERAL(570, 31),  // "onEmailVerificationCodeVerified"
        QT_MOC_LITERAL(602, 5),  // "login"
        QT_MOC_LITERAL(608, 15),  // "usernameOrEmail"
        QT_MOC_LITERAL(624, 8),  // "password"
        QT_MOC_LITERAL(633, 7),  // "captcha"
        QT_MOC_LITERAL(641, 12),  // "registerUser"
        QT_MOC_LITERAL(654, 5),  // "email"
        QT_MOC_LITERAL(660, 16),  // "verificationCode"
        QT_MOC_LITERAL(677, 6),  // "avatar"
        QT_MOC_LITERAL(684, 6),  // "logout"
        QT_MOC_LITERAL(691, 14),  // "refreshCaptcha"
        QT_MOC_LITERAL(706, 15),  // "connectToServer"
        QT_MOC_LITERAL(722, 4),  // "host"
        QT_MOC_LITERAL(727, 4),  // "port"
        QT_MOC_LITERAL(732, 16),  // "validateUsername"
        QT_MOC_LITERAL(749, 13),  // "validateEmail"
        QT_MOC_LITERAL(763, 16),  // "validatePassword"
        QT_MOC_LITERAL(780, 25),  // "checkUsernameAvailability"
        QT_MOC_LITERAL(806, 22),  // "checkEmailAvailability"
        QT_MOC_LITERAL(829, 25),  // "sendEmailVerificationCode"
        QT_MOC_LITERAL(855, 15),  // "verifyEmailCode"
        QT_MOC_LITERAL(871, 4),  // "code"
        QT_MOC_LITERAL(876, 10),  // "testMethod"
        QT_MOC_LITERAL(887, 5),  // "input"
        QT_MOC_LITERAL(893, 17),  // "getDefaultAvatars"
        QT_MOC_LITERAL(911, 18),  // "uploadCustomAvatar"
        QT_MOC_LITERAL(930, 8),  // "filePath"
        QT_MOC_LITERAL(939, 12),  // "tryAutoLogin"
        QT_MOC_LITERAL(952, 20),  // "saveLoginCredentials"
        QT_MOC_LITERAL(973, 8),  // "remember"
        QT_MOC_LITERAL(982, 9),  // "isLoading"
        QT_MOC_LITERAL(992, 12),  // "errorMessage"
        QT_MOC_LITERAL(1005, 13),  // "loginAttempts"
        QT_MOC_LITERAL(1019, 11),  // "needCaptcha"
        QT_MOC_LITERAL(1031, 12)   // "captchaImage"
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
    "username",
    "userId",
    "registerFailed",
    "logoutSuccess",
    "usernameValidationResult",
    "isValid",
    "emailValidationResult",
    "passwordValidationResult",
    "usernameAvailabilityResult",
    "isAvailable",
    "emailAvailabilityResult",
    "emailVerificationCodeSent",
    "success",
    "message",
    "emailVerificationCodeVerified",
    "onLoginResponse",
    "onRegisterResponse",
    "onNetworkError",
    "resetLoginAttempts",
    "onUsernameAvailability",
    "available",
    "onEmailAvailability",
    "onEmailVerificationCodeSent",
    "onEmailVerificationCodeVerified",
    "login",
    "usernameOrEmail",
    "password",
    "captcha",
    "registerUser",
    "email",
    "verificationCode",
    "avatar",
    "logout",
    "refreshCaptcha",
    "connectToServer",
    "host",
    "port",
    "validateUsername",
    "validateEmail",
    "validatePassword",
    "checkUsernameAvailability",
    "checkEmailAvailability",
    "sendEmailVerificationCode",
    "verifyEmailCode",
    "code",
    "testMethod",
    "input",
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
      45,   14, // methods
       5,  435, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      17,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  284,    2, 0x06,    6 /* Public */,
       3,    0,  285,    2, 0x06,    7 /* Public */,
       4,    0,  286,    2, 0x06,    8 /* Public */,
       5,    0,  287,    2, 0x06,    9 /* Public */,
       6,    0,  288,    2, 0x06,   10 /* Public */,
       7,    0,  289,    2, 0x06,   11 /* Public */,
       8,    1,  290,    2, 0x06,   12 /* Public */,
      10,    2,  293,    2, 0x06,   14 /* Public */,
      13,    1,  298,    2, 0x06,   17 /* Public */,
      14,    0,  301,    2, 0x06,   19 /* Public */,
      15,    2,  302,    2, 0x06,   20 /* Public */,
      17,    2,  307,    2, 0x06,   23 /* Public */,
      18,    2,  312,    2, 0x06,   26 /* Public */,
      19,    1,  317,    2, 0x06,   29 /* Public */,
      21,    1,  320,    2, 0x06,   31 /* Public */,
      22,    2,  323,    2, 0x06,   33 /* Public */,
      25,    2,  328,    2, 0x06,   36 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      26,    2,  333,    2, 0x08,   39 /* Private */,
      27,    2,  338,    2, 0x08,   42 /* Private */,
      28,    1,  343,    2, 0x08,   45 /* Private */,
      29,    0,  346,    2, 0x08,   47 /* Private */,
      30,    1,  347,    2, 0x08,   48 /* Private */,
      32,    1,  350,    2, 0x08,   50 /* Private */,
      33,    2,  353,    2, 0x08,   52 /* Private */,
      34,    2,  358,    2, 0x08,   55 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      35,    3,  363,    2, 0x02,   58 /* Public */,
      35,    2,  370,    2, 0x22,   62 /* Public | MethodCloned */,
      39,    5,  375,    2, 0x02,   65 /* Public */,
      43,    0,  386,    2, 0x02,   71 /* Public */,
      44,    0,  387,    2, 0x02,   72 /* Public */,
      45,    2,  388,    2, 0x02,   73 /* Public */,
      45,    1,  393,    2, 0x22,   76 /* Public | MethodCloned */,
      45,    0,  396,    2, 0x22,   78 /* Public | MethodCloned */,
      48,    1,  397,    2, 0x02,   79 /* Public */,
      49,    1,  400,    2, 0x02,   81 /* Public */,
      50,    1,  403,    2, 0x02,   83 /* Public */,
      51,    1,  406,    2, 0x02,   85 /* Public */,
      52,    1,  409,    2, 0x02,   87 /* Public */,
      53,    1,  412,    2, 0x02,   89 /* Public */,
      54,    2,  415,    2, 0x02,   91 /* Public */,
      56,    1,  420,    2, 0x02,   94 /* Public */,
      58,    0,  423,    2, 0x102,   96 /* Public | MethodIsConst  */,
      59,    1,  424,    2, 0x02,   97 /* Public */,
      61,    0,  427,    2, 0x02,   99 /* Public */,
      62,    3,  428,    2, 0x02,  100 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString, QMetaType::LongLong,   11,   12,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   16,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   16,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   16,    9,
    QMetaType::Void, QMetaType::Bool,   20,
    QMetaType::Void, QMetaType::Bool,   20,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   23,   24,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   23,   24,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   23,   24,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   23,   24,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,   31,
    QMetaType::Void, QMetaType::Bool,   31,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   23,   24,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   23,   24,

 // methods: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,   36,   37,   38,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   36,   37,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QUrl,   11,   40,   41,   37,   42,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   46,   47,
    QMetaType::Void, QMetaType::QString,   46,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QString,   11,
    QMetaType::Bool, QMetaType::QString,   40,
    QMetaType::Bool, QMetaType::QString,   37,
    QMetaType::Bool, QMetaType::QString,   11,
    QMetaType::Bool, QMetaType::QString,   40,
    QMetaType::Void, QMetaType::QString,   40,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   40,   55,
    QMetaType::QString, QMetaType::QString,   57,
    QMetaType::QStringList,
    QMetaType::Bool, QMetaType::QUrl,   60,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::Bool,   11,   37,   63,

 // properties: name, type, flags
      64, QMetaType::Bool, 0x00015001, uint(0), 0,
      65, QMetaType::QString, 0x00015001, uint(1), 0,
      66, QMetaType::Int, 0x00015001, uint(2), 0,
      67, QMetaType::Bool, 0x00015001, uint(3), 0,
      68, QMetaType::QString, 0x00015001, uint(4), 0,

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
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
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
        // method 'emailVerificationCodeSent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'emailVerificationCodeVerified'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onLoginResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
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
        // method 'onUsernameAvailability'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onEmailAvailability'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onEmailVerificationCodeSent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onEmailVerificationCodeVerified'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
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
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QUrl &, std::false_type>,
        // method 'logout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'refreshCaptcha'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'connectToServer'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'connectToServer'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'connectToServer'
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
        // method 'sendEmailVerificationCode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'verifyEmailCode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'testMethod'
        QtPrivate::TypeAndForceComplete<QString, std::false_type>,
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
        case 7: _t->registerSuccess((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[2]))); break;
        case 8: _t->registerFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->logoutSuccess(); break;
        case 10: _t->usernameValidationResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 11: _t->emailValidationResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->passwordValidationResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 13: _t->usernameAvailabilityResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 14: _t->emailAvailabilityResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 15: _t->emailVerificationCodeSent((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 16: _t->emailVerificationCodeVerified((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 17: _t->onLoginResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 18: _t->onRegisterResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 19: _t->onNetworkError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 20: _t->resetLoginAttempts(); break;
        case 21: _t->onUsernameAvailability((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 22: _t->onEmailAvailability((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 23: _t->onEmailVerificationCodeSent((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 24: _t->onEmailVerificationCodeVerified((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 25: _t->login((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 26: _t->login((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 27: _t->registerUser((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[5]))); break;
        case 28: _t->logout(); break;
        case 29: _t->refreshCaptcha(); break;
        case 30: _t->connectToServer((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 31: _t->connectToServer((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 32: _t->connectToServer(); break;
        case 33: { bool _r = _t->validateUsername((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 34: { bool _r = _t->validateEmail((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 35: { bool _r = _t->validatePassword((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 36: { bool _r = _t->checkUsernameAvailability((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 37: { bool _r = _t->checkEmailAvailability((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 38: _t->sendEmailVerificationCode((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 39: _t->verifyEmailCode((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 40: { QString _r = _t->testMethod((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 41: { QStringList _r = _t->getDefaultAvatars();
            if (_a[0]) *reinterpret_cast< QStringList*>(_a[0]) = std::move(_r); }  break;
        case 42: { bool _r = _t->uploadCustomAvatar((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 43: _t->tryAutoLogin(); break;
        case 44: _t->saveLoginCredentials((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
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
            using _t = void (UserController::*)(const QString & , qint64 );
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
        {
            using _t = void (UserController::*)(bool , const QString & );
            if (_t _q_method = &UserController::emailVerificationCodeSent; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (UserController::*)(bool , const QString & );
            if (_t _q_method = &UserController::emailVerificationCodeVerified; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 16;
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
        if (_id < 45)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 45;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 45)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 45;
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
void UserController::registerSuccess(const QString & _t1, qint64 _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 7, _a);
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

// SIGNAL 15
void UserController::emailVerificationCodeSent(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 15, _a);
}

// SIGNAL 16
void UserController::emailVerificationCodeVerified(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}
QT_WARNING_POP
