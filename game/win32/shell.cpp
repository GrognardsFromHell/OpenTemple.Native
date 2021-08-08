
// Enable this to make use of v6 which is needed for the new dialogs
#define ISOLATION_AWARE_ENABLED 1
#pragma comment(lib, "comctl32.lib")
#pragma comment( \
    linker,      \
    "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

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
                                wchar_t **pickedFolder) noexcept {
  *pickedFolder = nullptr;

  winrt::com_ptr<IFileOpenDialog> dialog;

  try {
    dialog = winrt::create_instance<IFileOpenDialog>(winrt::guid_of<FileOpenDialog>());
  } catch (...) {
    return -1;
  }

  dialog->SetTitle(title);

  // Try setting the default folder
  if (startingDirectory) {
    winrt::com_ptr<IShellItem> preselectedFolder;
    if (SUCCEEDED(SHCreateItemFromParsingName(
            startingDirectory, nullptr, IID_PPV_ARGS(preselectedFolder.put())))) {
      dialog->SetFolder(preselectedFolder.get());
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

  winrt::com_ptr<IShellItem> selectedItem;
  if (!SUCCEEDED(dialog->GetResult(selectedItem.put()))) {
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
