
#include "completion_source.h"
#include "utils.h"

// The storage allocated for CompletionSource should be independent of the underlying type
using OpaqueType = void *;
using SuccessCallback = void(void *, void*, int);
using ErrorCallback = void(void *, const char16_t *);
using CancelCallback = void(void *);

NATIVE_API CompletionSource<OpaqueType> *completion_source_create(
        void *handle,
        SuccessCallback *successCallback,
        ErrorCallback *errorCallback,
        CancelCallback *cancelCallback
) {
    return new CompletionSource<OpaqueType>(
            handle,
            successCallback,
            errorCallback,
            cancelCallback
    );
}
