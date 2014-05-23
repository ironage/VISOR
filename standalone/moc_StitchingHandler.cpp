/****************************************************************************
** Meta object code from reading C++ file 'StitchingHandler.h'
**
** Created: Fri May 2 22:54:04 2014
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "StitchingHandler.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'StitchingHandler.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_StitchingHandler[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      29,   18,   17,   17, 0x0a,
      75,   67,   17,   17, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_StitchingHandler[] = {
    "StitchingHandler\0\0updateData\0"
    "stitchingUpdate(StitchingUpdateData*)\0"
    "success\0stitchingFinished(bool)\0"
};

void StitchingHandler::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        StitchingHandler *_t = static_cast<StitchingHandler *>(_o);
        switch (_id) {
        case 0: _t->stitchingUpdate((*reinterpret_cast< StitchingUpdateData*(*)>(_a[1]))); break;
        case 1: _t->stitchingFinished((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData StitchingHandler::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject StitchingHandler::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_StitchingHandler,
      qt_meta_data_StitchingHandler, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &StitchingHandler::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *StitchingHandler::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *StitchingHandler::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_StitchingHandler))
        return static_cast<void*>(const_cast< StitchingHandler*>(this));
    return QObject::qt_metacast(_clname);
}

int StitchingHandler::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
