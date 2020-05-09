
#pragma once

#include <memory>
#include <utility>

class TypeLibrary;

#include "enuminfo.h"
#include "methodinfo.h"
#include "propinfo.h"

enum class TypeInfoKind : int { CppQObject = 0, CppGadget = 1, QmlQObject = 2, OpaqueValueType = 3 };

/**
 * Describes a native type, which can either be dynamically created when
 * a QML file is loaded, or describe a compiled Qt-type via it's QMetaObject.
 */
class TypeInfo {
 public:
  TypeInfo(TypeInfoKind kind, QString metaClassName, QString name)
      : kind(kind), metaClassName(std::move(metaClassName)), name(std::move(name)) {}

  const TypeInfoKind kind;

  // typeName found in QMetaObject
  const QString metaClassName;

  const QString name;

  // Relevant for OpaqueValueType
  int size = 0;

  // In case the type is exposed to QML via a QML Module, this contains
  // the module's name (optional).
  QString qmlModule;
  int qmlMajorVersion = -1;
  int qmlMinorVersion = -1;
  bool qmlCreatable = false;

  // Source URL of .qml file to retrieve the meta object at runtime
  QString qmlSourceUrl;

  // For QML Inline components
  const TypeInfo *enclosingType{};
  QString inlineComponentName;

  TypeInfo *parent{};

  std::vector<PropInfo> props;

  std::vector<MethodInfo> constructors;

  std::vector<MethodInfo> signalInfos;

  std::vector<MethodInfo> methods;

  std::vector<EnumInfo> enums;
};
