
#include "windows_headers.h"
#include "utils.h"
#include "propvarutil.h"
#include <propkey.h>

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

    int Init() {
        if (!SUCCEEDED(customList.CoCreateInstance(CLSID_DestinationList))) {
            return -1;
        }

        if (!SUCCEEDED(customList->BeginList(&minSlots, IID_PPV_ARGS(&poaRemoved)))) {
            return -2;
        }

        if (!SUCCEEDED(tasks.CoCreateInstance(CLSID_EnumerableObjectCollection))) {
            return -3;
        }

        return 0;
    }

    int Commit() {
        CComPtr<IObjectArray> poa;
        if (!SUCCEEDED(tasks->QueryInterface(IID_PPV_ARGS(&poa)))) {
            return -1;
        }

        if (!SUCCEEDED(customList->AddUserTasks(poa))) {
            return -2;
        }

        if (!SUCCEEDED(customList->CommitList())) {
            return -3;
        }

        return 0;
    }

    int AddTask(wchar_t *executable, wchar_t *arguments, wchar_t *title, wchar_t *description) {
        CComPtr<IShellLink> link;
        if (!SUCCEEDED(link.CoCreateInstance(CLSID_ShellLink))) {
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
        CComPtr<IPropertyStore> pps;
        if (!SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&pps)))) {
            return -4;
        }

        if (!SetStringProp(pps, PKEY_Title, title)) {
            return -5;
        }

        if (description && !SetStringProp(pps, PKEY_Link_Description, description)) {
            return -6;
        }

        if (!SUCCEEDED(pps->Commit())) {
            return -7;
        }

        if (!SUCCEEDED(tasks->AddObject(pps))) {
            return -8;
        }

        return 0;
    }

    UINT minSlots = 0;
    CComPtr<IObjectArray> poaRemoved;
    CComPtr<IObjectCollection> tasks;
    CComPtr<ICustomDestinationList> customList;
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
                                       wchar_t *executable,
                                       wchar_t *arguments,
                                       wchar_t *title,
                                       wchar_t *description
) {
    return builder->AddTask(executable, arguments, title, description);
}

NATIVE_API void JumpListBuilder_Free(JumpListBuilder *builder) {
    delete builder;
}
