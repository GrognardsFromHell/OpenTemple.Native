
#include "MemoryFontStream.h"

HRESULT MemoryFontStream::ReadFileFragment(void const** fragmentStart, uint64_t fileOffset,
                                           uint64_t fragmentSize, void** fragmentContext) noexcept {
  if (fileOffset > _data.size() || fragmentSize > _data.size() - fileOffset) {
    *fragmentStart = nullptr;
    *fragmentContext = nullptr;
    return E_FAIL;
  }

  *fragmentContext = nullptr;
  *fragmentStart = _data.data() + fileOffset;
  return S_OK;
}

void MemoryFontStream::ReleaseFileFragment(void* fragmentContext) noexcept {
  // No concept of freeing the const memory in _data
}

HRESULT MemoryFontStream::GetFileSize(uint64_t* fileSize) noexcept {
  *fileSize = _data.size();
  return S_OK;
}

HRESULT MemoryFontStream::GetLastWriteTime(uint64_t* lastWriteTime) noexcept {
  return E_NOTIMPL;
}

HRESULT MemoryFontStream::QueryInterface(REFIID uuid, LPVOID* objOut) noexcept {
  if (uuid == IID_IUnknown || uuid == __uuidof(IDWriteFontFileStream)) {
    *objOut = this;
    AddRef();
    return S_OK;
  } else {
    *objOut = nullptr;
    return E_NOINTERFACE;
  }
}

ULONG MemoryFontStream::AddRef() noexcept {
  return InterlockedIncrement(&_refCount);
}

ULONG MemoryFontStream::Release() noexcept {
  ULONG refCount = InterlockedDecrement(&_refCount);
  if (!refCount) {
    delete this;
  }
  return refCount;
}
