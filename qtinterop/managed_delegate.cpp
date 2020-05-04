
#include "managed_delegate.h"
#include "../game/utils.h"

struct ManagedDelegateCallbacks {
    void (*release_handle)(void *);
};

void (*release_managed_delegate_handle)(void *) = nullptr;

NATIVE_API void managed_delegate_set_callbacks(const ManagedDelegateCallbacks &callbacks) {
    release_managed_delegate_handle = callbacks.release_handle;
}
