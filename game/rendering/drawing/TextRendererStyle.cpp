
#include "TextRendererStyle.h"

HRESULT TextRendererStyle::QueryInterface(const IID &riid, void **ppvObject) noexcept {
  if (riid == __uuidof(IUnknown) || riid == __uuidof(TextRendererStyle)) {
    AddRef();
    *ppvObject = this;
    return S_OK;
  } else {
    *ppvObject = nullptr;
    return E_NOINTERFACE;
  }
}

ULONG TextRendererStyle::AddRef() noexcept {
  return InterlockedIncrement(&_refCount);
}

ULONG TextRendererStyle::Release() noexcept {
  ULONG refCount = InterlockedDecrement(&_refCount);
  if (!refCount) {
    delete this;
  }
  return refCount;
}
