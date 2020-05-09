#pragma once

#include <QString>
#include <utility>
#include <vector>
#include "typeref.h"

class EnumValueInfo {
 public:
  EnumValueInfo(QString name, const int value) : name(std::move(name)), value(value) {}

  const QString name;
  const int value;
};

class EnumInfo {
 public:
  explicit EnumInfo(QString name) : name(std::move(name)) {}

  const QString name;
  bool flags = false;
  BuiltInType baseType = BuiltInType::Int32;
  std::vector<EnumValueInfo> values;
};
