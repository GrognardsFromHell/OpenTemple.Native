
#pragma once

#include <atomic>
#include <vector>
#include <dwrite.h>

#include "FontFile.h"

class MemoryFontStream : public IDWriteFontFileStream {
 public:
  explicit MemoryFontStream(const std::vector<uint8_t>& data) : _data(data) {}

  HRESULT __stdcall ReadFileFragment(void const** fragmentStart,
                                     uint64_t fileOffset,
                                     uint64_t fragmentSize,
                                     void** fragmentContext) noexcept override;

  void __stdcall ReleaseFileFragment(void* fragmentContext) noexcept override;

  HRESULT __stdcall GetFileSize(uint64_t* fileSize) noexcept override;

  HRESULT __stdcall GetLastWriteTime(uint64_t* lastWriteTime) noexcept override;

  // IUnknown implementation
  HRESULT __stdcall QueryInterface(REFIID uuid, LPVOID* objOut) noexcept override;
  ULONG __stdcall AddRef() noexcept override;
  ULONG __stdcall Release() noexcept override;

 private:
  const std::vector<uint8_t>& _data;
  uint32_t _refCount = 1;
};
