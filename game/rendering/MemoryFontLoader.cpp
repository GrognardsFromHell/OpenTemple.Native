
#include "MemoryFontLoader.h"
#include "FontFile.h"
#include "MemoryFontStream.h"

HRESULT MemoryFontLoader::CreateEnumeratorFromKey(
    IDWriteFactory *factory,
    void const *collectionKey,
    UINT32 collectionKeySize,
    IDWriteFontFileEnumerator **fontFileEnumerator) noexcept {
  *fontFileEnumerator = this;
  _streamIndex = 0;
  return S_OK;
}

HRESULT MemoryFontLoader::MoveNext(BOOL *hasCurrentFile) noexcept {
  *hasCurrentFile = FALSE;

  if (_streamIndex < _fontFiles.size()) {
    _currentFontFile = nullptr;
    winrt::check_hresult(_factory->CreateCustomFontFileReference(
        &_streamIndex, sizeof(_streamIndex), this, _currentFontFile.put()));

    *hasCurrentFile = TRUE;
    ++_streamIndex;
  }

  return S_OK;
}

HRESULT MemoryFontLoader::GetCurrentFontFile(IDWriteFontFile **currentFontFile) noexcept {
  _currentFontFile.copy_to(currentFontFile);
  return S_OK;
}

HRESULT MemoryFontLoader::CreateStreamFromKey(void const *fontFileReferenceKey,
                                              uint32_t fontFileReferenceKeySize,
                                              IDWriteFontFileStream **fontFileStream) noexcept {
  assert(fontFileReferenceKeySize == sizeof(size_t));
  size_t index = *(size_t *)fontFileReferenceKey;

  auto &font = _fontFiles[index];
  *fontFileStream = new MemoryFontStream(font.data);
  return S_OK;
}

HRESULT MemoryFontLoader::QueryInterface(REFIID uuid, void **object) noexcept {
  if (uuid == IID_IUnknown || uuid == __uuidof(IDWriteFontCollectionLoader) ||
      uuid == __uuidof(IDWriteFontFileEnumerator) || uuid == __uuidof(IDWriteFontFileLoader)) {
    *object = this;
    return S_OK;
  } else {
    *object = nullptr;
    return E_NOINTERFACE;
  }
}

ULONG MemoryFontLoader::AddRef() noexcept {
  return InterlockedIncrement(&_refCount);
}

ULONG MemoryFontLoader::Release() noexcept {
  ULONG refCount = InterlockedDecrement(&_refCount);
  if (!refCount) {
    delete this;
  }
  return refCount;
}
