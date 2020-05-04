
#include <private/qqmlengine_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmltypemodule_p.h>
#include <QFile>
#include <utility>

#include "../game/utils.h"

#include "qmlinfo.h"
#include "typelibrary.h"

QmlInfo::QmlInfo(decltype(_logger) logger, QString basePath)
    : _logger(std::move(logger)), _basePath(std::move(basePath)) {
  int argc = 0;
  char* argv[1] = {{}};
  if (!QCoreApplication::instance()) {
    _app = std::make_unique<QGuiApplication>(argc, argv);
  }

  _engine = std::make_unique<QQmlEngine>();

  QObject::connect(_engine.get(), &QQmlEngine::warnings, [this](auto warnings) {
    for (auto warning : warnings) {
      _logger(LogLevel::WARNING, warning.toString());
    }
  });
}

bool QmlInfo::addFile(const QString& path) {
  _error.clear();

  auto component =
      std::make_unique<QQmlComponent>(_engine.get(), path, QQmlComponent::PreferSynchronous);
  if (component->status() == QQmlComponent::Error) {
    _logger(LogLevel::ERROR, component->errorString());
    return false;
  }
  if (component->status() != QQmlComponent::Ready) {
    _logger(LogLevel::ERROR, "Component did not load.");
    return false;
  }

  _components.push_back(std::move(component));
  return true;
}

bool QmlInfo::visitTypeLibrary(void (*visitor)(const TypeLibrary&)) {
  TypeLibrary typeLibrary(_engine.get(), _basePath);

  for (auto metaObject : _cppTypes) {
    typeLibrary.addCppType(metaObject);
  }

  for (const auto& component : _components) {
    typeLibrary.addComponent(component.get());
  }

  visitor(typeLibrary);

  return true;
}

bool QmlInfo::addNativeModule(const QString& uri, int majorVersion) {
  // Lazily register static types, if any
  auto typeModule = QQmlMetaType::typeModule(uri, majorVersion);
  if (!typeModule) {
    QQmlMetaType::qmlRegisterModuleTypes(uri, majorVersion);
    typeModule = QQmlMetaType::typeModule(uri, majorVersion);
    if (!typeModule) {
      return false;
    }
  }

  // May contain many duplicate registrations
  QHashedString moduleName(typeModule->module());
  QSet<QString> foundTypes;
  for (const auto& type : QQmlMetaType::qmlAllTypes()) {
    if (type.module() == moduleName && type.majorVersion() == majorVersion) {
      foundTypes.insert(type.typeName());
    }
  }

  // We can however retrieve the types by name from the type module
  auto maxMinor = typeModule->maximumMinorVersion();
  for (const auto& typeName : foundTypes) {
    auto type = typeModule->type(typeName, maxMinor);
    if (type.metaObject()) {
      _cppTypes.insert(type.metaObject());
    }
  }

  return true;
}

QmlInfo::~QmlInfo() = default;
