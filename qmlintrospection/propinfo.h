
#pragma once

#include <QString>
#include <QVariant>
#include <utility>

#include "typeref.h"

class TypeInfo;

class PropInfo {
 public:
  PropInfo(QString name, TypeRef type) : name(std::move(name)), type(type) {}

  const QString name;
  const TypeRef type;

  bool readable{};
  bool writable{};
  bool resettable{};

};
