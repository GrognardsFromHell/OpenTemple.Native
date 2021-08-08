
#pragma once

#include <d2d1_1.h>
#include <dwrite_3.h>
#include <unordered_map>
#include "LRUCache11.h"

#include "FontFile.h"
#include "Style.h"
#include "TextFormatKey.h"
#include "TextLayout.h"
#include "TextRenderer.h"

class MemoryFontLoader;

class TextEngine {
 public:
  explicit TextEngine(const winrt::com_ptr<ID2D1RenderTarget> &renderTarget);
  ~TextEngine();

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

  void RenderBackgroundAndBorder(
      float x, float y,
      float width, float height,
      const BackgroundAndBorderStyle &style
  );

 private:
  winrt::com_ptr<IDWriteTextFormat2> GetTextFormat(const ParagraphStyle &paragraphStyle,
                                                   const TextStyle &textStyle) {
    return GetTextFormat(TextFormatKey(paragraphStyle, textStyle));
  }
  winrt::com_ptr<IDWriteTextFormat2> GetTextFormat(const TextFormatKey &key);

  winrt::com_ptr<ID2D1RenderTarget> _renderTarget;

  std::unique_ptr<TextRenderer> _textRenderer;

  winrt::com_ptr<IDWriteFactory1> _factory;

  winrt::com_ptr<IDWriteFontCollection> _fontCollection;

  winrt::com_ptr<MemoryFontLoader> _fontLoader;

  std::vector<FontFile> _fontFiles;

  // TextFormat cache
  lru11::Cache<TextFormatKey, winrt::com_ptr<IDWriteTextFormat2>> _textFormatCache;

  std::wstring _locale;

  lru11::Cache<uint32_t, winrt::com_ptr<ID2D1SolidColorBrush>> _colorBrushCache;
};
