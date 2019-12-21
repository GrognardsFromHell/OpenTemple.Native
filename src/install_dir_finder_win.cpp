
#include "utils.h"
#include "../thirdparty/WinReg.hpp"

using namespace winreg;

NATIVE_API bool FindInstallDirectory(wchar_t **directoryOut) {
    *directoryOut = nullptr;

    // GoG Product ID for ToEE is 1207658889, and by pure experimentation, we
    // found the following registry key:
    // HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\GOG.com\Games\1207658889\path
    try {
        RegKey regKey(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\GOG.com\\Games\\1207658889",
                      KEY_READ | KEY_WOW64_32KEY);

        auto path = regKey.GetStringValue(L"path");

        *directoryOut = (wchar_t *) CoTaskMemAlloc((path.size() + 1) * sizeof(wchar_t));
        wcsncpy(*directoryOut, path.data(), path.size());
        return true;
    } catch (const RegException &) {
        return false;
    }

}
