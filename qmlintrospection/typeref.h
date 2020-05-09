
#pragma once

#include <type_traits>

#include <QVariant>
#include <utility>

enum class TypeRefKind : int {
  Void = 0,
  /**
   * References a TypeInfo
   */
  TypeInfo,
  /**
   * See BuiltInTypes
   */
  BuiltIn,
  /**
   * QQmlListProperty, where TypeInfo is filled.
   */
  QmlList,
  /**
   * Enumeration reference. Contained within TypeInfo.
   */
  TypeInfoEnum,
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
  String,
  ByteArray,
  DateTime,
  Date,
  Time,
  Color,
  Size,
  SizeFloat,
  Rectangle,
  RectangleFloat,
  Point,
  PointFloat,
  Url,
  OpaquePointer,
  CompletionSource
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

  static TypeRef fromTypeInfoList(const TypeInfo *typeInfo) {
    TypeRef result;
    result.kind = TypeRefKind::QmlList;
    result.typeInfo = typeInfo;
    return result;
  }

  static TypeRef fromTypeInfoEnum(const TypeInfo *typeInfo, QByteArray enumName) {
    TypeRef result;
    result.kind = TypeRefKind::TypeInfoEnum;
    result.typeInfo = typeInfo;
    result.enumName = std::move(enumName);
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
  QByteArray enumName{};
};
