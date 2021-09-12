
#pragma once

#include <d2d1_1.h>
#include <d2d1_2.h>
#include <d3d11.h>
#include <dwrite_3.h>
#include <unordered_map>
#include "LRUCache11.h"

#include "FontFile.h"
#include "Style.h"
#include "TextFormatKey.h"
#include "TextLayout.h"
#include "TextRenderer.h"

class MemoryFontLoader;

class DrawingEngine {
 public:
  explicit DrawingEngine(const winrt::com_ptr<ID3D11Device> &d3dDevice, bool debugDevice);
  ~DrawingEngine();

  void BeginDraw() noexcept;
  void EndDraw();

  void PushClipRect(float left, float top, float right, float bottom, bool antiAliased) noexcept;
  void PopClipRect() noexcept;

  void SetTransform(const D2D1_MATRIX_3X2_F &matrix) noexcept;
  void GetTransform(D2D1_MATRIX_3X2_F *matrix) noexcept;

  TextLayout *CreateTextLayout(const ParagraphStyle &paragraphStyle,
                               const TextStyle &textStyle,
                               const wchar_t *text,
                               uint32_t textLength,
                               float maxWidth,
                               float maxHeight);

  const std::vector<FontFile> &GetFontFiles() const {
    return _fontFiles;
  }

  void AddFontFile(FontFile fontFile) {
    _fontFiles.emplace_back(std::move(fontFile));
  }

  void ReloadFontFamilies();

  int GetFontFamiliesCount() const;
  std::wstring GetFontFamilyName(int familyIndex) const;

  winrt::com_ptr<ID2D1SolidColorBrush> GetBrush(uint32_t color);

  winrt::com_ptr<TextRendererStyle> CreateTextRendererStyle(const TextStyle &style);

  void RenderTextLayout(float x, float y, TextLayout &textLayout, float opacity);

  void RenderBackgroundAndBorder(float x, float y, float width, float height,
                                 const BackgroundAndBorderStyle &style);

  void SetRenderTarget(ID3D11Texture2D *renderTarget);

  /**
   * Sets the size of the "virtual canvas" that will be the space in which coordinates
   * passed to the drawing engine are interpreted. This space is projected onto the
   * render target by scaling the coordinates.
   */
  void SetCanvasSize(float width, float height) noexcept;
  void GetCanvasScale(float *width, float *height) noexcept;

 private:
  winrt::com_ptr<IDWriteTextFormat2> GetTextFormat(const ParagraphStyle &paragraphStyle,
                                                   const TextStyle &textStyle) {
    return GetTextFormat(TextFormatKey(paragraphStyle, textStyle));
  }
  winrt::com_ptr<IDWriteTextFormat2> GetTextFormat(const TextFormatKey &key);

  winrt::com_ptr<ID2D1DeviceContext> _context;

  std::unique_ptr<TextRenderer> _textRenderer;

  winrt::com_ptr<IDWriteFactory1> _textFactory;

  winrt::com_ptr<IDWriteFontCollection> _fontCollection;

  winrt::com_ptr<MemoryFontLoader> _fontLoader;

  std::vector<FontFile> _fontFiles;

  // TextFormat cache
  lru11::Cache<TextFormatKey, winrt::com_ptr<IDWriteTextFormat2>> _textFormatCache;

  std::wstring _locale;

  lru11::Cache<uint32_t, winrt::com_ptr<ID2D1SolidColorBrush>> _colorBrushCache;

  winrt::com_ptr<ID2D1Factory> _factory;

};
