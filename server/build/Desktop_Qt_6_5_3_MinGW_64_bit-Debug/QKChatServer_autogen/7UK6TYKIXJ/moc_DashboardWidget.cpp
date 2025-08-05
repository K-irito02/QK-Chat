/****************************************************************************
** Meta object code from reading C++ file 'DashboardWidget.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.5.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/admin/DashboardWidget.h"
#include <QtCore/qmetatype.h>

#if __has_include(<QtCore/qtmochelpers.h>)
#include <QtCore/qtmochelpers.h>
#else
QT_BEGIN_MOC_NAMESPACE
#endif


#include <memory>

#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'DashboardWidget.h' doesn't include <QObject>."
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
struct qt_meta_stringdata_CLASSDashboardWidgetENDCLASS_t {};
static constexpr auto qt_meta_stringdata_CLASSDashboardWidgetENDCLASS = QtMocHelpers::stringData(
    "DashboardWidget",
    "updateStatistics",
    "",
    "startStatisticsUpdate",
    "updateUIElements",
    "performSafeStatisticsUpdate",
    "onSafetyTimeout",
    "updateUptime",
    "updateUserData",
    "initializeStaticData"
);
#else  // !QT_MOC_HAS_STRING_DATA
struct qt_meta_stringdata_CLASSDashboardWidgetENDCLASS_t {
    uint offsetsAndSizes[20];
    char stringdata0[16];
    char stringdata1[17];
    char stringdata2[1];
    char stringdata3[22];
    char stringdata4[17];
    char stringdata5[28];
    char stringdata6[16];
    char stringdata7[13];
    char stringdata8[15];
    char stringdata9[21];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_CLASSDashboardWidgetENDCLASS_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_CLASSDashboardWidgetENDCLASS_t qt_meta_stringdata_CLASSDashboardWidgetENDCLASS = {
    {
        QT_MOC_LITERAL(0, 15),  // "DashboardWidget"
        QT_MOC_LITERAL(16, 16),  // "updateStatistics"
        QT_MOC_LITERAL(33, 0),  // ""
        QT_MOC_LITERAL(34, 21),  // "startStatisticsUpdate"
        QT_MOC_LITERAL(56, 16),  // "updateUIElements"
        QT_MOC_LITERAL(73, 27),  // "performSafeStatisticsUpdate"
        QT_MOC_LITERAL(101, 15),  // "onSafetyTimeout"
        QT_MOC_LITERAL(117, 12),  // "updateUptime"
        QT_MOC_LITERAL(130, 14),  // "updateUserData"
        QT_MOC_LITERAL(145, 20)   // "initializeStaticData"
    },
    "DashboardWidget",
    "updateStatistics",
    "",
    "startStatisticsUpdate",
    "updateUIElements",
    "performSafeStatisticsUpdate",
    "onSafetyTimeout",
    "updateUptime",
    "updateUserData",
    "initializeStaticData"
};
#undef QT_MOC_LITERAL
#endif // !QT_MOC_HAS_STRING_DATA
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_CLASSDashboardWidgetENDCLASS[] = {

 // content:
      11,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x08,    1 /* Private */,
       3,    0,   63,    2, 0x08,    2 /* Private */,
       4,    0,   64,    2, 0x08,    3 /* Private */,
       5,    0,   65,    2, 0x08,    4 /* Private */,
       6,    0,   66,    2, 0x08,    5 /* Private */,
       7,    0,   67,    2, 0x08,    6 /* Private */,
       8,    0,   68,    2, 0x08,    7 /* Private */,
       9,    0,   69,    2, 0x08,    8 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

Q_CONSTINIT const QMetaObject DashboardWidget::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_meta_stringdata_CLASSDashboardWidgetENDCLASS.offsetsAndSizes,
    qt_meta_data_CLASSDashboardWidgetENDCLASS,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_CLASSDashboardWidgetENDCLASS_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<DashboardWidget, std::true_type>,
        // method 'updateStatistics'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'startStatisticsUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateUIElements'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'performSafeStatisticsUpdate'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSafetyTimeout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateUptime'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'updateUserData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'initializeStaticData'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
    >,
    nullptr
} };

void DashboardWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<DashboardWidget *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->updateStatistics(); break;
        case 1: _t->startStatisticsUpdate(); break;
        case 2: _t->updateUIElements(); break;
        case 3: _t->performSafeStatisticsUpdate(); break;
        case 4: _t->onSafetyTimeout(); break;
        case 5: _t->updateUptime(); break;
        case 6: _t->updateUserData(); break;
        case 7: _t->initializeStaticData(); break;
        default: ;
        }
    }
    (void)_a;
}

const QMetaObject *DashboardWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DashboardWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_CLASSDashboardWidgetENDCLASS.stringdata0))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int DashboardWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
    }
    return _id;
}
QT_WARNING_POP
