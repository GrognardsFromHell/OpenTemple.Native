
#include "TextLayout.h"
#include "DrawingEngine.h"
#include "StyleConversion.h"
#include "TextRenderer.h"
#include "TextRendererStyle.h"

void TextLayout::Render(TextRenderer &renderer, float x, float y, float opacity) {
  TextRendererDrawingContext context(_defaultStyle, opacity);
  _layout->Draw(&context, &renderer, x + _indent, y);
}

constexpr auto RenderingPropertiesMask =
    TextStyleProperty::Color | TextStyleProperty::DropShadowColor | TextStyleProperty::Outline;

void TextLayout::SetStyle(uint32_t start,
                          uint32_t length,
                          TextStyleProperty properties,
                          const TextStyle &style) {
  if (_hangingIndent) {
    start++;
  }

  DWRITE_TEXT_RANGE range{start, length};

  if ((properties & TextStyleProperty::FontFace) == TextStyleProperty::FontFace) {
    winrt::check_hresult(_layout->SetFontFamilyName(style.FontFace, range));
  }

  if ((properties & TextStyleProperty::FontSize) == TextStyleProperty::FontSize) {
    winrt::check_hresult(_layout->SetFontSize(style.FontSize, range));
  }

  if ((properties & TextStyleProperty::Underline) == TextStyleProperty::Underline) {
    winrt::check_hresult(_layout->SetUnderline(style.Underline, range));
  }

  if ((properties & TextStyleProperty::LineThrough) == TextStyleProperty::LineThrough) {
    winrt::check_hresult(_layout->SetStrikethrough(style.LineThrough, range));
  }

  if ((properties & TextStyleProperty::FontStretch) == TextStyleProperty::FontStretch) {
    winrt::check_hresult(_layout->SetFontStretch(ConvertFontStretch(style.FontStretch), range));
  }

  if ((properties & TextStyleProperty::FontStyle) == TextStyleProperty::FontStyle) {
    winrt::check_hresult(_layout->SetFontStyle(ConvertFontStyle(style.FontStyle), range));
  }

  if ((properties & TextStyleProperty::FontWeight) == TextStyleProperty::FontWeight) {
    winrt::check_hresult(_layout->SetFontWeight(ConvertFontWeight(style.FontWeight), range));
  }

  if ((properties & TextStyleProperty::Kerning) == TextStyleProperty::Kerning) {
    // More easily applying kerning is a newer feature
    if (_layout1) {
      winrt::check_hresult(_layout1->SetPairKerning(style.Kerning, range));
    }
  }

  if ((properties & RenderingPropertiesMask) != static_cast<TextStyleProperty>(0)) {
    auto renderStyle = _engine.CreateTextRendererStyle(style);

    winrt::check_hresult(_layout->SetDrawingEffect(renderStyle.get(), range));
  }
}

void TextLayout::SetMaxWidth(float maxWidth) {
  winrt::check_hresult(_layout->SetMaxWidth(maxWidth));
  _metricsDirty = true;
}

void TextLayout::SetMaxHeight(float maxHeight) {
  winrt::check_hresult(_layout->SetMaxHeight(maxHeight));
  _metricsDirty = true;
}

const DWRITE_TEXT_METRICS &TextLayout::GetMetrics() {
  if (_metricsDirty) {
    winrt::check_hresult(_layout->GetMetrics(&_metrics));
    _metricsDirty = false;
  }

  return _metrics;
}

bool TextLayout::GetLineMetrics(DWRITE_LINE_METRICS *lineMetrics,
                                uint32_t count,
                                uint32_t *actualCount) {
  auto hr = _layout->GetLineMetrics(lineMetrics, count, actualCount);
  if (hr == E_NOT_SUFFICIENT_BUFFER) {
    return false;
  }
  winrt::check_hresult(hr);
  return true;
}

bool TextLayout::HitTest(float x, float y, int *position, int *length, bool *trailingHit) {

  x -= _indent;

  BOOL trailingHitBool = 0;
  BOOL inside = 0;
  DWRITE_HIT_TEST_METRICS metrics{};
  winrt::check_hresult(_layout->HitTestPoint(x, y, &trailingHitBool, &inside, &metrics));

  *trailingHit = trailingHitBool;
  *position = (int) metrics.textPosition;
  *length = (int) metrics.length;

  // If we have a hanging indent, the first character is a fake inline object
  // We need to adjust the position accordingly
  if (_hangingIndent) {
    if ((*position)-- == 0) {
      // If we did hit the hanging indent itself, mark it as an outside hit and
      // adjust the trailing hit since it's always "before" the first character
      *position = 0;
      *trailingHit = false;
      inside = 0;
    }
  }

  return inside != 0;
}
