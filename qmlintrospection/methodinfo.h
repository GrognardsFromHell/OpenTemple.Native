
#pragma once

#include <QString>
#include <utility>
#include "typeref.h"

struct MethodParamInfo {
  const QByteArray name;
  const TypeRef type;

  MethodParamInfo(QByteArray name, const TypeRef& type) : name(std::move(name)), type(type) {}
};

class MethodInfo {
 public:
  MethodInfo(QByteArray name, QByteArray signature, TypeRef returnType)
      : name(std::move(name)), signature(std::move(signature)), returnType(returnType) {}

  const QByteArray name;

  // From QMetaMethod::methodSignature to allow for resolution of overloads at runtime
  const QByteArray signature;

  const TypeRef returnType;

  std::vector<MethodParamInfo> params;

  // -1 = no overloads exist, otherwise index starting at 0
  int overloadIndex = -1;

};
