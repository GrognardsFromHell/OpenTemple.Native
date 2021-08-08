
#include "TextEngine.h"
#include "HangingIndentObject.h"
#include "MemoryFontLoader.h"
#include "StyleConversion.h"

TextEngine::TextEngine(const winrt::com_ptr<ID2D1RenderTarget>& renderTarget)
    : _renderTarget(renderTarget), _textFormatCache(500), _colorBrushCache(256) {
  winrt::check_hresult(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                           __uuidof(IDWriteFactory),
                                           reinterpret_cast<IUnknown**>(_factory.put_void())));

  // Create our custom font loader
  _fontLoader.attach(new MemoryFontLoader(_factory, _fontFiles));

  winrt::check_hresult(_factory->RegisterFontCollectionLoader(_fontLoader.get()));
  winrt::check_hresult(_factory->RegisterFontFileLoader(_fontLoader.get()));

  _textRenderer = std::make_unique<TextRenderer>(_factory, renderTarget);
}

TextEngine::~TextEngine() = default;

winrt::com_ptr<IDWriteTextFormat2> TextEngine::GetTextFormat(const TextFormatKey& key) {
  winrt::com_ptr<IDWriteTextFormat2> result;
  if (_textFormatCache.tryGet(key, result)) {
    return std::move(result);
  }

  winrt::com_ptr<IDWriteTextFormat> format;
  winrt::check_hresult(_factory->CreateTextFormat(key.FontFamilyName.c_str(),
                                                  _fontCollection.get(),
                                                  ConvertFontWeight(key.FontWeight),
                                                  ConvertFontStyle(key.FontStyle),
                                                  ConvertFontStretch(key.FontStretch),
                                                  key.FontSize,
                                                  _locale.c_str(),
                                                  format.put()));

  result = format.as<IDWriteTextFormat2>();

  if (key.TabStopWidth > 0) {
    result->SetIncrementalTabStop(key.TabStopWidth);
  }

  result->SetWordWrapping(ConvertWordWrap(key.WordWrap));
  result->SetTextAlignment(ConvertTextAlign(key.TextAlignment));
  result->SetParagraphAlignment(ConvertParagraphAlignment(key.ParagraphAlignment));
  if (key.TrimMode != TrimMode::None) {
    winrt::com_ptr<IDWriteInlineObject> trimmingSign;
    DWRITE_TRIMMING trimmingOptions{};
    switch (key.TrimMode) {
      default:
      case TrimMode::Character:
        trimmingOptions.granularity = DWRITE_TRIMMING_GRANULARITY_CHARACTER;
        break;
      case TrimMode::Word:
        trimmingOptions.granularity = DWRITE_TRIMMING_GRANULARITY_WORD;
        break;
    }

    if (key.TrimmingSign == TrimmingSign::Ellipsis) {
      _factory->CreateEllipsisTrimmingSign(result.get(), trimmingSign.put());
    }

    result->SetTrimming(&trimmingOptions, trimmingSign.get());
  }

  _textFormatCache.insert(key, result);

  return std::move(result);
}

winrt::com_ptr<ID2D1SolidColorBrush> TextEngine::GetBrush(uint32_t color) {
  winrt::com_ptr<ID2D1SolidColorBrush> result;
  if (!_colorBrushCache.tryGet(color, result)) {
    D2D1_COLOR_F d2dColor = ConvertColor(color);

    winrt::check_hresult(_renderTarget->CreateSolidColorBrush(d2dColor, result.put()));
    _colorBrushCache.insert(color, result);
  }
  return std::move(result);
}

void TextEngine::ReloadFontFamilies() {
  // When font families change, text formats must also be re-evaluated
  _textFormatCache.clear();

  // DirectWrite caches font collections internally.
  // If we reload, we need to generate a new key
  static uint32_t fontCollKey = 1;

  _fontCollection = nullptr;
  winrt::check_hresult(_factory->CreateCustomFontCollection(
      _fontLoader.get(), &fontCollKey, sizeof(fontCollKey), _fontCollection.put()));
  fontCollKey++;
}

int TextEngine::GetFontFamiliesCount() const {
  return _fontCollection ? (int)_fontCollection->GetFontFamilyCount() : 0;
}

std::wstring TextEngine::GetFontFamilyName(int familyIndex) const {
  if (familyIndex < 0 || familyIndex >= GetFontFamiliesCount()) {
    return L"<OUT_OF_RANGE>";
  }

  winrt::com_ptr<IDWriteFontFamily> family;
  winrt::check_hresult(_fontCollection->GetFontFamily(familyIndex, family.put()));

  winrt::com_ptr<IDWriteLocalizedStrings> familyNames;
  winrt::check_hresult(family->GetFamilyNames(familyNames.put()));

  if (familyNames->GetCount() < 1) {
    return L"<NO_NAME>";
  }

  // Simply return the first localized name since this is for debugging purposes only
  std::wstring nameBuffer;
  uint32_t nameLength;
  winrt::check_hresult(familyNames->GetStringLength(0, &nameLength));
  nameBuffer.resize(nameLength + 1);
  winrt::check_hresult(familyNames->GetString(0, &nameBuffer[0], nameLength + 1));
  nameBuffer.resize(nameLength);  // Remove the null-byte
  return nameBuffer;
}

TextLayout* TextEngine::CreateTextLayout(const ParagraphStyle& paragraphStyle,
                                         const TextStyle& textStyle,
                                         const wchar_t* text,
                                         uint32_t textLength,
                                         float maxWidth,
                                         float maxHeight) {
  auto rootFormat = GetTextFormat(paragraphStyle, textStyle);

  // A somewhat annoying hack to insert a negative-width inline object at the start to simulate
  // a hanging indent.
  std::unique_ptr<wchar_t[]> adjustedText;
  auto actualText = text;
  auto actualTextLength = textLength;
  if (paragraphStyle.HangingIndent) {
    actualTextLength = textLength + 1;
    adjustedText = std::make_unique<wchar_t[]>(actualTextLength);
    adjustedText[0] = L' ';
    wcsncpy(&adjustedText[1], text, textLength);
    actualText = adjustedText.get();

    maxWidth -= paragraphStyle.Indent;
  }

  winrt::com_ptr<IDWriteTextLayout> textLayout;
  winrt::check_hresult(_factory->CreateTextLayout(actualText, actualTextLength, rootFormat.get(),
                                                  maxWidth, maxHeight, textLayout.put()));

  if (paragraphStyle.HangingIndent) {
    winrt::com_ptr<IDWriteInlineObject> hangingIndent;
    hangingIndent.attach(new HangingIndentObject(-paragraphStyle.Indent));
    textLayout->SetInlineObject(hangingIndent.get(), {0, 1});
  }

  auto defaultStyle = CreateTextRendererStyle(textStyle);
  auto result = new TextLayout(*this, textLayout, paragraphStyle.HangingIndent, paragraphStyle.Indent,
                        std::move(*defaultStyle));

  // Some of the default text style needs to be applied to the whole range of the text layout
  // since it is not part of the DWrite text format.
  // To know which ones, check TextFormatKey to find out which parts of TextStyle are actually
  // made part of the text format. We need to apply the remaining properties here, unless
  // they're handled as rendering parameters below.
  result->SetStyle(
      0,
      textLength,
      TextStyleProperty::LineThrough | TextStyleProperty::Underline | TextStyleProperty::Kerning,
      textStyle);

  return result;
}

winrt::com_ptr<TextRendererStyle> TextEngine::CreateTextRendererStyle(const TextStyle& style) {
  // Convert the colors into strong-references to D2D1 brushes.
  auto color = GetBrush(style.Color).as<ID2D1Brush>();
  winrt::com_ptr<ID2D1Brush> dropShadowColor;
  if (!IsTransparent(style.DropShadowColor)) {
    dropShadowColor = GetBrush(style.DropShadowColor).as<ID2D1Brush>();
  }
  winrt::com_ptr<ID2D1Brush> outlineColor;
  if (!IsTransparent(style.OutlineColor)) {
    outlineColor = GetBrush(style.OutlineColor).as<ID2D1Brush>();
  }

  winrt::com_ptr<TextRendererStyle> renderStyle;
  renderStyle.attach(
      new TextRendererStyle(color, dropShadowColor, outlineColor, style.OutlineWidth));
  return renderStyle;
}

void TextEngine::RenderTextLayout(float x, float y, TextLayout& textLayout, float opacity) {
  textLayout.Render(*_textRenderer, x, y, opacity);
}

void TextEngine::RenderBackgroundAndBorder(float x, float y, float width, float height,
                                           const BackgroundAndBorderStyle &style) {
  auto rect = D2D1::RectF(x, y, x + width, y + height);

  // Rounded corners need to be drawn differently
  if (style.radiusX > 0 || style.radiusY > 0) {
    auto roundedRect = D2D1::RoundedRect(rect, style.radiusX, style.radiusY);

    if (!IsTransparent(style.backgroundColor)) {
      auto fillBrush = GetBrush(style.backgroundColor);
      _renderTarget->FillRoundedRectangle(&roundedRect, fillBrush.get());
    }
    if (!IsTransparent(style.borderColor)) {
      auto borderBrush = GetBrush(style.borderColor);
      _renderTarget->DrawRoundedRectangle(&roundedRect, borderBrush.get(), style.borderWidth);
    }
  } else {
    if (!IsTransparent(style.backgroundColor)) {
      auto fillBrush = GetBrush(style.backgroundColor);
      _renderTarget->FillRectangle(&rect, fillBrush.get());
    }
    if (!IsTransparent(style.borderColor)) {
      auto borderBrush = GetBrush(style.borderColor);
      _renderTarget->DrawRectangle(&rect, borderBrush.get(), style.borderWidth);
    }
  }
}
