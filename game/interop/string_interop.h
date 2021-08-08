
#pragma once

#include <string>

[[maybe_unused]] char16_t *copyString(const std::wstring_view &);
[[maybe_unused]] char *copyString(const std::string_view &);
[[maybe_unused]] char *copyString(const char *);
[[maybe_unused]] char16_t *copyString(const wchar_t *);
