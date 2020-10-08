
#include <utility>
#include <QCoreApplication>

#include "translator.h"
#include "utils.h"
#include "managed_delegate.h"

bool DynamicTranslator::isEmpty() const { return false; }

QString DynamicTranslator::translate(const char *context, const char *sourceText,
                                     const char *disambiguation, int n) const {
  return QString(
      reinterpret_cast<const QChar *>(_translator(context, sourceText, disambiguation, n)));
}

DynamicTranslator::DynamicTranslator(
    std::function<const char16_t *(const char *, const char *, const char *, int)> translator)
    : _translator(std::move(translator)) {}

NATIVE_API void translator_install(NativeDelegate<const char16_t *(const char *, const char *, const char *, int)> translator) {
  QCoreApplication::installTranslator(new DynamicTranslator(translator));
}
