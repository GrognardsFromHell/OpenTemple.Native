
#pragma once

#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <Commctrl.h>
#include <atlbase.h>
#include <wincrypt.h>
#include <shellapi.h>

#include <system_error>

inline std::error_code last_error_code() noexcept {
    return std::error_code{static_cast<int>(GetLastError()), std::system_category()};
}

inline const wchar_t *GetString(UINT id) {
    wchar_t *ptr;
    LoadString(nullptr, id, reinterpret_cast<LPWSTR>(&ptr), 0);
    return ptr;
}
