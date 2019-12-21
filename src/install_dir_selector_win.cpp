
// Enable this to make use of v6 which is needed for the new dialogs
#define ISOLATION_AWARE_ENABLED 1
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "windows_headers.h"

#include "utils.h"

// We do not use PInvoke directly for this because we need the ComCtl v6 indirection
// Normally this would not be an issue, but JetBrains Rider does not use the generated
// executable for debugging, which means our app manifest in .NET will not be picked up.
NATIVE_API ApiBool SelectInstallationDirectory(
        const wchar_t *title,
        const wchar_t *prompt,
        const wchar_t *currentDirectory,
        wchar_t **newDirectory
) {
    *newDirectory = nullptr;

    int button = 0;
    TaskDialog(
            NULL,
            NULL,
            L"Temple of Elemental Evil Files",
            L"Choose Temple of Elemental Evil Installation",
            L"The Temple of Elemental Evil data files are required to run OpenTemple.\n\n"
            L"Please selected the Folder where you have installed Temple of Elemental Evil to continue.",
            TDCBF_OK_BUTTON | TDCBF_CLOSE_BUTTON,
            TD_INFORMATION_ICON,
            &button
    );

    if (button != IDOK) {
        return false;
    }

    CComPtr<IFileOpenDialog> dialog;

    if (!SUCCEEDED(dialog.CoCreateInstance(__uuidof(FileOpenDialog)))) {
        return false;
    }

    dialog->SetTitle(L"Choose Temple of Elemental Evil Folder");

    // Try setting the default folder
    if (currentDirectory) {
        CComPtr<IShellItem> preselectedFolder;
        if (SUCCEEDED(SHCreateItemFromParsingName(currentDirectory, nullptr, IID_PPV_ARGS(&preselectedFolder)))) {
            dialog->SetFolder(preselectedFolder);
        }
    }

    // Make sure the dialog is the select-folder kind
    FILEOPENDIALOGOPTIONS options{};
    if (!SUCCEEDED(dialog->GetOptions(&options))) {
        return false;
    }
    options |= FOS_PATHMUSTEXIST | FOS_PICKFOLDERS;
    dialog->SetOptions(options);

    if (!SUCCEEDED(dialog->Show(nullptr))) {
        // NOTE: This will also fail if the user just cancels
        return false;
    }

    CComPtr<IShellItem> selectedItem;
    if (!SUCCEEDED(dialog->GetResult(&selectedItem))) {
        return false;
    }

    *newDirectory = nullptr;
    if (!SUCCEEDED(selectedItem->GetDisplayName(SIGDN_FILESYSPATH, newDirectory))) {
        return false;
    }

    return true;
}

NATIVE_API void FreeSelectInstallationDirectory(wchar_t *selectedPath) {
    CoTaskMemFree(selectedPath);
}
