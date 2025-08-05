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
    "emailVerificationSent",
    "success",
    "message",
    "onLoginResponse",
    "onRegisterResponse",
    "onNetworkError",
    "onEmailVerificationSent",
    "onEmailCodeVerificationResponse",
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
    "token",
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
    uint offsetsAndSizes[146];
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
    char stringdata29[22];
    char stringdata30[8];
    char stringdata31[8];
    char stringdata32[16];
    char stringdata33[19];
    char stringdata34[15];
    char stringdata35[24];
    char stringdata36[32];
    char stringdata37[19];
    char stringdata38[22];
    char stringdata39[29];
    char stringdata40[6];
    char stringdata41[16];
    char stringdata42[9];
    char stringdata43[8];
    char stringdata44[13];
    char stringdata45[7];
    char stringdata46[7];
    char stringdata47[15];
    char stringdata48[16];
    char stringdata49[5];
    char stringdata50[5];
    char stringdata51[17];
    char stringdata52[14];
    char stringdata53[17];
    char stringdata54[26];
    char stringdata55[23];
    char stringdata56[22];
    char stringdata57[24];
    char stringdata58[17];
    char stringdata59[6];
    char stringdata60[16];
    char stringdata61[5];
    char stringdata62[18];
    char stringdata63[19];
    char stringdata64[9];
    char stringdata65[13];
    char stringdata66[21];
    char stringdata67[9];
    char stringdata68[10];
    char stringdata69[13];
    char stringdata70[14];
    char stringdata71[12];
    char stringdata72[13];
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
        QT_MOC_LITERAL(492, 21),  // "emailVerificationSent"
        QT_MOC_LITERAL(514, 7),  // "success"
        QT_MOC_LITERAL(522, 7),  // "message"
        QT_MOC_LITERAL(530, 15),  // "onLoginResponse"
        QT_MOC_LITERAL(546, 18),  // "onRegisterResponse"
        QT_MOC_LITERAL(565, 14),  // "onNetworkError"
        QT_MOC_LITERAL(580, 23),  // "onEmailVerificationSent"
        QT_MOC_LITERAL(604, 31),  // "onEmailCodeVerificationResponse"
        QT_MOC_LITERAL(636, 18),  // "resetLoginAttempts"
        QT_MOC_LITERAL(655, 21),  // "onVerifyEmailResponse"
        QT_MOC_LITERAL(677, 28),  // "onResendVerificationResponse"
        QT_MOC_LITERAL(706, 5),  // "login"
        QT_MOC_LITERAL(712, 15),  // "usernameOrEmail"
        QT_MOC_LITERAL(728, 8),  // "password"
        QT_MOC_LITERAL(737, 7),  // "captcha"
        QT_MOC_LITERAL(745, 12),  // "registerUser"
        QT_MOC_LITERAL(758, 6),  // "avatar"
        QT_MOC_LITERAL(765, 6),  // "logout"
        QT_MOC_LITERAL(772, 14),  // "refreshCaptcha"
        QT_MOC_LITERAL(787, 15),  // "connectToServer"
        QT_MOC_LITERAL(803, 4),  // "host"
        QT_MOC_LITERAL(808, 4),  // "port"
        QT_MOC_LITERAL(813, 16),  // "validateUsername"
        QT_MOC_LITERAL(830, 13),  // "validateEmail"
        QT_MOC_LITERAL(844, 16),  // "validatePassword"
        QT_MOC_LITERAL(861, 25),  // "checkUsernameAvailability"
        QT_MOC_LITERAL(887, 22),  // "checkEmailAvailability"
        QT_MOC_LITERAL(910, 21),  // "sendEmailVerification"
        QT_MOC_LITERAL(932, 23),  // "resendEmailVerification"
        QT_MOC_LITERAL(956, 16),  // "verifyEmailToken"
        QT_MOC_LITERAL(973, 5),  // "token"
        QT_MOC_LITERAL(979, 15),  // "verifyEmailCode"
        QT_MOC_LITERAL(995, 4),  // "code"
        QT_MOC_LITERAL(1000, 17),  // "getDefaultAvatars"
        QT_MOC_LITERAL(1018, 18),  // "uploadCustomAvatar"
        QT_MOC_LITERAL(1037, 8),  // "filePath"
        QT_MOC_LITERAL(1046, 12),  // "tryAutoLogin"
        QT_MOC_LITERAL(1059, 20),  // "saveLoginCredentials"
        QT_MOC_LITERAL(1080, 8),  // "remember"
        QT_MOC_LITERAL(1089, 9),  // "isLoading"
        QT_MOC_LITERAL(1099, 12),  // "errorMessage"
        QT_MOC_LITERAL(1112, 13),  // "loginAttempts"
        QT_MOC_LITERAL(1126, 11),  // "needCaptcha"
        QT_MOC_LITERAL(1138, 12)   // "captchaImage"
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
    "emailVerificationSent",
    "success",
    "message",
    "onLoginResponse",
    "onRegisterResponse",
    "onNetworkError",
    "onEmailVerificationSent",
    "onEmailCodeVerificationResponse",
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
    "token",
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
      51,   14, // methods
       5,  485, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
      22,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,  320,    2, 0x06,    6 /* Public */,
       3,    0,  321,    2, 0x06,    7 /* Public */,
       4,    0,  322,    2, 0x06,    8 /* Public */,
       5,    0,  323,    2, 0x06,    9 /* Public */,
       6,    0,  324,    2, 0x06,   10 /* Public */,
       7,    0,  325,    2, 0x06,   11 /* Public */,
       8,    1,  326,    2, 0x06,   12 /* Public */,
      10,    3,  329,    2, 0x06,   14 /* Public */,
      14,    1,  336,    2, 0x06,   18 /* Public */,
      15,    0,  339,    2, 0x06,   20 /* Public */,
      16,    2,  340,    2, 0x06,   21 /* Public */,
      18,    2,  345,    2, 0x06,   24 /* Public */,
      19,    2,  350,    2, 0x06,   27 /* Public */,
      20,    1,  355,    2, 0x06,   30 /* Public */,
      22,    1,  358,    2, 0x06,   32 /* Public */,
      23,    0,  361,    2, 0x06,   34 /* Public */,
      24,    1,  362,    2, 0x06,   35 /* Public */,
      25,    0,  365,    2, 0x06,   37 /* Public */,
      26,    1,  366,    2, 0x06,   38 /* Public */,
      27,    0,  369,    2, 0x06,   40 /* Public */,
      28,    1,  370,    2, 0x06,   41 /* Public */,
      29,    2,  373,    2, 0x06,   43 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      32,    2,  378,    2, 0x08,   46 /* Private */,
      33,    2,  383,    2, 0x08,   49 /* Private */,
      34,    1,  388,    2, 0x08,   52 /* Private */,
      35,    2,  391,    2, 0x08,   54 /* Private */,
      36,    2,  396,    2, 0x08,   57 /* Private */,
      37,    0,  401,    2, 0x08,   60 /* Private */,
      38,    2,  402,    2, 0x08,   61 /* Private */,
      39,    2,  407,    2, 0x08,   64 /* Private */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
      40,    3,  412,    2, 0x02,   67 /* Public */,
      40,    2,  419,    2, 0x22,   71 /* Public | MethodCloned */,
      44,    4,  424,    2, 0x02,   74 /* Public */,
      46,    0,  433,    2, 0x02,   79 /* Public */,
      47,    0,  434,    2, 0x02,   80 /* Public */,
      48,    2,  435,    2, 0x02,   81 /* Public */,
      48,    1,  440,    2, 0x22,   84 /* Public | MethodCloned */,
      48,    0,  443,    2, 0x22,   86 /* Public | MethodCloned */,
      51,    1,  444,    2, 0x02,   87 /* Public */,
      52,    1,  447,    2, 0x02,   89 /* Public */,
      53,    1,  450,    2, 0x02,   91 /* Public */,
      54,    1,  453,    2, 0x02,   93 /* Public */,
      55,    1,  456,    2, 0x02,   95 /* Public */,
      56,    1,  459,    2, 0x02,   97 /* Public */,
      57,    1,  462,    2, 0x02,   99 /* Public */,
      58,    1,  465,    2, 0x02,  101 /* Public */,
      60,    2,  468,    2, 0x02,  103 /* Public */,
      62,    0,  473,    2, 0x102,  106 /* Public | MethodIsConst  */,
      63,    1,  474,    2, 0x02,  107 /* Public */,
      65,    0,  477,    2, 0x02,  109 /* Public */,
      66,    3,  478,    2, 0x02,  110 /* Public */,

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
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   30,   31,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   30,   31,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   30,   31,
    QMetaType::Void, QMetaType::QString,    9,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   30,   31,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   30,   31,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   30,   31,
    QMetaType::Void, QMetaType::Bool, QMetaType::QString,   30,   31,

 // methods: parameters
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString,   41,   42,   43,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   41,   42,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::QString, QMetaType::QUrl,   11,   12,   42,   45,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int,   49,   50,
    QMetaType::Void, QMetaType::QString,   49,
    QMetaType::Void,
    QMetaType::Bool, QMetaType::QString,   11,
    QMetaType::Bool, QMetaType::QString,   12,
    QMetaType::Bool, QMetaType::QString,   42,
    QMetaType::Bool, QMetaType::QString,   11,
    QMetaType::Bool, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,   12,
    QMetaType::Void, QMetaType::QString,   59,
    QMetaType::Void, QMetaType::QString, QMetaType::QString,   12,   61,
    QMetaType::QStringList,
    QMetaType::Bool, QMetaType::QUrl,   64,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::QString, QMetaType::Bool,   11,   42,   67,

 // properties: name, type, flags
      68, QMetaType::Bool, 0x00015001, uint(0), 0,
      69, QMetaType::QString, 0x00015001, uint(1), 0,
      70, QMetaType::Int, 0x00015001, uint(2), 0,
      71, QMetaType::Bool, 0x00015001, uint(3), 0,
      72, QMetaType::QString, 0x00015001, uint(4), 0,

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
        // method 'emailVerificationSent'
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
        // method 'onEmailVerificationSent'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onEmailCodeVerificationResponse'
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
        case 21: _t->emailVerificationSent((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 22: _t->onLoginResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 23: _t->onRegisterResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 24: _t->onNetworkError((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 25: _t->onEmailVerificationSent((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 26: _t->onEmailCodeVerificationResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 27: _t->resetLoginAttempts(); break;
        case 28: _t->onVerifyEmailResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 29: _t->onResendVerificationResponse((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 30: _t->login((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3]))); break;
        case 31: _t->login((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 32: _t->registerUser((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[4]))); break;
        case 33: _t->logout(); break;
        case 34: _t->refreshCaptcha(); break;
        case 35: _t->connectToServer((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 36: _t->connectToServer((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 37: _t->connectToServer(); break;
        case 38: { bool _r = _t->validateUsername((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 39: { bool _r = _t->validateEmail((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 40: { bool _r = _t->validatePassword((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 41: { bool _r = _t->checkUsernameAvailability((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 42: { bool _r = _t->checkEmailAvailability((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 43: _t->sendEmailVerification((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 44: _t->resendEmailVerification((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 45: _t->verifyEmailToken((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 46: _t->verifyEmailCode((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2]))); break;
        case 47: { QStringList _r = _t->getDefaultAvatars();
            if (_a[0]) *reinterpret_cast< QStringList*>(_a[0]) = std::move(_r); }  break;
        case 48: { bool _r = _t->uploadCustomAvatar((*reinterpret_cast< std::add_pointer_t<QUrl>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 49: _t->tryAutoLogin(); break;
        case 50: _t->saveLoginCredentials((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<bool>>(_a[3]))); break;
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
        {
            using _t = void (UserController::*)(bool , const QString & );
            if (_t _q_method = &UserController::emailVerificationSent; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 21;
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
        if (_id < 51)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 51;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 51)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 51;
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

// SIGNAL 21
void UserController::emailVerificationSent(bool _t1, const QString & _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 21, _a);
}
QT_WARNING_POP
