
#include <d2d1_2.h>
#include <dwrite_2.h>
#include <winrt/base.h>
#include <functional>

#include "StyleConversion.h"
#include "TextRenderer.h"
#include "TextRendererStyle.h"

static inline void WithOpacity(ID2D1Brush *brush, float opacity,
                               const std::function<void(ID2D1Brush *)> &callback) {
  if (opacity < 1) {
    float orgOpacity = brush->GetOpacity();
    brush->SetOpacity(opacity);
    callback(brush);
    brush->SetOpacity(orgOpacity);
  } else {
    callback(brush);
  }
}

static inline TextRendererDrawingContext &GetDrawingContext(void *clientDrawingContext) {
  return *reinterpret_cast<TextRendererDrawingContext *>(clientDrawingContext);
}

static inline const TextRendererStyle &GetStyle(const TextRendererDrawingContext &context,
                                                IUnknown *clientDrawingEffect) {
  auto style = reinterpret_cast<TextRendererStyle *>(clientDrawingEffect);
  return style ? *style : context.defaultStyle;
}

TextRenderer::TextRenderer(const winrt::com_ptr<IDWriteFactory> &dWriteFactory,
                           const winrt::com_ptr<ID2D1DeviceContext> &deviceContext)
    : _dWriteFactory2(dWriteFactory.as<IDWriteFactory2>()), _context(deviceContext) {
  _context->GetFactory(_factory.put());
}

HRESULT TextRenderer::IsPixelSnappingDisabled(void *clientDrawingContext,
                                              BOOL *isDisabled) noexcept {
  *isDisabled = FALSE;
  return S_OK;
}

HRESULT TextRenderer::GetCurrentTransform(void *clientDrawingContext,
                                          DWRITE_MATRIX *transform) noexcept {
  _context->GetTransform(reinterpret_cast<D2D1_MATRIX_3X2_F *>(transform));
  return S_OK;
}

HRESULT TextRenderer::GetPixelsPerDip(void *clientDrawingContext, FLOAT *pixelsPerDip) noexcept {
  float unused;
  _context->GetDpi(pixelsPerDip, &unused);
  return S_OK;
}

HRESULT TextRenderer::DrawGlyphRun(void *clientDrawingContext,
                                   FLOAT baselineOriginX,
                                   FLOAT baselineOriginY,
                                   DWRITE_MEASURING_MODE measuringMode,
                                   const DWRITE_GLYPH_RUN *glyphRun,
                                   const DWRITE_GLYPH_RUN_DESCRIPTION *glyphRunDescription,
                                   IUnknown *clientDrawingEffect) noexcept {
  auto context = GetDrawingContext(clientDrawingContext);
  float opacity = context.opacity;
  auto &style = GetStyle(context, clientDrawingEffect);

  // If the style dictates a drop shadow, draw it first by simply drawing the glyph run
  // offset by 1,1
  auto &shadowBrush = style.GetDropShadowColor();
  if (shadowBrush) {
    auto origin = D2D1::Point2(baselineOriginX + 1, baselineOriginY + 1);
    WithOpacity(shadowBrush.get(), opacity, [&](auto brush) {
      _context->DrawGlyphRun(origin, glyphRun, brush, measuringMode);
    });
  }

  // Attempt to draw colored glyph layers first,
  // and only if that fails, draw the solid colored glyphs
  if (!DrawColoredLayers(baselineOriginX, baselineOriginY, measuringMode, glyphRun, style,
                         opacity)) {
    if (style.HasOutline()) {
      DrawOutline(style, baselineOriginX, baselineOriginY, glyphRun, opacity);
    } else {
      auto brush = style.GetColor().get();
      WithOpacity(brush, opacity, [&](auto brush) {
        _context->DrawGlyphRun(D2D1::Point2(baselineOriginX, baselineOriginY), glyphRun, brush,
                                    measuringMode);
      });
    }
  }

  return S_OK;
}

bool TextRenderer::DrawColoredLayers(float baselineOriginX,
                                     float baselineOriginY,
                                     DWRITE_MEASURING_MODE &measuringMode,
                                     const DWRITE_GLYPH_RUN *glyphRun,
                                     const TextRendererStyle &style,
                                     float opacity) {
  // IDWriteFactory2 -> Windows 8.1 -> Support colored emojis / font glyphs
  if (!_dWriteFactory2) {
    return false;
  }

  // Analyze the color layers in the current glyph run
  DWRITE_MATRIX *worldToDeviceTransform = nullptr;
  winrt::com_ptr<IDWriteColorGlyphRunEnumerator> colorLayers;
  auto result = _dWriteFactory2->TranslateColorGlyphRun(baselineOriginX,
                                                        baselineOriginY,
                                                        glyphRun,
                                                        nullptr,
                                                        measuringMode,
                                                        worldToDeviceTransform,
                                                        0,
                                                        colorLayers.put());

  if (result == DWRITE_E_NOCOLOR) {
    return false;
  }

  winrt::check_hresult(result);

  // DirectWrite will tell us if the glyph run has no layers to save us time
  winrt::com_ptr<ID2D1SolidColorBrush> solidBrush;

  // If it has layers, draw each layer with the correct color
  for (;;) {
    BOOL haveRun;
    winrt::check_hresult(colorLayers->MoveNext(&haveRun));
    if (!haveRun)
      break;

    DWRITE_COLOR_GLYPH_RUN const *colorRun;
    winrt::check_hresult(colorLayers->GetCurrentRun(&colorRun));

    ID2D1Brush *layerBrush;
    if (colorRun->paletteIndex == 0xFFFF) {
      layerBrush = style.GetColor().get();
    } else {
      if (solidBrush == nullptr) {
        winrt::check_hresult(
            _context->CreateSolidColorBrush(&colorRun->runColor, nullptr, solidBrush.put()));
      } else {
        solidBrush->SetColor(colorRun->runColor);
      }
      layerBrush = solidBrush.get();
    }

    WithOpacity(layerBrush, opacity, [&](auto brush) {
      _context->DrawGlyphRun(
          D2D1::Point2(colorRun->baselineOriginX, colorRun->baselineOriginY),
          &colorRun->glyphRun,
          brush,
          measuringMode);
    });
  }

  return true;
}

void TextRenderer::FillRectangle(const D2D_RECT_F &rect,
                                 const TextRendererStyle &style,
                                 float opacity) {
  WithOpacity(style.GetColor().get(), opacity,
              [&](auto brush) { _context->FillRectangle(&rect, brush); });
}

HRESULT TextRenderer::DrawUnderline(void *clientDrawingContext, FLOAT baselineOriginX,
                                    FLOAT baselineOriginY, const DWRITE_UNDERLINE *underline,
                                    IUnknown *clientDrawingEffect) noexcept {
  auto context = GetDrawingContext(clientDrawingContext);
  auto &style = GetStyle(context, clientDrawingEffect);

  auto rect = D2D1::RectF(baselineOriginX,
                          baselineOriginY + underline->offset,
                          baselineOriginX + underline->width,
                          baselineOriginY + underline->offset + underline->thickness);

  FillRectangle(rect, style, context.opacity);

  return S_OK;
}

HRESULT TextRenderer::DrawStrikethrough(void *clientDrawingContext, FLOAT baselineOriginX,
                                        FLOAT baselineOriginY,
                                        const DWRITE_STRIKETHROUGH *strikethrough,
                                        IUnknown *clientDrawingEffect) noexcept {
  auto context = GetDrawingContext(clientDrawingContext);
  auto &style = GetStyle(context, clientDrawingEffect);

  auto rect = D2D1::RectF(baselineOriginX,
                          baselineOriginY + strikethrough->offset,
                          baselineOriginX + strikethrough->width,
                          baselineOriginY + strikethrough->offset + strikethrough->thickness);

  FillRectangle(rect, style, context.opacity);

  return S_OK;
}

HRESULT TextRenderer::DrawInlineObject(void *clientDrawingContext, FLOAT originX, FLOAT originY,
                                       IDWriteInlineObject *inlineObject, BOOL isSideways,
                                       BOOL isRightToLeft, IUnknown *clientDrawingEffect) noexcept {
  return inlineObject->Draw(
      clientDrawingContext,
      this,
      originX,
      originY,
      isSideways,
      isRightToLeft,
      clientDrawingEffect
  );
}

HRESULT TextRenderer::QueryInterface(const IID &riid, void **ppvObject) {
  if (__uuidof(IDWriteTextRenderer) == riid || __uuidof(IDWritePixelSnapping) == riid ||
      __uuidof(IUnknown) == riid) {
    *ppvObject = this;
    this->AddRef();
    return S_OK;
  } else {
    *ppvObject = nullptr;
    return E_NOINTERFACE;
  }
}

ULONG TextRenderer::AddRef() {
  return 0;
}

ULONG TextRenderer::Release() {
  return 0;
}

/**
 * Convenience function to create a path geometry using a geometry sink.
 */
inline winrt::com_ptr<ID2D1PathGeometry> CreatePathGeometry(
    ID2D1Factory &factory, const std::function<void(ID2D1GeometrySink *)> &consumer) {
  winrt::com_ptr<ID2D1PathGeometry> pathGeometry;
  winrt::check_hresult(factory.CreatePathGeometry(pathGeometry.put()));

  winrt::com_ptr<ID2D1GeometrySink> sink;
  winrt::check_hresult(pathGeometry->Open(sink.put()));
  consumer(sink.get());
  winrt::check_hresult(sink->Close());

  return std::move(pathGeometry);
}

void TextRenderer::DrawOutline(const TextRendererStyle &style,
                               float x,
                               float y,
                               const DWRITE_GLYPH_RUN *glyphRun,
                               float opacity) {
  auto pathGeometry = CreatePathGeometry(*_factory, [&](auto sink) {
    auto fontFace = glyphRun->fontFace;
    winrt::check_hresult(fontFace->GetGlyphRunOutline(glyphRun->fontEmSize,
                                                      glyphRun->glyphIndices,
                                                      glyphRun->glyphAdvances,
                                                      glyphRun->glyphOffsets,
                                                      glyphRun->glyphCount,
                                                      glyphRun->isSideways,
                                                      glyphRun->bidiLevel % 2 != 0,
                                                      sink));
  });

  // Initialize a matrix to translate the origin of the glyph run.
  auto matrix = new D2D1::Matrix3x2F(1.0f, 0.0f, 0.0f, 1.0f, x, y);
  winrt::com_ptr<ID2D1TransformedGeometry> transformedGeometry;
  winrt::check_hresult(
      _factory->CreateTransformedGeometry(pathGeometry.get(), matrix, transformedGeometry.put()));

  auto outlineWidth = style.GetOutlineWidth();
  auto &outlineBrush = style.GetOutlineColor();
  auto &fillBrush = style.GetColor();

  if (fillBrush) {
    WithOpacity(fillBrush.get(), opacity,
                [&](auto brush) { _context->FillGeometry(transformedGeometry.get(), brush); });
  }
  if (outlineBrush) {
    WithOpacity(outlineBrush.get(), opacity, [&](auto brush) {
      _context->DrawGeometry(transformedGeometry.get(), brush, outlineWidth);
    });
  }
}
