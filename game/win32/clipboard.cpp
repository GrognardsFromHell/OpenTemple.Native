
#include "../interop/string_interop.h"
#include "../utils.h"
#include "windows_headers.h"

struct ClipboardCloser {
  ~ClipboardCloser() {
    CloseClipboard();
  }
};

// Copies the text to the clipboard
// See https://docs.microsoft.com/en-us/windows/win32/dataxchg/using-the-clipboard
NATIVE_API int Shell_SetClipboardText(HWND windowHandle, wchar_t *text) {
  if (!OpenClipboard(windowHandle)) {
    return -1;
  }
  ClipboardCloser closer;

  if (!EmptyClipboard()) {
    return -2;
  }

  auto charCount = wcslen(text);
  auto hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (charCount + 1) * sizeof(TCHAR));
  if (!hglbCopy) {
    return -3;
  }

  auto lptstrCopy = GlobalLock(hglbCopy);
  memcpy(lptstrCopy, text, charCount * sizeof(TCHAR));
  ((wchar_t *)lptstrCopy)[charCount] = 0;
  GlobalUnlock(hglbCopy);

  SetClipboardData(CF_UNICODETEXT, hglbCopy);

  return 0;
}

// Gets the current clipboard content as Unicode text
// See https://docs.microsoft.com/en-us/windows/win32/dataxchg/using-the-clipboard
NATIVE_API int Shell_GetClipboardText(HWND windowHandle, char16_t **text) {
  *text = nullptr;

  if (!OpenClipboard(windowHandle)) {
    return -1;
  }
  ClipboardCloser closer;

  auto handle = GetClipboardData(CF_UNICODETEXT);

  if (!handle) {
    return -2;
  }

  auto data = reinterpret_cast<wchar_t *>(GlobalLock(handle));
  *text = copyString(data);
  GlobalUnlock(handle);

  return 0;
}
