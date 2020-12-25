
// Enable this to make use of v6 which is needed for the new dialogs
#define ISOLATION_AWARE_ENABLED 1
#pragma comment(lib, "comctl32.lib")
#pragma comment( \
    linker,      \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <string>
#include "../interop/string_interop.h"
#include "../utils.h"
// clang-format off
#include "windows_headers.h"
#include <shellapi.h>
// clang-format on

/**
 * Opens a folder in the user's preferred file browser.
 */
NATIVE_API void Shell_OpenFolder(const wchar_t *path) {
  // On Linux we might use sth like xdg-open
  ShellExecute(nullptr, L"explore", path, nullptr, nullptr, SW_NORMAL);
}

/**
 * Opens a URL in the user's preferred browser.
 */
NATIVE_API void Shell_OpenUrl(const wchar_t *url) {
  // On Linux we might use sth like xdg-open
  ShellExecute(nullptr, L"open", url, nullptr, nullptr, SW_NORMAL);
}

/**
 * Allows the user to pick a folder on their system.
 * Return values:
 * - 0: Success
 * - 1: User Canceled
 * - Any negative: Error
 */
NATIVE_API int Shell_PickFolder(wchar_t *title,
                                wchar_t *startingDirectory,
                                wchar_t **pickedFolder) {
  *pickedFolder = nullptr;
  CComPtr<IFileOpenDialog> dialog;

  if (!SUCCEEDED(dialog.CoCreateInstance(__uuidof(FileOpenDialog)))) {
    return -1;
  }

  dialog->SetTitle(title);

  // Try setting the default folder
  if (startingDirectory) {
    CComPtr<IShellItem> preselectedFolder;
    if (SUCCEEDED(SHCreateItemFromParsingName(
            startingDirectory, nullptr, IID_PPV_ARGS(&preselectedFolder)))) {
      dialog->SetFolder(preselectedFolder);
    }
  }

  // Make sure the dialog is the select-folder kind
  FILEOPENDIALOGOPTIONS options{};
  if (!SUCCEEDED(dialog->GetOptions(&options))) {
    return -2;
  }
  options |= FOS_PATHMUSTEXIST | FOS_PICKFOLDERS;
  dialog->SetOptions(options);

  if (!SUCCEEDED(dialog->Show(nullptr))) {
    // NOTE: This will also fail if the user just cancels
    return 1;
  }

  CComPtr<IShellItem> selectedItem;
  if (!SUCCEEDED(dialog->GetResult(&selectedItem))) {
    return -3;
  }

  LPWSTR fullPath;
  if (!SUCCEEDED(selectedItem->GetDisplayName(SIGDN_FILESYSPATH, &fullPath))) {
    return -4;
  }

  *pickedFolder = fullPath;
  return 0;
}

// We do not use PInvoke directly for this because we need the ComCtl v6 indirection
// Normally this would not be an issue, but JetBrains Rider does not use the generated
// executable for debugging, which means our app manifest in .NET will not be picked up.
NATIVE_API ApiBool Shell_ShowPrompt(bool errorIcon,
                                    const wchar_t *promptTitle,
                                    const wchar_t *promptEmphasized,
                                    const wchar_t *promptDetailed) {
  int button = 0;
  TaskDialog(nullptr,
             nullptr,
             promptTitle,
             promptEmphasized,
             promptDetailed,
             TDCBF_OK_BUTTON | TDCBF_CLOSE_BUTTON,
             errorIcon ? TD_ERROR_ICON : TD_INFORMATION_ICON,
             &button);

  return button == IDOK;
}

// Similar to Shell_ShowPrompt, but does not allow for cancellation
NATIVE_API void Shell_ShowMessage(bool errorIcon,
                                  const wchar_t *promptTitle,
                                  const wchar_t *promptEmphasized,
                                  const wchar_t *promptDetailed) {
  int button = 0;
  TaskDialog(nullptr,
             nullptr,
             promptTitle,
             promptEmphasized,
             promptDetailed,
             TDCBF_OK_BUTTON,
             errorIcon ? TD_ERROR_ICON : TD_INFORMATION_ICON,
             &button);
}

// Copies the text to the clipboard
// See https://docs.microsoft.com/en-us/windows/win32/dataxchg/using-the-clipboard
NATIVE_API int Shell_CopyToClipboard(HWND windowHandle, wchar_t *text) {
  if (!OpenClipboard(windowHandle)) {
    return -1;
  }

  if (!EmptyClipboard()) {
    CloseClipboard();
    return -2;
  }

  auto charCount = wcslen(text);
  auto hglbCopy = GlobalAlloc(GMEM_MOVEABLE, (charCount + 1) * sizeof(TCHAR));
  if (!hglbCopy) {
    CloseClipboard();
    return -3;
  }

  auto lptstrCopy = GlobalLock(hglbCopy);
  memcpy(lptstrCopy, text, charCount * sizeof(TCHAR));
  ((wchar_t *)lptstrCopy)[charCount] = 0;
  GlobalUnlock(hglbCopy);

  SetClipboardData(CF_UNICODETEXT, hglbCopy);

  CloseClipboard();
  return 0;
}

// Gets the current clipboard content as Unicode text
// See https://docs.microsoft.com/en-us/windows/win32/dataxchg/using-the-clipboard
NATIVE_API int Shell_GetClipboardText(HWND windowHandle, char16_t **text) {
  text = nullptr;

  if (!OpenClipboard(windowHandle)) {
    return -1;
  }

  auto data = reinterpret_cast<wchar_t *>(GetClipboardData(CF_UNICODETEXT));

  if (!data) {
    CloseClipboard();
    return -2;
  }

  *text = copyString(data);
  CloseClipboard();
  return 0;
}
