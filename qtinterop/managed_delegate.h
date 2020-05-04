
#pragma once

#include <functional>

extern void (*release_managed_delegate_handle)(void *);

template<typename T>
struct NativeDelegate {
    // The GC handle to keep the delegate on the C# side alive
    void *handle;
    // The callable function pointer generated by C# for the delegate
    // The signature of the function pointer generated by C# CANNOT be automatically 
    // type-checked and we essentially perform an unsafe cast.
    T *functor;

    ~NativeDelegate() {
        // If the GCHandle was not converted into a function pointer, we MUST
        // release it now, otherwise we'll leak the managed object.
        if (handle) {
            release_managed_delegate_handle(handle);
            handle = nullptr;
        }
    }

    /**
     * Automatic conversion to a std::function which internally keeps a shared_ptr reference
     * to the GCHandle. As soon as the std::function and all copies are destroyed, the GCHandle
     * is released.
     */
    operator std::function<T>() {
        auto functor = this->functor;  // This will be valid as long as the GCHandle isn't released
        if (!functor) {
            return {};
        }
        if (!handle) {
            // If there's no GC handle, but a functor exists, it must be a direct C# function pointer,
            // which is future stuff (https://docs.microsoft.com/en-us/dotnet/api/system.runtime.interopservices.unmanagedfunctionpointerattribute)
            return functor;
        }

        std::shared_ptr<void> shared_handle(handle, release_managed_delegate_handle);
        // We took ownership at this point, notify C# of it
        this->handle = nullptr;
        this->functor = nullptr;
        return [shared_handle, functor](auto &&... ts) {
            return functor(ts...);
        };
    }

    // Don't allow this to be constructed, this is for marshalling from C# only
    NativeDelegate() = delete;

    NativeDelegate(NativeDelegate &) = delete;

    NativeDelegate &operator=(NativeDelegate &) = delete;

    NativeDelegate(NativeDelegate &&) = delete;

    NativeDelegate &operator=(NativeDelegate &&) = delete;
};
