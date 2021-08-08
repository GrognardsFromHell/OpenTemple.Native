
// clang-format off
#include "windows_headers.h"
#include <propvarutil.h>
#include <propkey.h>
// clang-format on

#include "../utils.h"

inline bool SetStringProp(IPropertyStore *link, REFPROPERTYKEY propKey, wchar_t *value) {
  PROPVARIANT propvar;
  if (!SUCCEEDED(InitPropVariantFromString(value, &propvar))) {
    return false;
  }

  auto hr = link->SetValue(propKey, propvar);
  PropVariantClear(&propvar);

  return SUCCEEDED(hr);
}

struct JumpListBuilder {
  int Init() noexcept {
    try {
      customList = winrt::create_instance<ICustomDestinationList>(CLSID_DestinationList);
    } catch (...) {
      return -1;
    }

    if (!SUCCEEDED(customList->BeginList(&minSlots, IID_PPV_ARGS(poaRemoved.put())))) {
      return -2;
    }

    try {
      tasks = winrt::create_instance<IObjectCollection>(CLSID_EnumerableObjectCollection);
    } catch (...) {
      return -3;
    }

    return 0;
  }

  int Commit() noexcept {
    auto poa = tasks.try_as<IObjectArray>();
    if (!poa) {
      return -1;
    }

    if (!SUCCEEDED(customList->AddUserTasks(poa.get()))) {
      return -2;
    }

    if (!SUCCEEDED(customList->CommitList())) {
      return -3;
    }

    return 0;
  }

  int AddTask(wchar_t *executable, wchar_t *arguments, wchar_t *title, wchar_t *iconPath,
              int iconIndex, wchar_t *description) noexcept {
    winrt::com_ptr<IShellLink> link;
    try {
      link = winrt::create_instance<IShellLink>(CLSID_ShellLink);
    } catch (...) {
      return -1;
    }

    if (!SUCCEEDED(link->SetPath(executable))) {
      return -2;
    }

    if (!SUCCEEDED(link->SetArguments(arguments))) {
      return -3;
    }

    // The title property is required on Jump List items provided as an IShellLink
    // instance.  This value is used as the display name in the Jump List.
    auto pps = link.try_as<IPropertyStore>();
    if (!pps) {
      return -4;
    }

    if (!SetStringProp(pps.get(), PKEY_Title, title)) {
      return -5;
    }

    if (!SUCCEEDED(pps->Commit())) {
      return -6;
    }

    if (!SUCCEEDED(link->SetIconLocation(iconPath, iconIndex))) {
      return -7;
    }

    if (description && !SUCCEEDED(link->SetDescription(description))) {
      return -8;
    }

    if (!SUCCEEDED(tasks->AddObject(pps.get()))) {
      return -9;
    }

    return 0;
  }

  UINT minSlots = 0;
  winrt::com_ptr<IObjectArray> poaRemoved;
  winrt::com_ptr<IObjectCollection> tasks;
  winrt::com_ptr<ICustomDestinationList> customList;
};

NATIVE_API JumpListBuilder *JumpListBuilder_Create() {
  return new JumpListBuilder;
}

NATIVE_API int JumpListBuilder_Init(JumpListBuilder *builder) {
  return builder->Init();
}

NATIVE_API int JumpListBuilder_Commit(JumpListBuilder *builder) {
  return builder->Commit();
}

NATIVE_API int JumpListBuilder_AddTask(JumpListBuilder *builder,
                                       wchar_t *arguments,
                                       wchar_t *title,
                                       wchar_t *iconPath,
                                       int iconIndex,
                                       wchar_t *description) {
  wchar_t moduleName[MAX_PATH];
  GetModuleFileName(nullptr, moduleName, ARRAYSIZE(moduleName));

  return builder->AddTask(moduleName, arguments, title, iconPath, iconIndex, description);
}

NATIVE_API void JumpListBuilder_Free(JumpListBuilder *builder) {
  delete builder;
}
