
#pragma once

#include <Unknwn.h>
#include <winrt/base.h>
#include <d2d1.h>
#include "Style.h"

class __declspec(uuid("{4e27f114-78ee-410c-8a9b-4f641654214c}")) TextRendererStyle
    : public IUnknown {
 public:
  TextRendererStyle(const winrt::com_ptr<ID2D1Brush> &color,
                    const winrt::com_ptr<ID2D1Brush> &dropShadowColor,
                    const winrt::com_ptr<ID2D1Brush> &outlineColor,
                    const float outlineWidth)
      : _color(color),
        _dropShadowColor(dropShadowColor),
        _outlineColor(outlineColor),
        _outlineWidth(outlineWidth) {}

  [[nodiscard]] const winrt::com_ptr<ID2D1Brush> &GetColor() const {
    return _color;
  }
  [[nodiscard]] const winrt::com_ptr<ID2D1Brush> &GetDropShadowColor() const {
    return _dropShadowColor;
  }
  [[nodiscard]] const winrt::com_ptr<ID2D1Brush> &GetOutlineColor() const {
    return _outlineColor;
  }
  [[nodiscard]] float GetOutlineWidth() const {
    return _outlineWidth;
  }
  [[nodiscard]] bool HasOutline() const {
    return _outlineColor && _outlineWidth > 0;
  }

  HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid, void **ppvObject) noexcept override;
  ULONG STDMETHODCALLTYPE AddRef() noexcept override;
  ULONG STDMETHODCALLTYPE Release() noexcept override;

 private:
  uint32_t _refCount = 1;
  const winrt::com_ptr<ID2D1Brush> _color;
  const winrt::com_ptr<ID2D1Brush> _dropShadowColor;
  const winrt::com_ptr<ID2D1Brush> _outlineColor;
  const float _outlineWidth;
};
