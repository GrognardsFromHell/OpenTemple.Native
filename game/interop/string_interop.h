
#pragma once

#include <string>

[[maybe_unused]] char16_t *copyString(const std::wstring_view &);
[[maybe_unused]] char *copyString(const std::string_view &);
[[maybe_unused]] char *copyString(const char *);
[[maybe_unused]] char16_t *copyString(const wchar_t *);

/**
 * Converts from whatever the platform uses as it's default encoding to a wide string.
 */
[[maybe_unused]] std::wstring localToWide(std::string_view view);
