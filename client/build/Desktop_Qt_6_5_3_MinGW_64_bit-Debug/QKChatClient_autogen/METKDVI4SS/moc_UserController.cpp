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
    "email",
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
    "emailVerified",
    "emailVerificationFailed",
    "emailVerificationResent",
    "emailVerificationResendFailed",
    "emailCodeVerified",
    "emailCodeVerificationFailed",
    "onLoginResponse",
    "success",
    "message",
    "token",
    "onRegisterResponse",
    "onNetworkError",
    "onEmailVerificationSent",
    "resetLoginAttempts",
    "onVerifyEmailResponse",
    "onResendVerificationResponse",
    "login",
    "usernameOrEmail",
    "password",
    "captcha",
    "registerUser",
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
    "sendEmailVerification",
    "resendEmailVerification",
    "verifyEmailToken",
    "verifyEmailCode",
    "code",
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
    uint offsetsAndSizes[142];
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
    char stringdata12[6];
    char stringdata13[7];
    char stringdata14[15];
    char stringdata15[14];
    char stringdata16[25];
    char stringdata17[8];
    char stringdata18[22];
    char stringdata19[25];
    char stringdata20[27];
    char stringdata21[12];
    char stringdata22[24];
    char stringdata23[14];
    char stringdata24[24];
    char stringdata25[24];
    char stringdata26[30];
    char stringdata27[18];
    char stringdata28[28];
    char stringdata29[16];
    char stringdata30[8];
    char stringdata31[8];
    char stringdata32[6];
    char stringdata33[19];
    char stringdata34[15];
    char stringdata35[24];
    char stringdata36[19];
    char stringdata37[22];
    char stringdata38[29];
    char stringdata39[6];
    char stringdata40[16];
    char stringdata41[9];
    char stringdata42[8];
    char stringdata43[13];
    char stringdata44[7];
    char stringdata45[7];
    char stringdata46[15];
    char stringdata47[16];
    char stringdata48[5];
    char stringdata49[5];
    char stringdata50[17];
    char stringdata51[14];
    char stringdata52[17];
    char stringdata53[26];
    char stringdata54[23];
    char stringdata55[22];
    char stringdata56[24];
    char stringdata57[17];
    char stringdata58[16];
    char stringdata59[5];
    char stringdata60[18];
    char stringdata61[19];
    char stringdata62[9];
    char stringdata63[13];
    char stringdata64[21];
    char stringdata65[9];
    char stringdata66[10];
    char stringdata67[13];
    char stringdata68[14];
    char stringdata69[12];
    char stringdata70[13];
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
        QT_MOC_LITERAL(169, 5),  // "email"
        QT_MOC_LITERAL(175, 6),  // "userId"
        QT_MOC_LITERAL(182, 14),  // "registerFailed"
        QT_MOC_LITERAL(197, 13),  // "logoutSuccess"
        QT_MOC_LITERAL(211, 24),  // "usernameValidationResult"
        QT_MOC_LITERAL(236, 7),  // "isValid"
        QT_MOC_LITERAL(244, 21),  // "emailValidationResult"
        QT_MOC_LITERAL(266, 24),  // "passwordValidationResult"
        QT_MOC_LITERAL(291, 26),  // "usernameAvailabilityResult"
        QT_MOC_LITERAL(318, 11),  // "isAvailable"
        QT_MOC_LITERAL(330, 23),  // "emailAvailabilityResult"
        QT_MOC_LITERAL(354, 13),  // "emailVerified"
        QT_MOC_LITERAL(368, 23),  // "emailVerificationFailed"
        QT_MOC_LITERAL(392, 23),  // "emailVerificationResent"
        QT_MOC_LITERAL(416, 29),  // "emailVerificationResendFailed"
        QT_MOC_LITERAL(446, 17),  // "emailCodeVerified"
        QT_MOC_LITERAL(464, 27),  // "emailCodeVerificationFailed"
        QT_MOC_LITERAL(492, 15),  // "onLoginResponse"
        QT_MOC_LITERAL(508, 7),  // "success"
        QT_MOC_LITERAL(516, 7),  // "message"
        QT_MOC_LITERAL(524, 5),  // "token"
        QT_MOC_LITERAL(530, 18),  // "onRegisterResponse"
        QT_MOC_LITERAL(549, 14),  // "onNetworkError"
        QT_MOC_LITERAL(564, 23),  // "onEmailVerificationSent"
        QT_MOC_LITERAL(588, 18),  // "resetLoginAttempts"
        QT_MOC_LITERAL(607, 21),  // "onVerifyEmailResponse"
        QT_MOC_LITERAL(629, 28),  // "onResendVerificationResponse"
        QT_MOC_LITERAL(658, 5),  // "login"
        QT_MOC_LITERAL(664, 15),  // "usernameOrEmail"
        QT_MOC_LITERAL(680, 8),  // "password"
        QT_MOC_LITERAL(689, 7),  // "captcha"
        QT_MOC_LITERAL(697, 12),  // "registerUser"
        QT_MOC_LITERAL(710, 6),  // "avatar"
        QT_MOC_LITERAL(717, 6),  // "logout"
        QT_MOC_LITERAL(724, 14),  // "refreshCaptcha"
        QT_MOC_LITERAL(739, 15),  // "connectToServer"
        QT_MOC_LITERAL(755, 4),  // "host"
        QT_MOC_LITERAL(760, 4),  // "port"
        QT_MOC_LITERAL(765, 16),  // "validateUsername"
        QT_MOC_LITERAL(782, 13),  // "validateEmail"
        QT_MOC_LITERAL(796, 16),  // "validatePassword"
        QT_MOC_LITERAL(813, 25),  // "checkUsernameAvailability"
        QT_MOC_LITERAL(839, 22),  // "checkEmailAvailability"
        QT_MOC_LITERAL(862, 21),  // "sendEmailVerification"
        QT_MOC_LITERAL(884, 23),  // "resendEmailVerification"
        QT_MOC_LITERAL(908, 16),  // "verifyEmailToken"
        QT_MOC_LITERAL(925, 15),  // "verifyEmailCode"
        QT_MOC_LITERAL(941, 4),  // "code"
        QT_MOC_LITERAL(946, 17),  // "getDefaultAvatars"
        QT_MOC_LITERAL(964, 18),  // "uploadCustomAvatar"
        QT_MOC_LITERAL(983, 8),  // "filePath"
        QT_MOC_LITERAL(992, 12),  // "tryAutoLogin"
        QT_MOC_LITERAL(1005, 20),  // "saveLoginCredentials"
        QT_MOC_LITERAL(1026, 8),  // "remember"
        QT_MOC_LITERAL(1035, 9),  // "isLoading"
        QT_MOC_LITERAL(1045, 12),  // "errorMessage"
        QT_MOC_LITERAL(1058, 13),  // "loginAttempts"
        QT_MOC_LITERAL(1072, 11),  // "needCaptcha"
        QT_MOC_LITERAL(1084, 12)   // "captchaImage"
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
    "email",
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
    "emailVerified",
    "emailVerificationFailed",
    "emailVerificationResent",
    "emailVerificationResendFailed",
    "emailCodeVerified",
    "emailCodeVerificationFailed",
    "onLoginResponse",
    "success",
    "message",
    "token",
    "onRegisterResponse",
    "onNetworkError",
    "onEmailVerificationSent",
    "resetLoginAttempts",
    "onVerifyEmailResponse",
    "onResendVerificationResponse",
    "login",
    "usernameOrEmail",
    "password",
    "captcha",
    "registerUser",
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
    "sendEmailVerification",
    "resendEmailVerification",
    "verifyEmailToken",
    "verifyEmailCode",
    "code",
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
      49,   14, // methods
       5,  471, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      21,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  308,    2, 0x06,    6 /* Public */,
       3,    0,  309,    2, 0x06,    7 /* Public */,
       4,    0,  310,    2, 0x06,    8 /* Public */,
       5,    0,  311,    2, 0x06,    9 /* Public */,
       6,    0,  312,    2, 0x06,   10 /* Public */,
       7,    0,  313,    2, 0x06,   11 /* Public */,
       8,    1,  314,    2, 0x06,   12 /* Public */,
      10,    3,  317,    2, 0x06,   14 /* Public */,
      14,    1,  324,    2, 0x06,   18 /* Public */,
      15,    0,  327,    2, 0x06,   20 /* Public */,
      16,    2,  328,    2, 0x06,   21 /* Public */,
      18,    2,  333,    2, 0x06,   24 /* Public */,
      19,    2,  338,    2, 0x06,   27 /* Public */,
      20,    1,  343,    2, 0x06,   30 /* Public */,
      22,    1,  346,    2, 0x06,   32 /* Public */,
      23,    0,  349,    2, 0x06,   34 /* Public */,
      24,    1,  350,    2, 0x06,   35 /* Public */,
      25,    0,  353,    2, 0x06,   37 /* Public */,
      26,    1,  354,    2, 0x06,   38 /* Public */,
      27,    0,  357,    2, 0x06,   40 /* Public */,
      28,    1,  358,    2, 0x06,   41 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      29,    3,  361,    2, 0x08,   43 /* Private */,
      33,    5,  368,    2, 0x08,   47 /* Private */,
      34,    1,  379,    2, 0x08,   53 /* Private */,
      35,    2,  382,    2, 0x08,   55 /* Private */,
      36,    0,  387,    2, 0x08,   58 /* Private */,
      37,    2,  388,    2, 0x08,   59 /* Private */,
      38,    2,  393,    2, 0x08,   62 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      39,    3,  398,    2, 0x02,   65 /* Public */,
      39,    2,  405,    2, 0x22,   69 /* Public | MethodCloned */,
      43,    4,  410,    2, 0x02,   72 /* Public */,
      45,    0,  419,    2, 0x02,   77 /* Public */,
      46,    0,  420,    2, 0x02,   78 /* Public */,
      47,    2,  421,    2, 0x02,   79 /* Public */,
      47,    1,  426,    2, 0x22,   82 /* Public | MethodCloned */,
      47,    0,  429,    2, 0x22,   84 /* Public | MethodCloned */,
      50,    1,  430,    2, 0x02,   85 /* Public */,
      51,    1,  433,    2, 0x02,   87 /* Public */,
      52,    1,  436,    2, 0x02,   89 /* Public */,
      53,    1,  439,    2, 0x02,   91 /* Public */,
      54,    1,  442,    2, 0x02,   93 /* Public */,
      55,    1,  445,    2, 0x02,   95 /* Public */,
      56,    1,  448,    2, 0x02,   97 /* Public */,
      57,    1,  451,    2, 0x02,   99 /* Public */,
      58,    2,  454,    2, 0x02,  101 /* Public */,
      60,    0,  459,    2, 0x102,  104 /* Public | MethodIsConst  */,
      61,    1,  460,    2, 0x02,  105 /* Public */,
      63,    0,  463,    2, 0x02,  107 /* Public */,
      64,    3,  464,    2, 0x02,  108 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::LongLong,   11,   12,   13,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   17,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   17,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   17,    9,
    QMetaType::Void, QMetaType::Bool,   21,
    QMetaType::Void, QMetaType::Bool,   21,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString,    9,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool, QMetaType::QString, QMetaType::QString,   30,   31,   32,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::LongLong,   30,   31,   11,   12,   13,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   30,   31,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   30,   31,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   30,   31,

 // methods: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,   40,   41,   42,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   40,   41,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QUrl,   11,   12,   41,   44,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   48,   49,
    QMetaType::Void, QMetaType::QString,   48,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QString,   11,
    QMetaType::Bool, QMetaType::QString,   12,
    QMetaType::Bool, QMetaType::QString,   41,
    QMetaType::Bool, QMetaType::QString,   11,
    QMetaType::Bool, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,   32,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   12,   59,
    QMetaType::QStringList,
    QMetaType::Bool, QMetaType::QUrl,   62,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::Bool,   11,   41,   65,

 // properties: name, type, flags
      66, QMetaType::Bool, 0x00015001, uint(0), 0,
      67, QMetaType::QString, 0x00015001, uint(1), 0,
      68, QMetaType::Int, 0x00015001, uint(2), 0,
      69, QMetaType::Bool, 0x00015001, uint(3), 0,
      70, QMetaType::QString, 0x00015001, uint(4), 0,

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
        // method 'emailVerified'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'emailVerificationFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'emailVerificationResent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'emailVerificationResendFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'emailCodeVerified'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'emailCodeVerificationFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onLoginResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onRegisterResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        QtPrivate::TypeAndForceComplete<qint64, std::false_type>,
        // method 'onNetworkError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onEmailVerificationSent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resetLoginAttempts'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onVerifyEmailResponse'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onResendVerificationResponse'
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
        // method 'sendEmailVerification'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'resendEmailVerification'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'verifyEmailToken'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'verifyEmailCode'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
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
        case 7: _t->registerSuccess((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[3]))); break;
        case 8: _t->registerFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->logoutSuccess(); break;
        case 10: _t->usernameValidationResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 11: _t->emailValidationResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 12: _t->passwordValidationResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 13: _t->usernameAvailabilityResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 14: _t->emailAvailabilityResult((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 15: _t->emailVerified(); break;
        case 16: _t->emailVerificationFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 17: _t->emailVerificationResent(); break;
        case 18: _t->emailVerificationResendFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 19: _t->emailCodeVerified(); break;
        case 20: _t->emailCodeVerificationFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 21: _t->onLoginResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 22: _t->onRegisterResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<qint64>>(_a[5]))); break;
        case 23: _t->onNetworkError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 24: _t->onEmailVerificationSent((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 25: _t->resetLoginAttempts(); break;
        case 26: _t->onVerifyEmailResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 27: _t->onResendVerificationResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 28: _t->login((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 29: _t->login((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 30: _t->registerUser((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[4]))); break;
        case 31: _t->logout(); break;
        case 32: _t->refreshCaptcha(); break;
        case 33: _t->connectToServer((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 34: _t->connectToServer((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 35: _t->connectToServer(); break;
        case 36: { bool _r = _t->validateUsername((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 37: { bool _r = _t->validateEmail((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 38: { bool _r = _t->validatePassword((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 39: { bool _r = _t->checkUsernameAvailability((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 40: { bool _r = _t->checkEmailAvailability((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 41: _t->sendEmailVerification((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 42: _t->resendEmailVerification((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 43: _t->verifyEmailToken((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 44: _t->verifyEmailCode((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 45: { QStringList _r = _t->getDefaultAvatars();
            if (_a[0]) *reinterpret_cast< QStringList*>(_a[0]) = std::move(_r); }  break;
        case 46: { bool _r = _t->uploadCustomAvatar((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 47: _t->tryAutoLogin(); break;
        case 48: _t->saveLoginCredentials((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
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
            using _t = void (UserController::*)(const QString & , const QString & , qint64 );
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
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::emailVerified; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 15;
                return;
            }
        }
        {
            using _t = void (UserController::*)(const QString & );
            if (_t _q_method = &UserController::emailVerificationFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 16;
                return;
            }
        }
        {
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::emailVerificationResent; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 17;
                return;
            }
        }
        {
            using _t = void (UserController::*)(const QString & );
            if (_t _q_method = &UserController::emailVerificationResendFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 18;
                return;
            }
        }
        {
            using _t = void (UserController::*)();
            if (_t _q_method = &UserController::emailCodeVerified; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 19;
                return;
            }
        }
        {
            using _t = void (UserController::*)(const QString & );
            if (_t _q_method = &UserController::emailCodeVerificationFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 20;
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
        if (_id < 49)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 49;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 49)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 49;
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
void UserController::registerSuccess(const QString & _t1, const QString & _t2, qint64 _t3)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t3))) };
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
void UserController::emailVerified()
{
    QMetaObject::activate(this, &staticMetaObject, 15, nullptr);
}

// SIGNAL 16
void UserController::emailVerificationFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 16, _a);
}

// SIGNAL 17
void UserController::emailVerificationResent()
{
    QMetaObject::activate(this, &staticMetaObject, 17, nullptr);
}

// SIGNAL 18
void UserController::emailVerificationResendFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 18, _a);
}

// SIGNAL 19
void UserController::emailCodeVerified()
{
    QMetaObject::activate(this, &staticMetaObject, 19, nullptr);
}

// SIGNAL 20
void UserController::emailCodeVerificationFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 20, _a);
}
QT_WARNING_POP
