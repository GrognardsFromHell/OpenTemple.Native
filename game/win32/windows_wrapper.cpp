
#include "../utils.h"

#include <cstdint>
#include <vector>

#include "windows_headers.h"

NATIVE_API ApiBool Win32_GetMonitorName(HMONITOR monitor, wchar_t *deviceName,
                                        uint32_t *deviceNameLength) {
  MONITORINFOEXW monInfoEx{};
  monInfoEx.cbSize = sizeof(MONITORINFOEXW);
  if (!GetMonitorInfoW(monitor, &monInfoEx)) {
    return false;
  }

  DISPLAY_DEVICEW dispDev{};
  dispDev.cb = sizeof(DISPLAY_DEVICEW);
  if (!EnumDisplayDevicesW(monInfoEx.szDevice, 0, &dispDev, 0)) {
    return false;
  }

  if (*deviceNameLength * sizeof(wchar_t) <
      sizeof(DISPLAY_DEVICEW::DeviceString)) {
    return false;
  }

  memcpy(deviceName, dispDev.DeviceString, sizeof(dispDev.DeviceString));

  auto actualLength = wcslen(deviceName);
  if (actualLength < *deviceNameLength) {
    *deviceNameLength = (uint32_t)actualLength;
  }

  return true;
}
