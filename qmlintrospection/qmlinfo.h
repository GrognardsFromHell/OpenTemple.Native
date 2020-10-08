
#pragma once

#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlError>
#include <QString>
#include <QRegExp>
#include <utility>

#include "typeinfo.h"

class TypeInfo;

enum class LogLevel : int { WARNING = 0, ERROR = 1 };

using QmlInfoLogger = std::function<void(LogLevel, const QString&)>;

class QmlInfo {
 public:
  QmlInfo(QmlInfoLogger logger, QString basePath);
  ~QmlInfo();

  bool addFile(const QString& path);
  bool addNativeModule(const QString& uri, int majorVersion);
  bool addMetaClass(const char *metaClass);
  void addImportPath(const QString &path);
  bool addExcludePattern(const QString &pattern);

  bool visitTypeLibrary(void (*visitor)(const TypeLibrary&));

  [[nodiscard]] QV4::ExecutableCompilationUnit* findCompilationUnit(
      int typeId) const;

  [[nodiscard]] const QString& error() const { return _error; }

 private:
  std::unique_ptr<QGuiApplication> _app;
  std::unique_ptr<QQmlEngine> _engine;
  QString _error;
  QmlInfoLogger _logger;
  std::vector<std::unique_ptr<QQmlComponent>> _components;
  QSet<const QMetaObject*> _cppTypes;
  QString _basePath;
  QList<QRegExp> _excludePatterns;
};
