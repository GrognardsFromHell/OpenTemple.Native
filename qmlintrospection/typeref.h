
#pragma once

#include <type_traits>

#include <QVariant>

enum class TypeRefKind : int {
  Void = 0,
  /**
   * References a TypeInfo
   */
  TypeInfo,
  /**
   * See BuiltInTypes
   */
  BuiltIn
};

class TypeInfo;

enum class BuiltInType : int {
  Bool = 0,
  Int32,
  UInt32,
  Int64,
  UInt64,
  Double,
  Char,
  String
};

/**
 * Typeref can be:
 * - Anything a QVariant could be
 * - Additionally, QML extends this further, especially it can now be an enum
 */
class TypeRef {
 public:
  static TypeRef fromTypeInfo(const TypeInfo *typeInfo) {
    TypeRef result;
    result.kind = TypeRefKind::TypeInfo;
    result.typeInfo = typeInfo;
    return result;
  }
  static TypeRef fromBuiltIn(BuiltInType builtInType) {
    TypeRef result;
    result.kind = TypeRefKind::BuiltIn;
    result.builtIn = builtInType;
    return result;
  }

  TypeRefKind kind = TypeRefKind::Void;
  const TypeInfo *typeInfo{};
  BuiltInType builtIn{};
};
