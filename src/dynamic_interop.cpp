
#include "utils.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

static HMODULE getDllHandle() {
    HMODULE hModule;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR) &getDllHandle, &hModule);
    return hModule;
}

#endif

NATIVE_API void *get_exported_function(const char *symbolName) {
#ifdef _WIN32
    static HMODULE library = getDllHandle();
    FARPROC symbol = GetProcAddress(library, symbolName);
    return (void *) symbol;
#else
    void* dll = dlopen(nativeModule.isNull() || nativeModule.isEmpty() ? nullptr : nativeModule.toLocal8Bit(),
                       RTLD_LAZY);
    void* result = dlsym(dll, symbolName);
    dlclose(dll);
    return result;
#endif
}
