
#include "../game/utils.h"
#include "managed_delegate.h"
#include "methodinfo.h"
#include "propinfo.h"
#include "qmlinfo.h"
#include "string_interop.h"
#include "typeinfo.h"
#include "typelibrary.h"
#include "typeref.h"

// ALl of these methods are purely exported, and not used here

#pragma region MethodInfo Wrappers

NATIVE_API char* MethodInfo_name(MethodInfo& method) { return copyString(method.name); }

NATIVE_API char* MethodInfo_signature(MethodInfo& method) { return copyString(method.signature); }

NATIVE_API const TypeRef* MethodInfo_returnType(MethodInfo& method) { return &method.returnType; }

NATIVE_API int MethodInfo_overloadIndex(MethodInfo& method) { return method.overloadIndex; }

NATIVE_API int MethodInfo_params_size(MethodInfo& method) { return (int) method.params.size(); }

NATIVE_API char* MethodInfo_params_name(MethodInfo& method, int index) {
  return copyString(method.params[index].name);
}

NATIVE_API const TypeRef* MethodInfo_params_type(MethodInfo& method, int index) {
  return &method.params[index].type;
}

#pragma endregion

#pragma region PropInfo Wrappers

NATIVE_API char16_t* PropInfo_name(PropInfo& prop) {
  return copyString(prop.name);
}

NATIVE_API bool PropInfo_readable(PropInfo& prop) { return prop.readable; }

NATIVE_API bool PropInfo_writable(PropInfo& prop) { return prop.writable; }

NATIVE_API bool PropInfo_resettable(PropInfo& prop) { return prop.resettable; }

NATIVE_API const TypeRef* PropInfo_type(PropInfo& prop) { return &prop.type; }

#pragma endregion QmlInfo Wrappers

NATIVE_API QmlInfo* QmlInfo_new(NativeDelegate<void(LogLevel, char16_t*)> logger,
    const char16_t *basePath) {
  std::function<void(LogLevel, char16_t*)> loggerFn = logger;

  QmlInfoLogger qmlLogger = [loggerFn](auto level, auto message) {
    loggerFn(level, copyString(message));
  };

  return new QmlInfo(qmlLogger, QString::fromUtf16(basePath));
}

NATIVE_API void QmlInfo_delete(QmlInfo* qmlInfo) { delete qmlInfo; }

NATIVE_API char16_t* QmlInfo_error(QmlInfo& qmlInfo) {
  return copyString(qmlInfo.error());
}

NATIVE_API bool QmlInfo_addFile(QmlInfo& qmlInfo, const char16_t* path) {
  return qmlInfo.addFile(QString::fromUtf16(path));
}

NATIVE_API bool QmlInfo_addNativeModule(QmlInfo& qmlInfo, const char16_t* uri, int majorVersion) {
  return qmlInfo.addNativeModule(QString::fromUtf16(uri), majorVersion);
}

NATIVE_API bool QmlInfo_visitTypeLibrary(QmlInfo& qmlInfo,
                                         void (*visitor)(const TypeLibrary&)) {
  return qmlInfo.visitTypeLibrary(visitor);
}

#pragma endregion

#pragma region TypeInfo Wrappers

NATIVE_API TypeInfoKind TypeInfo_kind(const TypeInfo &info) {
  return info.kind;
}

NATIVE_API char16_t *TypeInfo_metaClassName(const TypeInfo &info) {
  return copyString(info.metaClassName);
}

NATIVE_API char16_t *TypeInfo_name(const TypeInfo &info) {
  return copyString(info.name);
}

NATIVE_API char16_t *TypeInfo_qmlModule(const TypeInfo &info) {
  return copyString(info.qmlModule);
}

NATIVE_API int TypeInfo_qmlMajorVersion(const TypeInfo &info) {
  return info.qmlMajorVersion;
}

NATIVE_API int TypeInfo_qmlMinorVersion(const TypeInfo &info) {
  return info.qmlMinorVersion;
}

NATIVE_API bool TypeInfo_qmlCreatable(const TypeInfo &info) {
  return info.qmlCreatable;
}

NATIVE_API char16_t *TypeInfo_qmlSourceUrl(const TypeInfo &info) {
  return copyString(info.qmlSourceUrl);
}

NATIVE_API const TypeInfo *TypeInfo_enclosingType(const TypeInfo &info) {
  return info.enclosingType;
}

NATIVE_API char16_t *TypeInfo_inlineComponentName(const TypeInfo &info) {
  return copyString(info.inlineComponentName);
}

NATIVE_API int TypeInfo_props_size(const TypeInfo &info) {
  return (int)info.props.size();
}

NATIVE_API const PropInfo *TypeInfo_props(const TypeInfo &info, int index) {
  return &info.props[index];
}

NATIVE_API int TypeInfo_signals_size(const TypeInfo &info) {
  return (int)info.signalInfos.size();
}

NATIVE_API const MethodInfo *TypeInfo_signals(const TypeInfo &info, int index) {
  return &info.signalInfos[index];
}

NATIVE_API int TypeInfo_methods_size(const TypeInfo &info) {
  return (int)info.methods.size();
}

NATIVE_API const MethodInfo *TypeInfo_methods(const TypeInfo &info, int index) {
  return &info.methods[index];
}

NATIVE_API int TypeInfo_constructors_size(const TypeInfo &info) {
  return (int)info.constructors.size();
}

NATIVE_API const MethodInfo *TypeInfo_constructors(const TypeInfo &info, int index) {
  return &info.constructors[index];
}

NATIVE_API int TypeInfo_enums_size(const TypeInfo &info) {
  return (int)info.enums.size();
}

NATIVE_API const EnumInfo *TypeInfo_enums(const TypeInfo &info, int index) {
  return &info.enums[index];
}

NATIVE_API const TypeInfo *TypeInfo_parent(const TypeInfo &info) {
  return info.parent;
}

#pragma endregion

#pragma region TypeLibrary Wrappers

NATIVE_API void TypeLibrary_visitTypes(
    TypeLibrary &library, void (*visitor)(const TypeInfo *)) {
  library.visitTypes(visitor);
}

#pragma endregion

#pragma region TypeRef Wrappers

NATIVE_API TypeRefKind TypeRef_kind(TypeRef &typeRef) { return typeRef.kind; }

NATIVE_API const TypeInfo *TypeRef_typeInfo(TypeRef &typeRef) {
  return typeRef.typeInfo;
}

NATIVE_API BuiltInType TypeRef_builtIn(TypeRef &typeRef) {
  return typeRef.builtIn;
}

#pragma endregion

#pragma region EnumInfo Wrappers

NATIVE_API char16_t* EnumInfo_name(EnumInfo &info) {
  return copyString(info.name);
}

NATIVE_API int EnumInfo_values_size(EnumInfo &info) {
  return (int) info.values.size();
}

NATIVE_API const EnumValueInfo * EnumInfo_values(EnumInfo &info, int index) {
  return &info.values[index];
}

#pragma endregion

#pragma region EnumValueInfo Wrappers

NATIVE_API char16_t* EnumValueInfo_name(EnumValueInfo &info) {
  return copyString(info.name);
}

NATIVE_API int EnumValueInfo_value(EnumValueInfo &info) {
  return info.value;
}

#pragma endregion
