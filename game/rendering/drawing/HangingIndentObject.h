
#pragma once

#include <winrt/base.h>
#include <dwrite_3.h>

/**
 * Inline object used to reserve negative space for a hanging indent.
 */
class HangingIndentObject : public IDWriteInlineObject {
 public:
  explicit HangingIndentObject(float indent) : _indent(indent) {}

  HRESULT STDMETHODCALLTYPE Draw(void *clientDrawingContext, IDWriteTextRenderer *renderer, FLOAT originX,
               FLOAT originY, BOOL isSideways, BOOL isRightToLeft,
               IUnknown *clientDrawingEffect) noexcept override {
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetMetrics(DWRITE_INLINE_OBJECT_METRICS *metrics) noexcept override {
    *metrics = {};
    metrics->height = 1;
    metrics->width = _indent;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetOverhangMetrics(DWRITE_OVERHANG_METRICS *overhangs) noexcept override {
    *overhangs = {};
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetBreakConditions(DWRITE_BREAK_CONDITION *breakConditionBefore,
                             DWRITE_BREAK_CONDITION *breakConditionAfter) noexcept override {
    *breakConditionBefore = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
    *breakConditionAfter = DWRITE_BREAK_CONDITION_MAY_NOT_BREAK;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid, void **ppvObject) noexcept override {
    if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWriteInlineObject)) {
      AddRef();
      *ppvObject = this;
      return S_OK;
    } else {
      *ppvObject = nullptr;
      return E_NOINTERFACE;
    }
  }

  ULONG STDMETHODCALLTYPE AddRef() noexcept override {
    return InterlockedIncrement(&_refCount);
  }

  ULONG STDMETHODCALLTYPE Release() noexcept override {
    ULONG refCount = InterlockedDecrement(&_refCount);
    if (!refCount) {
      delete this;
    }
    return refCount;
  }

 private:
  const float _indent = 0;
  uint32_t _refCount = 1;
};

