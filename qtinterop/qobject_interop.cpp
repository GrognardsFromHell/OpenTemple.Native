
#include <QObject>
#include <QQuickItem>
#include <QUrl>

#include <private/qmetaobject_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmltypedata_p.h>

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

NATIVE_API bool QObject_setPropertyQString(QObject *target, int idx, const char16_t *value,
                                           int length) {
  QString string(reinterpret_cast<const QChar *>(value), length);
  void *stringPtr = &string;
  return QMetaObject::metacall(target, QMetaObject::WriteProperty, idx, &stringPtr) == -1;
}

NATIVE_API bool QObject_getPropertyQString(QObject *target, int idx, const char16_t **valueOut,
                                           int *lengthOut) {
  QString string;
  void *stringPtr = &string;
  if (QMetaObject::metacall(target, QMetaObject::ReadProperty, idx, &stringPtr) != -1) {
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

NATIVE_API char16_t *QString_read(const QString *str) { return copyString(*str); }

NATIVE_API QString *QString_create_null() { return new QString(); }

NATIVE_API QString *QString_create(const char16_t *charsPtr, int charCount) {
  if (!charsPtr) {
    return new QString();
  }
  return new QString(reinterpret_cast<const QChar *>(charsPtr), charCount);
}

NATIVE_API void QString_delete(QString *ptr) { delete ptr; }
