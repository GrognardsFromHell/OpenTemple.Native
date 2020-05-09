
#pragma once

#include <functional>
#include <QString>

template<typename T>
class CompletionSource {
public:
    CompletionSource(void *handle,
                     const std::function<void(void *, void*, int)> &success,
                     const std::function<void(void *, const char16_t *)> &error,
                     const std::function<void(void *)> &cancel) : _handle(handle),
                                                                  _success(success),
                                                                  _error(error),
                                                                  _cancel(cancel) {
    }

    CompletionSource(CompletionSource &) = delete;

    CompletionSource &operator=(CompletionSource &) = delete;

    void succeed(T result) {
        if (_handle) {
            _success(_handle, &result, (int) sizeof(T));
            _handle = nullptr;
        }
    }

    void fail(const char16_t *message) {
        if (_handle) {
            _error(_handle, message);
            _handle = nullptr;
        }
    }

    void fail(const QString &message) {
        fail(reinterpret_cast<const char16_t *>(message.utf16()));
    }

    void cancel() {
        if (_handle) {
            _cancel(_handle);
            _handle = nullptr;
        }
    }

private:
    void *_handle = nullptr;
    const std::function<void(void *, void*, int)> _success = nullptr;
    const std::function<void(void *, const char16_t *error)> _error = nullptr;
    const std::function<void(void *)> _cancel = nullptr;
};

using QObjectCompletionSource = CompletionSource<void*>;

#include <QMetaType>

Q_DECLARE_METATYPE(QObjectCompletionSource*);
