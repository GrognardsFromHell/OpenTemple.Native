
#include <QString>
#include <QByteArray>

#include "string_interop.h"

#ifdef Q_OS_WIN
#include <combaseapi.h>
#define interop_malloc CoTaskMemAlloc
#else
#define interop_malloc malloc
#endif

char16_t *copyString(const QString &str) {
  if (str.isNull()) {
    return nullptr;
  }

  auto len = str.length();
  auto result = reinterpret_cast<char16_t *>(interop_malloc(len * 2 + 2));

  memcpy(result, str.utf16(), sizeof(char16_t) * len);
  result[len] = 0;

  return result;
}

char *copyString(const QByteArray &str) {
  if (str.isNull()) {
    return nullptr;
  }

  auto len = str.length();
  auto result = reinterpret_cast<char *>(interop_malloc(len + 1));

  memcpy(result, str.constData(), len);
  result[len] = 0;

  return result;
}

char *copyString(const char *str) {
  auto len = strlen(str);
  auto result = reinterpret_cast<char *>(interop_malloc(len + 1));

  memcpy(result, str, len);
  result[len] = 0;

  return result;
}

wchar_t *copyString(const wchar_t *str) {
  auto len = wcslen(str);
  auto result = reinterpret_cast<wchar_t *>(interop_malloc(sizeof(wchar_t) * (len + 1)));

  memcpy(result, str, sizeof(wchar_t) * len);
  result[len] = 0;

  return result;
}
