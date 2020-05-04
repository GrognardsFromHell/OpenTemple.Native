
#include "windows_headers.h"

class ComInitializer {
 public:
  ComInitializer() {
    auto err =
        CoInitializeEx(nullptr, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE);
    // S_FALSE means COM was already initialized, but docs say
    // CoUninitialize should still be called.
    _initialized = (err == S_OK || err == S_FALSE);
  }

  explicit operator bool() const { return _initialized; }

  ~ComInitializer() {
    if (_initialized) {
      CoUninitialize();
    }
  }

 private:
  bool _initialized = false;
};
