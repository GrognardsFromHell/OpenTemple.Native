
#include "../../thirdparty/WinReg.hpp"
#include "../interop/string_interop.h"
#include "../utils.h"

using namespace winreg;

NATIVE_API wchar_t *FindInstallDirectory() {
  // GoG Product ID for ToEE is 1207658889, and by pure experimentation, we
  // found the following registry key:
  // HKEY_LOCAL_MACHINE\SOFTWARE\WOW6432Node\GOG.com\Games\1207658889\path
  try {
    RegKey regKey(HKEY_LOCAL_MACHINE, L"SOFTWARE\\GOG.com\\Games\\1207658889",
                  KEY_READ | KEY_WOW64_32KEY);

    auto path = regKey.GetStringValue(L"path");

    return copyString(path.data());
  } catch (const RegException &) {
    return nullptr;
  }
}
