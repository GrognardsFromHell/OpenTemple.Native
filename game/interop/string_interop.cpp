
#include "string_interop.h"
#include <combaseapi.h>
#include <codecvt>
#include <string>

/*
 * As per .NET docs, on Windows, we can directly return strings from P/Invoke as long as they
 * have been allocated with CoTaskMemAlloc.
 */

// This is obviously not true for Linux at the moment, and needs to be redone
static_assert(sizeof(wchar_t) == sizeof(char16_t));

char16_t *copyString(const std::wstring_view &str) {
  auto len = str.length();
  auto result = reinterpret_cast<char16_t *>(CoTaskMemAlloc(len * 2 + 2));

  memcpy(result, str.data(), sizeof(char16_t) * len);
  result[len] = 0;

  return result;
}

char *copyString(const std::string_view &str) {
  auto len = str.length();
  auto result = reinterpret_cast<char *>(CoTaskMemAlloc(len + 1));

  memcpy(result, str.data(), len);
  result[len] = 0;

  return result;
}

char *copyString(const char *str) {
  auto len = strlen(str);
  auto result = reinterpret_cast<char *>(CoTaskMemAlloc(len + 1));

  memcpy(result, str, len);
  result[len] = 0;

  return result;
}

char16_t *copyString(const wchar_t *str) {
  auto len = wcslen(str);
  auto result = reinterpret_cast<char16_t *>(CoTaskMemAlloc(sizeof(wchar_t) * (len + 1)));

  memcpy(result, str, sizeof(wchar_t) * len);
  result[len] = 0;

  return result;
}

std::wstring localToWide(std::string_view view) {
  std::wstring result;

  auto wideLength = MultiByteToWideChar(CP_ACP, 0, view.data(), view.length(), nullptr, 0);
  if (wideLength == 0) {
    return result;
  }

  result.resize(wideLength);

  if (!MultiByteToWideChar(CP_ACP, 0, view.data(), view.length(), &result[0], result.length())) {
    result.resize(0);
    return result;
  }

  return result;
}
