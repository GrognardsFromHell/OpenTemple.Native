
#include <memory>

#include <private/qqmlcomponent_p.h>
#include <private/qqmlpropertycachemethodarguments_p.h>

#include "typelibrary.h"

bool TypeLibrary::addComponent(QQmlComponent* component) {
  auto cp = QQmlComponentPrivate::get(component);
  auto cu = cp->compilationUnit;
  if (cu->metaTypeId == 0) {
    qDebug() << "Component" << cu->url() << "has no meta type id.";
    return false;
  }

  return (bool)addQmlType(cu.data());
}

TypeInfo* TypeLibrary::addCppType(const QMetaObject* metaObject) {
  if (_complexTypes.find(metaObject) != _complexTypes.end()) {
    return _complexTypes[metaObject].get();
  }

  qDebug() << "Adding C++ type " << metaObject->className();

  TypeInfo* parent = nullptr;
  if (metaObject->superClass()) {
    parent = addCppType(metaObject->superClass());
  }

  QString typeName = metaObject->className();

  // Finding a type by it's meta object is sadly not unique
  // We'll attempt to use the FIRST registration here
  QQmlType qmlType;
  for (const auto& potentialType : QQmlMetaType::qmlAllTypes()) {
    if (potentialType.metaObject() == metaObject) {
      qmlType = potentialType;
      break;
    }
  }

  if (qmlType.isValid()) {
    // Note: C++ name is often less ambiguous than QML Element name
    // Decide what to use (i.e. QQuickItem has QML name "")
    typeName = qmlType.elementName();
    // Non-QObject types registered may have no element name
    if (typeName.isEmpty()) {
      typeName = qmlType.typeName();
    }
  }

  auto kind = TypeInfoKind::CppQObject;
  if (!metaObject->inherits(&QObject::staticMetaObject)) {
    kind = TypeInfoKind::CppGadget;
  }

  // We need to create and add the type before processing props
  // Because they can refer to it
  auto typeInfo = std::make_unique<TypeInfo>(kind, metaObject->className(), typeName);
  if (qmlType.isValid()) {
    typeInfo->qmlModule.push_back(qmlType.module());
    typeInfo->qmlMajorVersion = qmlType.majorVersion();
    typeInfo->qmlMinorVersion = qmlType.minorVersion();
    typeInfo->qmlCreatable = qmlType.isCreatable();
  }
  auto result = typeInfo.get();
  result->parent = parent;
  _complexTypes[metaObject] = std::move(typeInfo);

  auto& props = result->props;
  props.reserve(metaObject->propertyCount() - metaObject->propertyOffset());
  for (auto i = metaObject->propertyOffset(); i < metaObject->propertyCount(); i++) {
    auto metaProp = metaObject->property(i);
    auto typeRef = resolveMetaTypeRef((QMetaType::Type)metaProp.userType());
    if (typeRef.kind == TypeRefKind::Void) {
      qDebug() << "Skipping property" << metaProp.name() << "due to unknown type"
               << metaProp.typeName();
      continue;
    }
    auto& prop = props.emplace_back(metaProp.name(), typeRef);
    prop.readable = metaProp.isReadable();
    prop.writable = metaProp.isWritable();
    prop.resettable = metaProp.isResettable();
  }

  processMethodsAndSignals(metaObject, result);

  return result;
}

void TypeLibrary::processMethodsAndSignals(const QMetaObject* metaObject, TypeInfo* result) {
  auto &cons = result->constructors;
  cons.reserve(metaObject->constructorCount());
  for (auto i = 0; i < metaObject->constructorCount(); i++) {
    auto metaMethod = metaObject->constructor(i);
    if (metaMethod.access() != QMetaMethod::Public) {
      continue;
    }

    auto returnType = resolveMetaTypeRef((QMetaType::Type)metaMethod.returnType());
    auto methodName = metaMethod.name();
    auto& method = cons.emplace_back(methodName, metaMethod.methodSignature(), returnType);

    if (convertParameters(metaMethod, method)) {
      qDebug() << "Skipping constructor" << metaMethod.methodSignature()
               << "due to unknown parameter types.";
      cons.pop_back();
      continue;
    }
  }

  auto& sigs = result->signalInfos;
  auto& methods = result->methods;
  sigs.reserve(metaObject->methodCount() - metaObject->methodOffset());
  methods.reserve(metaObject->methodCount() - metaObject->methodOffset());
  for (auto i = metaObject->methodOffset(); i < metaObject->methodCount(); i++) {
    auto metaMethod = metaObject->method(i);
    if (metaMethod.access() != QMetaMethod::Public) {
      continue;
    }

    std::vector<MethodInfo>* targetList;
    if (metaMethod.methodType() == QMetaMethod::Signal) {
      targetList = &sigs;
    } else if (metaMethod.methodType() == QMetaMethod::Slot ||
               metaMethod.methodType() == QMetaMethod::Method) {
      targetList = &methods;
    } else {
      // Skip constructors for now
      continue;
    }

    auto returnType = resolveMetaTypeRef((QMetaType::Type)metaMethod.returnType());

    auto methodName = metaMethod.name();
    auto& method = targetList->emplace_back(methodName, metaMethod.methodSignature(), returnType);

    // Figure out if the method is overloaded and assigned increasing indices
    int highestOverloadFound = -1;
    for (auto& otherMethod : *targetList) {
      if (otherMethod.name == methodName && &otherMethod != &method) {
        if (otherMethod.overloadIndex == -1) {
          // Found the first overload for another method
          otherMethod.overloadIndex = 0;
        }
        highestOverloadFound = otherMethod.overloadIndex;
      }
    }
    if (highestOverloadFound != -1) {
      method.overloadIndex = highestOverloadFound + 1;
    }

    if (convertParameters(metaMethod, method)) {
      qDebug() << "Skipping method" << metaMethod.methodSignature()
               << "due to unknown parameter types.";
      targetList->pop_back();
      continue;
    }
  }
}

bool TypeLibrary::convertParameters(const QMetaMethod& metaMethod, MethodInfo& method) {
  // Convert parameters
  bool failedParameters = false;
  auto parameterNames = metaMethod.parameterNames();
  for (auto j = 0; j < metaMethod.parameterCount(); j++) {
    auto paramTypeRef = resolveMetaTypeRef((QMetaType::Type)metaMethod.parameterType(j));
    if (paramTypeRef.kind == TypeRefKind::Void) {
      failedParameters = true;
      break;
    }

    // Ensure it is null if missing
    QByteArray paramName = parameterNames[j];
    if (paramName.isEmpty()) {
      paramName = QByteArray();
    }
    method.params.emplace_back(std::move(paramName), paramTypeRef);
  }
  return failedParameters;
}

static bool getTypeNameAndModuleFromQmlFile(const QUrl& url, const QDir& basePath,
                                            QUrl* runtimeSourceUrl, QString* moduleName,
                                            QString* typeName) {
  // Many components are not part of an actual _module_
  // so we'll instead check if they are relative to the base path
  // and build a path to the file ourselves
  if (url.isLocalFile()) {
    QString relativePath = basePath.relativeFilePath(url.toLocalFile());
    if (!relativePath.isNull()) {
      *runtimeSourceUrl = QUrl(relativePath);
      QFileInfo relativeFileInfo(relativePath);
      *typeName = relativeFileInfo.baseName();
      *moduleName = relativeFileInfo.dir().path().replace('/', '.').replace('\\', '.');
      if (moduleName->startsWith('.')) {
        moduleName->remove(0, 1);
      }
      if (moduleName->endsWith('.')) {
        moduleName->chop(1);
      }
      if (moduleName->isEmpty()) {
        *moduleName = "QmlFiles";
      } else {
        *moduleName = "QmlFiles." + *moduleName;
      }
    } else {
      // See if there's a qmldir (module-style)
      return false;
    }
  } else if (url.scheme() == "qrc") {
    *runtimeSourceUrl = url;

    // For some reason, lookup via metaObject or type id does not work,
    // but the module has still registered the URL...
    for (const auto& t : QQmlMetaType::qmlTypes()) {
      if (t.sourceUrl() == url) {
        *typeName = t.elementName();
        moduleName->push_back(t.module());
        break;
      }
    }
  }

  return true;
}

TypeInfo* TypeLibrary::addQmlType(QV4::ExecutableCompilationUnit* compilationUnit) {
  auto metaObject = compilationUnit->rootPropertyCache()->createMetaObject();

  if (_complexTypes.find(metaObject) != _complexTypes.end()) {
    return _complexTypes[metaObject].get();
  }

  qDebug() << "Parsing QML file" << compilationUnit->url();

  QString typeName, moduleName;
  QUrl sourceUrl;
  if (!getTypeNameAndModuleFromQmlFile(compilationUnit->finalUrl(), _basePath, &sourceUrl,
                                       &moduleName, &typeName)) {
    return nullptr;
  }

  qDebug() << "Adding QML Type" << moduleName << typeName;

  // We need to create and save the type info immediately, because props
  // can be self-referential
  auto typeInfo = std::make_unique<TypeInfo>(TypeInfoKind::QmlQObject, metaObject->className(), typeName);
  typeInfo->qmlSourceUrl = sourceUrl.toString();
  typeInfo->qmlModule = moduleName;
  auto result = typeInfo.get();
  Q_ASSERT(_complexTypes.find(metaObject) == _complexTypes.end());
  _complexTypes[metaObject] = std::move(typeInfo);

  // Pre-create all inline component types as well
  QHash<int, TypeInfo*> inlineComponentTypes;  // key is the objectIndex
  for (auto& it : compilationUnit->inlineComponentData) {
    auto inlineTypeInfo = std::make_unique<TypeInfo>(TypeInfoKind::QmlQObject,
                                                     metaObject->className(), typeName);
    inlineTypeInfo->qmlModule = moduleName;
    inlineTypeInfo->qmlSourceUrl = sourceUrl.toString();
    inlineTypeInfo->inlineComponentName = compilationUnit->stringAt(it.nameIndex);
    inlineTypeInfo->enclosingType = result;
    auto inlinePc = compilationUnit->propertyCaches.at(it.objectIndex);
    auto inlineMo = inlinePc->createMetaObject();
    inlineComponentTypes[it.objectIndex] = inlineTypeInfo.get();
    qDebug() << "  Inline component:" << inlineTypeInfo->inlineComponentName;
    _complexTypes[inlineMo] = std::move(inlineTypeInfo);
  }

  processQmlComponent(compilationUnit, 0, result);
  for (auto it = inlineComponentTypes.begin(); it != inlineComponentTypes.end(); it++) {
    processQmlComponent(compilationUnit, it.key(), it.value());
  }

  return result;
}

void TypeLibrary::processQmlComponent(const QV4::ExecutableCompilationUnit* compilationUnit,
                                      int objectIndex, TypeInfo* result) {
  // Fully resolve the parent type first
  // QML types can inherit from other QML types or C++
  auto rootObject = compilationUnit->objectAt(objectIndex);
  if (rootObject->inheritedTypeNameIndex != 0) {
    auto parentTypeRef = compilationUnit->resolvedType(rootObject->inheritedTypeNameIndex);
    if (!parentTypeRef) {
      qDebug() << "Failed to resolve parent type of " << compilationUnit->url();
    } else {
      auto parentQmlType = parentTypeRef->type;
      if (parentTypeRef->compilationUnit()) {
        qDebug() << "Parent is QML type";
        result->parent = addQmlType(parentTypeRef->compilationUnit().data());
        // TODO: What if an inline component from another file is referenced???
      } else if (parentQmlType.isValid() && parentQmlType.metaObject()) {
        qDebug() << "Parent is C++ type";
        result->parent = addCppType(parentQmlType.metaObject());
      } else {
        qDebug() << "Cannot handle parent type of " << compilationUnit->url();
      }
    }
  }

  auto pc = compilationUnit->propertyCaches.at(objectIndex);
  auto metaObject = pc->createMetaObject();

  auto& props = result->props;
  props.reserve(pc->propertyCount() - pc->propertyOffset());

  // Process property cache
  for (auto i = pc->propertyOffset(); i < pc->propertyCount(); i++) {
    auto qmlProp = pc->property(i);
    auto name = qmlProp->name(metaObject);
    auto typeRef = resolveQmlPropTypeRef(qmlProp);
    if (typeRef.kind == TypeRefKind::Void) {
      qDebug() << "Skipping property " << name << " because its type is unresolvable";
      continue;
    }
    auto& prop = props.emplace_back(name, typeRef);
    prop.writable = qmlProp->isWritable();
    prop.readable = qmlProp->isValid();
  }

  processMethodsAndSignals(metaObject, result);

  for (auto i = 0; i < pc->qmlEnumCount(); i++) {
    auto qmlEnum = pc->qmlEnum(i);
    auto name = qmlEnum->name;
    auto &enumInfo = result->enums.emplace_back(name);
    enumInfo.values.reserve(qmlEnum->values.size());
    for (auto& qmlEnumValue : qmlEnum->values) {
      enumInfo.values.emplace_back(qmlEnumValue.namedValue, qmlEnumValue.value);
    }
  }
}

void TypeLibrary::visitTypes(void (*visitor)(const TypeInfo*)) {
  for (auto& [_, value] : _complexTypes) {
    visitor(value.get());
  }
}

TypeRef TypeLibrary::resolveMetaTypeRef(QMetaType::Type type) {
  if (type >= QMetaType::User || type == QMetaType::QObjectStar) {
    int userType = (int)type;
    auto it = _complexTypeRefs.find(userType);
    if (it != _complexTypeRefs.end()) {
      return it->second;
    }

    auto typeInfo = typeInfoForUserType(userType);
    if (!typeInfo) {
      return {};
    }
    // TODO: make it void* if typeInfo is null
    _complexTypeRefs.emplace(userType, TypeRef::fromTypeInfo(typeInfo));
    return TypeRef::fromTypeInfo(typeInfo);
  }

  switch (type) {
    case QMetaType::Bool:
      return TypeRef::fromBuiltIn(BuiltInType::Bool);
    case QMetaType::Int:
      return TypeRef::fromBuiltIn(BuiltInType::Int32);
    case QMetaType::UInt:
      return TypeRef::fromBuiltIn(BuiltInType::UInt32);
    case QMetaType::LongLong:
      return TypeRef::fromBuiltIn(BuiltInType::Int64);
    case QMetaType::ULongLong:
      return TypeRef::fromBuiltIn(BuiltInType::UInt64);
    case QMetaType::Double:
      return TypeRef::fromBuiltIn(BuiltInType::Double);
    case QMetaType::Char:
      return TypeRef::fromBuiltIn(BuiltInType::Char);
    case QMetaType::QString:
      return TypeRef::fromBuiltIn(BuiltInType::String);
    default:
      break;
  }

  return {};
}

const TypeInfo* TypeLibrary::typeInfoForUserType(int typeId) {
  auto cu = QQmlEnginePrivate::get(_engine)->obtainExecutableCompilationUnit(typeId);
  if (cu) {
    return addQmlType(cu);
  }

  auto metaObject = QMetaType::metaObjectForType(typeId);
  if (!metaObject) {
    qDebug() << "Could not resolve type id " << typeId;
    return nullptr;
  }

  return addCppType(metaObject);
}
TypeRef TypeLibrary::resolveQmlPropTypeRef(QQmlPropertyData* qmlProp) {
  auto type = qmlProp->flags().type;

  using Types = QQmlPropertyData::Flags::Types;

  switch (type) {
    case Types::OtherType:
      break;
    case Types::FunctionType:
      break;
    case Types::QObjectDerivedType:
      return resolveMetaTypeRef((QMetaType::Type)qmlProp->propType());
    case Types::EnumType:
      break;
    case Types::QListType:
      break;
    case Types::QmlBindingType:
      break;
    case Types::QJSValueType:
      break;
    case Types::VarPropertyType:
      break;
    case Types::QVariantType:
      break;
    default:
      return {};
  }

  return resolveMetaTypeRef((QMetaType::Type)qmlProp->propType());
}
