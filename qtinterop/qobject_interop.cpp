
#include <QObject>
#include <QQuickItem>
#include <QUrl>

#include <private/qmetaobject_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmltypedata_p.h>
#include <QtGui/QColor>

#include "../game/utils.h"
#include "string_interop.h"

NATIVE_API bool QMetaType_resolveCppType(const char *name, int *type,
                                         const QMetaObject **metaObject) {
  *type = QMetaType::type(name);
  if (*type != 0) {
    *metaObject = QMetaType::metaObjectForType(*type);
    return metaObject != nullptr;
  } else {
    *metaObject = nullptr;
    return false;
  }
}

NATIVE_API bool QMetaType_resolveQmlCppType(const char *metaClassName, const char16_t *typeName,
                                            const char16_t *moduleName, int majorVersion,
                                            int minorVersion, int *type,
                                            const QMetaObject **metaObject) {
  QString typeNameStr = QString::fromUtf16(typeName);
  QString moduleNameStr = QString::fromUtf16(moduleName);
  if (!QQmlMetaType::typeModule(moduleNameStr, majorVersion)) {
    // This is slow and not great, but it'll load the module and keep at least the static types
    // around, even after the engine is destroyed
    QQmlEngine e;
    QQmlComponent c(&e);
    auto componentCode =
        QString("import %1 %2\nItem{}").arg(moduleNameStr).arg(majorVersion).toUtf8();
    c.setData(componentCode, QUrl());
  }

  *type = QMetaType::type(QByteArray(metaClassName) + "*");
  if (*type) {
    *metaObject = QMetaType::metaObjectForType(*type);
    if (*metaObject) {
      return true;
    }
  }

  auto qmlType = QQmlMetaType::qmlType(typeNameStr, moduleNameStr, majorVersion, minorVersion);
  if (!qmlType.isValid()) {
    return false;
  }

  *type = qmlType.typeId();
  *metaObject = qmlType.metaObject();
  return true;
}

NATIVE_API bool QMetaType_resolveQmlType(QObject *exampleObject, const char16_t *urlString,
                                         const char16_t *inlineComponentNameRaw, int *type,
                                         const QMetaObject **metaObject) {
  auto engine = qmlEngine(exampleObject);
  if (!engine) {
    return false;
  }

  // This is a "lightweight" replica of what QQmlComponent is doing
  QUrl url(QString::fromUtf16(urlString));
  if (url.isRelative()) {
    url = engine->baseUrl().resolved(url);
  }

  auto data = QQmlEnginePrivate::get(engine)->typeLoader.getType(url, QQmlTypeLoader::Synchronous);

  if (!data->isComplete()) {
    qWarning() << "Failed to load component " << url;
    return false;
  }

  if (data->count() <= 1) {
    qWarning() << "We're the first to load" << url << "meaning it'll go"
               << "out of scope and likely cause a crash.";
  }

  QV4::ExecutableCompilationUnit *compilationUnit = data->compilationUnit();
  Q_ASSERT(compilationUnit->isRegisteredWithEngine);

  if (inlineComponentNameRaw) {
    QString inlineComponentName =
        QString::fromRawData(reinterpret_cast<const QChar *>(inlineComponentNameRaw),
                             std::char_traits<char16_t>::length(inlineComponentNameRaw));

    for (auto &ic : compilationUnit->inlineComponentData) {
      QString icName = compilationUnit->stringAt(ic.nameIndex);
      if (icName == inlineComponentName) {
        *type = ic.typeIds.id;
        if (!*type) {
          qDebug() << "Was able to load" << url << " and find inline component "
                   << inlineComponentName << " but it has no type id.";
          return false;
        }
        *metaObject = compilationUnit->propertyCaches.at(ic.objectIndex)->createMetaObject();
        if (!*metaObject) {
          qDebug() << "Was able to load" << url << " and find inline component "
                   << inlineComponentName << " but couldn't create a meta object for it.";
          return false;
        }

        return true;
      }
    }

    qDebug() << "Was able to load" << url << " but couldn't find inline component "
             << inlineComponentName;
    return false;
  } else {
    // The root object was requested

    *type = compilationUnit->metaTypeId;
    if (!*type) {
      qDebug() << "Was able to load" << url << "but it has no type id";
      return false;
    }
    *metaObject = compilationUnit->rootPropertyCache()->createMetaObject();
  }

  return *metaObject != nullptr;
}

NATIVE_API int QMetaObject_indexOfProperty(const QMetaObject *mo, const char *name) {
  return mo->indexOfProperty(name);
}

NATIVE_API void QObject_setObjectOwnership(QObject *target, bool jsOwned) {
  QQmlEngine::setObjectOwnership(
      target, jsOwned ? QQmlEngine::JavaScriptOwnership : QQmlEngine::CppOwnership);
}

NATIVE_API bool QObject_getObjectOwnership(QObject *target) {
  return QQmlEngine::objectOwnership(target) == QQmlEngine::JavaScriptOwnership;
}

NATIVE_API bool QObject_setPropertyQString(QObject *target, int idx, const char16_t *value,
                                           int length) {
  QString string(reinterpret_cast<const QChar *>(value), length);
  void *stringPtr = &string;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &stringPtr) < 0;
}

NATIVE_API bool QObject_getPropertyQString(QObject *target, int idx, const char16_t **valueOut,
                                           int *lengthOut) {
  QString string;
  void *stringPtr = &string;
  if (QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &stringPtr) >= 0) {
    return false;
  }

  // The intent here is that QString itself is temporary, but the
  // storage that backs it is not.
  *valueOut = reinterpret_cast<const char16_t *>(string.utf16());
  *lengthOut = string.length();
  return true;
}

NATIVE_API bool QObject_setProperty(QObject *target, int idx, void *value) {
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_getProperty(QObject *target, int idx, void *value) {
  return QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_setPropertyQSize(QObject *target, int idx, int width, int height) {
  QSize size(width, height);
  void *value = &size;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_getPropertyQSize(QObject *target, int idx, int *width, int *height) {
  QSize size;
  void *value = &size;
  auto result = QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &value) < 0;
  *width = size.width();
  *height = size.height();
  return result;
}

NATIVE_API bool QObject_setPropertyQSizeF(QObject *target, int idx, double width, double height) {
  QSizeF size(width, height);
  void *value = &size;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_getPropertyQSizeF(QObject *target, int idx, double *width, double *height) {
  QSizeF size;
  void *value = &size;
  auto result = QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &value) < 0;
  *width = size.width();
  *height = size.height();
  return result;
}

NATIVE_API bool QObject_setPropertyQRect(QObject *target, int idx, int x, int y, int width,
                                         int height) {
  QRect rect(x, y, width, height);
  void *value = &rect;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_getPropertyQRect(QObject *target, int idx, int *x, int *y, int *width,
                                         int *height) {
  QRect rect;
  void *value = &rect;
  auto result = QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &value) < 0;
  *x = rect.x();
  *y = rect.y();
  *width = rect.width();
  *height = rect.height();
  return result;
}

NATIVE_API bool QObject_setPropertyQRectF(QObject *target, int idx, double x, double y,
                                          double width, double height) {
  QRectF rect(x, y, width, height);
  void *value = &rect;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_getPropertyQRectF(QObject *target, int idx, double *x, double *y,
                                          double *width, double *height) {
  QRectF rect;
  void *value = &rect;
  auto result = QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &value) < 0;
  *x = rect.x();
  *y = rect.y();
  *width = rect.width();
  *height = rect.height();
  return result;
}

NATIVE_API bool QObject_setPropertyQPoint(QObject *target, int idx, int x, int y) {
  QPoint point(x, y);
  void *value = &point;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_getPropertyQPoint(QObject *target, int idx, int *x, int *y) {
  QPoint point;
  void *value = &point;
  auto result = QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &value) < 0;
  *x = point.x();
  *y = point.y();
  return result;
}

NATIVE_API bool QObject_setPropertyQPointF(QObject *target, int idx, double x, double y) {
  QPointF point(x, y);
  void *value = &point;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_getPropertyQPointF(QObject *target, int idx, double *x, double *y) {
  QPointF point;
  void *value = &point;
  auto result = QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &value) < 0;
  *x = point.x();
  *y = point.y();
  return result;
}

NATIVE_API bool QObject_setPropertyQUrl(QObject *target, int idx, const char16_t *urlString,
                                        int urlStringLength) {
  QUrl url(QString::fromRawData(reinterpret_cast<const QChar *>(urlString), urlStringLength));
  void *value = &url;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_getPropertyQUrl(QObject *target, int idx, char16_t **urlStringOut) {
  QUrl url;
  void *value = &url;
  auto result = QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &value) < 0;
  if (result) {
    *urlStringOut = copyString(url.toString());
  }
  return result;
}

NATIVE_API bool QObject_setPropertyQColor(QObject *target, int idx, uint32_t argb) {
  QColor color = QColor::fromRgba(argb);  // It says RGBA, but really it's ARGB
  void *value = &color;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_getPropertyQColor(QObject *target, int idx, uint32_t *argb) {
  QColor color;
  void *value = &color;
  auto result = QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &value) < 0;
  *argb = color.rgba();
  return result;
}

NATIVE_API bool QObject_setPropertyQDateTime(QObject *target, int idx, int64_t msecsSinceEpoch,
                                             bool localTime) {
  QDateTime dateTime =
      QDateTime::fromMSecsSinceEpoch(msecsSinceEpoch, localTime ? Qt::LocalTime : Qt::UTC);
  void *value = &dateTime;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &value) < 0;
}

NATIVE_API bool QObject_getPropertyQDateTime(QObject *target, int idx, int64_t *msecsSinceEpoch,
                                             bool *localTime) {
  QDateTime dateTime;
  void *value = &dateTime;
  auto result = QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &value) < 0;
  if (dateTime.timeSpec() != Qt::LocalTime && dateTime.timeSpec() != Qt::UTC) {
    dateTime = dateTime.toUTC();
  }
  *msecsSinceEpoch = dateTime.toMSecsSinceEpoch();
  *localTime = dateTime.timeSpec() == Qt::LocalTime;
  return result;
}

NATIVE_API void QDateTime_write(QDateTime *dateTime, int64_t msecsSinceEpoch, bool localTime) {
  *dateTime = QDateTime::fromMSecsSinceEpoch(msecsSinceEpoch, localTime ? Qt::LocalTime : Qt::UTC);
}

NATIVE_API void QDateTime_read(QDateTime *dateTime, int64_t *msecsSinceEpoch, bool *localTime) {
  *msecsSinceEpoch = dateTime->toMSecsSinceEpoch();
  *localTime = dateTime->timeSpec() == Qt::LocalTime;
}

NATIVE_API bool QObject_callMethod(QObject *target, int idx, void **args) {
  return QMetaObject::metacall(target, QMetaObject::InvokeMetaMethod, idx, args) < 0;
}

NATIVE_API QObject *QObject_create(const QMetaObject *metaObject) {
  auto qmlType = QQmlMetaType::qmlType(metaObject);
  if (qmlType.isValid()) {
    return qmlType.create();
  }
  return metaObject->newInstance();
}

NATIVE_API int QMetaObject_indexOfMethod(const QMetaObject *mo, const char *name) {
  return mo->indexOfMethod(name);
}

NATIVE_API const QMetaObject *QObject_metaObject(QObject *obj) { return obj->metaObject(); }

static QString getRelativeComponentUrl(QQmlEngine *engine, const QString &url) {
  // Try to resolve the URL relative to the import URLs and base path
  if (!engine->baseUrl().isEmpty()) {
    auto engineBaseUrl = engine->baseUrl().toString();
    if (url.startsWith(engineBaseUrl)) {
      return url.mid(engineBaseUrl.size());
    }
  }

  // TODO: It'd make sense to actually check each import URL as well

  return url;
}

NATIVE_API bool QObject_objectRuntimeType(QObject *target, bool *isQmlFile, char **metaClassName,
                                          char16_t **qmlSourceUrl,
                                          char16_t **qmlInlineComponentName) {
  auto qmlData = QQmlData::get(target);
  if (qmlData && qmlData->context && qmlData->context->engine && qmlData->compilationUnit) {
    auto engine = qmlData->context->engine;
    auto compilationUnit = qmlData->compilationUnit;

    *isQmlFile = true;
    *metaClassName = nullptr;
    // Try to resolve the URL relative to the import URLs and base path
    QString url = getRelativeComponentUrl(engine, compilationUnit->finalUrlString());
    *qmlSourceUrl = copyString(url);
    *qmlInlineComponentName = nullptr;

    auto propertyCache = qmlData->ensurePropertyCache(engine, target);

    // Check if the object is of the QML file's primary type
    if (compilationUnit->rootPropertyCache().data() == propertyCache) {
      return true;
    }
    // Otherwise search through the inline components
    for (auto &&data : compilationUnit->inlineComponentData) {
      if (compilationUnit->propertyCaches.at(data.objectIndex) == propertyCache) {
        *qmlInlineComponentName = copyString(compilationUnit->stringAt(data.nameIndex));
        return true;
      }
    }

    qDebug() << "Failed to figure out which inline component of "
             << compilationUnit->finalUrlString() << " maps to type "
             << target->metaObject()->className();
    return false;
  }

  const QMetaObject *metaObject = target->metaObject();
  int typeId;
  while (metaObject) {
    QByteArray ptrType = metaObject->className();
    ptrType.push_back('*');
    typeId = QMetaType::type(ptrType);
    if (typeId == QMetaType::UnknownType) {
      metaObject = metaObject->superClass();
    } else {
      break;
    }
  }

  if (!metaObject) {
    *isQmlFile = false;
    *metaClassName = nullptr;
    *qmlSourceUrl = nullptr;
    *qmlInlineComponentName = nullptr;
    return false;
  }

  *isQmlFile = false;
  *metaClassName = copyString(metaObject->className());
  *qmlSourceUrl = nullptr;
  *qmlInlineComponentName = nullptr;
  return true;
}

using DelegateSlotDispatcher = void(void *delegateHandle, void **args);
using DelegateSlotReleaseDelegate = void(void *delegateHandle);
static DelegateSlotReleaseDelegate *DelegateSlotObject_releaseDelegate = nullptr;

NATIVE_API void DelegateSlotObject_setCallbacks(DelegateSlotReleaseDelegate *release) {
  DelegateSlotObject_releaseDelegate = release;
}

// See QV4::QObjectSlotDispatcher for inspiration
class DelegateSlotObject : public QtPrivate::QSlotObjectBase {
  void *const delegateHandle;
  DelegateSlotDispatcher *const dispatcher;

  static void impl(int which, QSlotObjectBase *this_, QObject *r, void **a, bool *ret) {
    auto self = static_cast<DelegateSlotObject *>(this_);

    switch (which) {
      case Destroy:
        DelegateSlotObject_releaseDelegate(self->delegateHandle);
        delete self;
        break;
      case Call:
        self->dispatcher(self->delegateHandle, a);
        break;
      case Compare:
        *ret = *a == self->delegateHandle;
        break;
      case NumOperations:;
    }
  }

 public:
  DelegateSlotObject(void *delegateHandle, DelegateSlotDispatcher *dispatcher)
      : QSlotObjectBase(&impl), delegateHandle(delegateHandle), dispatcher(dispatcher) {}
};

NATIVE_API bool QObject_connect(const QObject *obj, int signalIndex, void *gcHandle,
                                DelegateSlotDispatcher *dispatcher) {
  if (Q_UNLIKELY(signalIndex < 0)) {
    return false;
  }
  auto slotObject = new DelegateSlotObject(gcHandle, dispatcher);

  return QObjectPrivate::connect(obj, signalIndex, slotObject, Qt::DirectConnection);
}
