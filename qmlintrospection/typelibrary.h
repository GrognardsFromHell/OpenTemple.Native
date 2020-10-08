
#pragma once

#include <map>

#include <QDir>
#include <QMap>
#include <QList>
#include <QRegExp>
#include <QQmlComponent>
#include <unordered_set>

#include "typeinfo.h"

class QQmlPropertyData;

class TypeLibrary {
 public:
  explicit TypeLibrary(QQmlEngine *engine, const QString &basePath)
      : _engine(engine), _basePath(basePath) {}

  void addExcludePattern(QRegExp excludePattern) {
    _excludePatterns.append(std::move(excludePattern));
  }

  bool addComponent(QQmlComponent *c);
  TypeInfo *addQmlType(QV4::ExecutableCompilationUnit *compilationUnit);
  TypeInfo *addCppType(const QMetaObject *metaObject);

  void visitTypes(void (*visitor)(const TypeInfo *));

 private:
  QQmlEngine *_engine;

  const TypeInfo *typeInfoForUserType(int typeId);

  // Pre-constructed type references for all the objects we know about
  std::map<int, TypeRef> _complexTypeRefs;

  std::map<const QMetaObject *, std::unique_ptr<TypeInfo>> _complexTypes;
  std::map<int, std::unique_ptr<TypeInfo>> _valueTypes;
  TypeRef resolveMetaTypeRef(QMetaType::Type type);

  TypeRef resolveQmlPropTypeRef(QQmlPropertyData *qmlProp);

  QDir _basePath;
  void processMethodsAndSignals(const QMetaObject *metaObject, TypeInfo *result, bool skipMethods = false);
  void processQmlComponent(const QV4::ExecutableCompilationUnit *compilationUnit, int objectIndex,
                           TypeInfo *result);
  bool convertParameters(const QMetaMethod &metaMethod, MethodInfo &method);
  TypeInfo *resolveQmlBaseType(const QV4::ExecutableCompilationUnit *compilationUnit,
                               int objectIndex);
  QList<QRegExp> _excludePatterns;
};
