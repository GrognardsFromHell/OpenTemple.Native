
#include "DrawingEngine.h"
#include "../../logging/Logger.h"
#include "HangingIndentObject.h"
#include "MemoryFontLoader.h"
#include "StyleConversion.h"

DrawingEngine::DrawingEngine(const winrt::com_ptr<ID3D11Device>& d3dDevice, bool debugDevice)
    : _textFormatCache(500), _colorBrushCache(256) {
  // Create the D2D factory
  D2D1_FACTORY_OPTIONS factoryOptions{};
  if (debugDevice) {
    factoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;

    Logger::Info(L"Creating Direct2D Factory (debug=true).");
  } else {
    factoryOptions.debugLevel = D2D1_DEBUG_LEVEL_NONE;
    Logger::Info(L"Creating Direct2D Factory (debug=false).");
  }

  winrt::check_hresult(
      D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, factoryOptions, _factory.put()));

  auto dxgiDevice = d3dDevice.as<IDXGIDevice>();

  // Create a D2D device on top of the DXGI device
  winrt::com_ptr<ID2D1Device> d2dDevice;
  winrt::check_hresult(D2D1CreateDevice(dxgiDevice.get(), nullptr, d2dDevice.put()));

  // Get Direct2D device's corresponding device context object.
  winrt::check_hresult(
      d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, _context.put()));

  _context->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

  winrt::check_hresult(DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED,
                                           __uuidof(IDWriteFactory),
                                           reinterpret_cast<IUnknown**>(_textFactory.put_void())));

  // Create our custom font loader
  _fontLoader.attach(new MemoryFontLoader(_textFactory, _fontFiles));

  winrt::check_hresult(_textFactory->RegisterFontCollectionLoader(_fontLoader.get()));
  winrt::check_hresult(_textFactory->RegisterFontFileLoader(_fontLoader.get()));

  _textRenderer = std::make_unique<TextRenderer>(_textFactory, _context);
}

DrawingEngine::~DrawingEngine() = default;

winrt::com_ptr<IDWriteTextFormat2> DrawingEngine::GetTextFormat(const TextFormatKey& key) {
  winrt::com_ptr<IDWriteTextFormat2> result;
  if (_textFormatCache.tryGet(key, result)) {
    return std::move(result);
  }

  winrt::com_ptr<IDWriteTextFormat> format;
  winrt::check_hresult(_textFactory->CreateTextFormat(key.FontFamilyName.c_str(),
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
      _textFactory->CreateEllipsisTrimmingSign(result.get(), trimmingSign.put());
    }

    result->SetTrimming(&trimmingOptions, trimmingSign.get());
  }

  _textFormatCache.insert(key, result);

  return std::move(result);
}

winrt::com_ptr<ID2D1SolidColorBrush> DrawingEngine::GetBrush(uint32_t color) {
  winrt::com_ptr<ID2D1SolidColorBrush> result;
  if (!_colorBrushCache.tryGet(color, result)) {
    D2D1_COLOR_F d2dColor = ConvertColor(color);

    winrt::check_hresult(_context->CreateSolidColorBrush(d2dColor, result.put()));
    _colorBrushCache.insert(color, result);
  }
  return std::move(result);
}

void DrawingEngine::ReloadFontFamilies() {
  // When font families change, text formats must also be re-evaluated
  _textFormatCache.clear();

  // DirectWrite caches font collections internally.
  // If we reload, we need to generate a new key
  static uint32_t fontCollKey = 1;

  _fontCollection = nullptr;
  winrt::check_hresult(_textFactory->CreateCustomFontCollection(
      _fontLoader.get(), &fontCollKey, sizeof(fontCollKey), _fontCollection.put()));
  fontCollKey++;
}

int DrawingEngine::GetFontFamiliesCount() const {
  return _fontCollection ? (int)_fontCollection->GetFontFamilyCount() : 0;
}

std::wstring DrawingEngine::GetFontFamilyName(int familyIndex) const {
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

TextLayout* DrawingEngine::CreateTextLayout(const ParagraphStyle& paragraphStyle,
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
  winrt::check_hresult(_textFactory->CreateTextLayout(
      actualText, actualTextLength, rootFormat.get(), maxWidth, maxHeight, textLayout.put()));

  if (paragraphStyle.HangingIndent) {
    winrt::com_ptr<IDWriteInlineObject> hangingIndent;
    hangingIndent.attach(new HangingIndentObject(-paragraphStyle.Indent));
    textLayout->SetInlineObject(hangingIndent.get(), {0, 1});
  }

  auto defaultStyle = CreateTextRendererStyle(textStyle);
  auto result = new TextLayout(*this, textLayout, paragraphStyle.HangingIndent,
                               paragraphStyle.Indent, std::move(*defaultStyle));

  // Some default text styles needs to be applied to the whole range of the text layout
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

winrt::com_ptr<TextRendererStyle> DrawingEngine::CreateTextRendererStyle(const TextStyle& style) {
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

void DrawingEngine::RenderTextLayout(float x, float y, TextLayout& textLayout, float opacity) {
  textLayout.Render(*_textRenderer, x, y, opacity);
}

void DrawingEngine::RenderBackgroundAndBorder(float x, float y, float width, float height,
                                              const BackgroundAndBorderStyle& style) {
  auto rect = D2D1::RectF(x, y, x + width, y + height);

  // Rounded corners need to be drawn differently
  if (style.radiusX > 0 || style.radiusY > 0) {
    auto roundedRect = D2D1::RoundedRect(rect, style.radiusX, style.radiusY);

    if (!IsTransparent(style.backgroundColor)) {
      auto fillBrush = GetBrush(style.backgroundColor);
      _context->FillRoundedRectangle(&roundedRect, fillBrush.get());
    }
    if (!IsTransparent(style.borderColor)) {
      auto borderBrush = GetBrush(style.borderColor);
      _context->DrawRoundedRectangle(&roundedRect, borderBrush.get(), style.borderWidth);
    }
  } else {
    if (!IsTransparent(style.backgroundColor)) {
      auto fillBrush = GetBrush(style.backgroundColor);
      _context->FillRectangle(&rect, fillBrush.get());
    }
    if (!IsTransparent(style.borderColor)) {
      auto borderBrush = GetBrush(style.borderColor);
      _context->DrawRectangle(&rect, borderBrush.get(), style.borderWidth);
    }
  }
}

void DrawingEngine::BeginDraw() noexcept {
  _context->BeginDraw();
}

void DrawingEngine::EndDraw() {
  winrt::check_hresult(_context->EndDraw());
}

void DrawingEngine::PushClipRect(float left, float top, float right, float bottom,
                                 bool antiAliased) noexcept {
  D2D1_RECT_F rect{};
  rect.left = left;
  rect.top = top;
  rect.right = right;
  rect.bottom = bottom;
  _context->PushAxisAlignedClip(
      rect, antiAliased ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);
}

void DrawingEngine::PopClipRect() noexcept {
  _context->PopAxisAlignedClip();
}

void DrawingEngine::SetRenderTarget(ID3D11Texture2D* renderTarget) {
  if (!renderTarget) {
    _context->SetTarget(nullptr);
    return;
  }

  // Get the underlying DXGI surface
  winrt::com_ptr<IDXGISurface> dxgiSurface;
  winrt::check_hresult(renderTarget->QueryInterface(dxgiSurface.put()));

  // Create a D2D RT bitmap for it
  D2D1_BITMAP_PROPERTIES1 bitmapProperties{};
  bitmapProperties.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_IGNORE);
  bitmapProperties.dpiX = 96.0f;
  bitmapProperties.dpiY = 96.0f;
  bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;

  winrt::com_ptr<ID2D1Bitmap1> bitmapTarget;
  winrt::check_hresult(_context->CreateBitmapFromDxgiSurface(dxgiSurface.get(), bitmapProperties,
                                                             bitmapTarget.put()));

  _context->SetTarget(bitmapTarget.get());
}

void DrawingEngine::SetTransform(const D2D1_MATRIX_3X2_F& matrix) noexcept {
  _context->SetTransform(matrix);
}

void DrawingEngine::GetTransform(D2D1_MATRIX_3X2_F* matrix) noexcept {
  _context->GetTransform(matrix);
}

void DrawingEngine::SetCanvasSize(float width, float height) noexcept {
  auto realSize = _context->GetPixelSize();
  auto hDpi = 96.0f * static_cast<float>(realSize.width) / width;
  auto vDpi = 96.0f * static_cast<float>(realSize.height) / height;
  _context->SetDpi(hDpi, vDpi);
}

void DrawingEngine::GetCanvasScale(float* width, float* height) noexcept {
  _context->GetDpi(width, height);
  *width /= 96.0f;
  *height /= 96.0f;
}
