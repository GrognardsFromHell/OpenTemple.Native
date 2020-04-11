
#include <QmlNet/qml/NetQObject.h>
#include "qobject_interop.h"

void *wrap_qobject(QObject *qobject, bool ownsObject) {
    return new NetQObjectContainer{QSharedPointer<NetQObject>(new NetQObject(qobject, ownsObject))};
}
