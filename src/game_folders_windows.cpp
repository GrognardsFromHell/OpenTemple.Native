
#include "utils.h"

#include <string>

#include "com_helper.h"
#include "string_wrapper.h"

#include "windows_headers.h"

static CComPtr<IKnownFolderManager> GetFolderManager() {
  CComPtr<IKnownFolderManager> mgr;

  if (!mgr) {
    auto hr = CoCreateInstance(CLSID_KnownFolderManager, nullptr,
                               CLSCTX_INPROC_SERVER, IID_IKnownFolderManager,
                               reinterpret_cast<void **>(&mgr));

    if (!SUCCEEDED(hr)) {
      return nullptr;
    }
  }

  return mgr;
}

using WidePathArr = wchar_t[MAX_PATH];

static bool GetKnownFolder(const KNOWNFOLDERID &folderId,
                           WidePathArr &path) {
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
NATIVE_API ApiBool GameFolders_GetUserDataFolder(WideStringResult *result) {
  ComInitializer com;

  if (!com) {
    return false;
  }

  WidePathArr path;
  if (!GetKnownFolder(FOLDERID_Documents, path)) {
      return false;
  }

  if (wcscat_s(path, L"\\OpenTemple") != 0) {
      return false;
  }

  *result = new std::wstring(path);
  return true;

}
