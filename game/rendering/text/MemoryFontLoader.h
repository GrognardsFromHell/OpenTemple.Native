
#pragma once

#include <dwrite.h>
#include <winrt/base.h>
#include <vector>

struct FontFile;

class MemoryFontLoader : public IDWriteFontCollectionLoader,
                         public IDWriteFontFileEnumerator,
                         public IDWriteFontFileLoader {
 public:
  MemoryFontLoader(winrt::com_ptr<IDWriteFactory> factory,
      const std::vector<FontFile>& fontFiles) :
        _factory(std::move(factory)),
        _fontFiles(fontFiles) {}

  HRESULT STDMETHODCALLTYPE
  CreateEnumeratorFromKey(IDWriteFactory* factory,
                          void const* collectionKey,
                          UINT32 collectionKeySize,
                          IDWriteFontFileEnumerator** fontFileEnumerator) noexcept override;

  HRESULT STDMETHODCALLTYPE MoveNext(BOOL* hasCurrentFile) noexcept override;

  HRESULT STDMETHODCALLTYPE GetCurrentFontFile(IDWriteFontFile** currentFontFile) noexcept override;

  HRESULT STDMETHODCALLTYPE
  CreateStreamFromKey(void const* fontFileReferenceKey,
                      UINT32 fontFileReferenceKeySize,
                      IDWriteFontFileStream** fontFileStream) noexcept override;

  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID uuid, void** object) noexcept override;

  ULONG STDMETHODCALLTYPE AddRef() noexcept override;

  ULONG STDMETHODCALLTYPE Release() noexcept override;

 private:
  winrt::com_ptr<IDWriteFactory> _factory;
  const std::vector<FontFile>& _fontFiles;

  size_t _streamIndex = 0;
  winrt::com_ptr<IDWriteFontFile> _currentFontFile;

  uint32_t _refCount = 1;
};
