
#include "utils.h"

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

NATIVE_API HCURSOR Win32_LoadImageToCursor(uint8_t *pixelData, int width,
                                           int height, uint32_t hotspotX,
                                           uint32_t hotspotY) {
  BITMAPV5HEADER bi;
  ZeroMemory(&bi, sizeof(BITMAPV5HEADER));
  bi.bV5Size = sizeof(BITMAPV5HEADER);
  bi.bV5Width = width;
  bi.bV5Height = -height;
  bi.bV5Planes = 1;
  bi.bV5BitCount = 32;
  bi.bV5Compression = BI_BITFIELDS;
  bi.bV5RedMask = 0x00FF0000;
  bi.bV5GreenMask = 0x0000FF00;
  bi.bV5BlueMask = 0x000000FF;
  bi.bV5AlphaMask = 0xFF000000;
  HDC hdc;
  hdc = GetDC(NULL);

  void *lpBits = nullptr;
  auto hBitmap = CreateDIBSection(hdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS,
                                  (void **)&lpBits, NULL, (DWORD)0);

  memcpy(lpBits, pixelData, width * height * 4);

  ReleaseDC(NULL, hdc);

  auto hMonoBitmap = CreateBitmap(width, height, 1, 1, NULL);

  ICONINFO ii;
  ii.fIcon = FALSE;
  ii.xHotspot = hotspotX;
  ii.yHotspot = hotspotY;
  ii.hbmMask = hMonoBitmap;
  ii.hbmColor = hBitmap;

  auto hAlphaCursor = CreateIconIndirect(&ii);

  DeleteObject(hBitmap);
  DeleteObject(hMonoBitmap);

  return hAlphaCursor;
}
