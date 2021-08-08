
#include "../interop/string_interop.h"
#include "../utils.h"
#include "Logger.h"

using LogCallback = void(LogLevel, const char16_t *, unsigned int);

NATIVE_API void Logger_SetSink(LogCallback *callback) {
  static_assert(sizeof(wchar_t) == sizeof(char16_t), ".NET uses UTF-16");
  Logger::SetSink([=](LogLevel level, std::wstring_view message) {
    callback(level, reinterpret_cast<const char16_t *>(message.data()), message.length());
  });
}

NATIVE_API void Logger_ClearSink() {
  Logger::ClearSink();
}
