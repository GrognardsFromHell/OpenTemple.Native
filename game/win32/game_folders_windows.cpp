
#include "../utils.h"

#include <string>

#include "com_helper.h"

#include "../interop/string_interop.h"
#include "windows_headers.h"

static CComPtr<IKnownFolderManager> GetFolderManager() {
  CComPtr<IKnownFolderManager> mgr;

  if (!SUCCEEDED(mgr.CoCreateInstance(CLSID_KnownFolderManager))) {
    return nullptr;
  }

  return mgr;
}

using WidePathArr = wchar_t[MAX_PATH];

static bool GetKnownFolder(const KNOWNFOLDERID &folderId, WidePathArr &path) {
  auto mgr = GetFolderManager();
  if (!mgr) {
    return false;
  }

  CComPtr<IKnownFolder> folder;
  if (!SUCCEEDED(mgr->GetFolder(folderId, &folder))) {
    return false;
  }

  LPWSTR folderPath;
  if (!SUCCEEDED(folder->GetPath(KF_FLAG_CREATE | KF_FLAG_INIT, &folderPath))) {
    return false;
  }

  auto copyResult = wcscpy_s(path, folderPath);
  CoTaskMemFree(folderPath);
  return copyResult == 0;
}

// Get the system's preferred folder for storing the user's save games
NATIVE_API wchar_t * GameFolders_GetUserDataFolder() {
  ComInitializer com;

  if (!com) {
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
