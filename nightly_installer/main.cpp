
#include <system_error>

#include "os.h"
#include "resource.h"
#include "certificate.h"

bool ShowUserConfirmation(HINSTANCE hInstance, CertificateContext &cert);

bool IsElevated();

void LaunchInstaller();

int wWinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPWSTR lpCmdLine,
        int nShowCmd
) {

    try {
        CertificateContext cert;
        cert.LoadFromResource();

        if (IsElevated()) {
            CertificateStore store(false);
            if (!store.Contains(cert)) {
                store.Install(cert);
            }
            LaunchInstaller();
            return 0;
        } else {
            CertificateStore store(true);

            // Check if the certificate is already installed, if that's the case, we can skip straight to installing
            if (store.Contains(cert)) {
                LaunchInstaller();
                return 0;
            }
        }

        if (!ShowUserConfirmation(hInstance, cert)) {
            return -1;
        }

        // Try figuring out our own executable name
        wchar_t *executable_path;
        int error = _get_wpgmptr(&executable_path);
        if (error) {
            throw std::system_error(error, std::system_category(), "Failed to find own executable path.");
        }

        // Relaunch ourselves with elevation
        ShellExecute(
                nullptr,
                L"runas",
                executable_path,
                L"",
                nullptr,
                SW_NORMAL
        );

        return 0;
    } catch (const std::exception &e) {
        MessageBoxA(NULL, e.what(), "Fatal Error", MB_OK | MB_ICONERROR);
        return -1;
    }
}

bool ShowUserConfirmation(HINSTANCE hInstance, CertificateContext &cert) {

    int nButtonPressed = 0;
    TASKDIALOGCONFIG config = {0};
    const TASKDIALOG_BUTTON buttons[] = {
            {IDOK, L"Install"}
    };
    config.cbSize = sizeof(config);
    config.dwFlags = TDF_ENABLE_HYPERLINKS;
    config.hInstance = hInstance;
    config.dwCommonButtons = TDCBF_CANCEL_BUTTON;
    config.pszMainIcon = MAKEINTRESOURCEW(1);
    config.pszWindowTitle = L"OpenTemple Nightly Installer";
    config.pszMainInstruction = L"Install OpenTemple Nightly Builds";
    config.pszContent = L"Continuing will install the <a href=\"cert\">security certificate</a> required to use OpenTemple nightly builds.\n\n"
                        L"Please keep in mind that this is only intended for development and testing purposes!";
    config.pButtons = buttons;
    config.cButtons = ARRAYSIZE(buttons);
    // This callback will set the shield icon on the Install button, since it requires UAC
    // See: https://stackoverflow.com/questions/5350721/how-do-i-add-the-uac-shield-icon-to-the-standard-messagebox

    struct DialogState {
        CertificateContext &cert;
    } state{cert};
    config.lpCallbackData = reinterpret_cast<LONG_PTR>(&state);
    config.pfCallback = [](
            HWND hwnd,
            UINT msg,
            WPARAM wParam,
            LPARAM lParam,
            LONG_PTR dwRefData
    ) {
        auto state = reinterpret_cast<DialogState *>(dwRefData);

        if (msg == TDN_CREATED) {
            SendMessage(
                    hwnd,
                    TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE,
                    IDOK,
                    1
            );
        } else if (msg == TDN_HYPERLINK_CLICKED) {
            // The link to show the cert was clicked
            auto clickedLink = reinterpret_cast<const wchar_t *>(lParam);
            if (!wcscmp(clickedLink, L"cert")) {
                state->cert.ShowDialog(hwnd);
            }
        }
        return S_OK;
    };

    TaskDialogIndirect(&config, &nButtonPressed, NULL, NULL);
    switch (nButtonPressed) {
        case IDOK:
            return true;
        default:
            return false;
    }

}

// See: https://stackoverflow.com/questions/8046097/how-to-check-if-a-process-has-the-administrative-rights
bool IsElevated() {
    CHandle hToken;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken.m_h)) {
        TOKEN_ELEVATION Elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &Elevation, sizeof(Elevation), &cbSize)) {
            return Elevation.TokenIsElevated != 0;
        }
    }
    return false;
}

void LaunchInstaller() {
    auto url = GetString(IDS_INSTALLER_URL);

    ShellExecute(
            nullptr,
            L"open",
            url,
            nullptr,
            nullptr,
            SW_NORMAL
    );
}
