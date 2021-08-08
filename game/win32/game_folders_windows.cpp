
#include "../utils.h"

#include "windows_headers.h"
#include <string>

#include "com_helper.h"

#include "../interop/string_interop.h"
#include "../logging/Logger.h"

static winrt::com_ptr<IKnownFolderManager> GetFolderManager() noexcept {
  try {
    return winrt::create_instance<IKnownFolderManager>(CLSID_KnownFolderManager);
  } catch (winrt::hresult_error &e) {
    Logger::Error(std::wstring(L"Failed to create IKnownFolderManager: ") + e.message());
    return nullptr;
  } catch (std::exception &e) {
    Logger::Error(std::wstring(L"Failed to create IKnownFolderManager: ") + localToWide(e.what()));
    return nullptr;
  }
}

using WidePathArr = wchar_t[MAX_PATH];

static bool GetKnownFolder(const KNOWNFOLDERID &folderId, WidePathArr &path) {
  auto mgr = GetFolderManager();
  if (!mgr) {
    return false;
  }

  winrt::com_ptr<IKnownFolder> folder;
  if (!SUCCEEDED(mgr->GetFolder(folderId, folder.put()))) {
    Logger::Error(L"Failed to get known folder.");
    return false;
  }

  LPWSTR folderPath;
  if (!SUCCEEDED(folder->GetPath(KF_FLAG_CREATE | KF_FLAG_INIT, &folderPath))) {
    Logger::Error(L"Failed to get path of known folder.");
    return false;
  }

  auto copyResult = wcscpy_s(path, folderPath);
  CoTaskMemFree(folderPath);
  return copyResult == 0;
}

// Get the system's preferred folder for storing the user's save games
NATIVE_API char16_t * GameFolders_GetUserDataFolder() {
  ComInitializer com;

  if (!com) {
    Logger::Error(L"Failed to initialize COM.");
    return nullptr;
  }

  WidePathArr path;
  if (!GetKnownFolder(FOLDERID_Documents, path)) {
    return nullptr;
  }

  if (wcscat_s(path, L"\\OpenTemple") != 0) {
    return nullptr;
  }

  return copyString(path);
}
