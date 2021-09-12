
#pragma once

#include <d2d1_1.h>
#include <dwrite_3.h>

#include "LRUCache11.h"
#include "Style.h"

struct TextRendererStyle;

struct TextRendererDrawingContext {
  /**
   * The default style to use for text that has no other styles set.
   */
  const TextRendererStyle &defaultStyle;

  const float opacity;

  explicit TextRendererDrawingContext(const TextRendererStyle &defaultStyle, float opacity)
      : defaultStyle(defaultStyle), opacity(opacity) {}
};

class TextRenderer : public IDWriteTextRenderer {
 public:
  TextRenderer(const winrt::com_ptr<IDWriteFactory> &dWriteFactory,
               const winrt::com_ptr<ID2D1DeviceContext> &deviceContext);

  HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void *clientDrawingContext, BOOL *isDisabled) noexcept override;
  HRESULT STDMETHODCALLTYPE GetCurrentTransform(void *clientDrawingContext,
                              DWRITE_MATRIX *transform) noexcept override;
  HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void *clientDrawingContext, FLOAT *pixelsPerDip) noexcept override;
  HRESULT STDMETHODCALLTYPE DrawGlyphRun(void *clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY,
                       DWRITE_MEASURING_MODE measuringMode, const DWRITE_GLYPH_RUN *glyphRun,
                       const DWRITE_GLYPH_RUN_DESCRIPTION *glyphRunDescription,
                       IUnknown *clientDrawingEffect) noexcept override;
  HRESULT STDMETHODCALLTYPE DrawUnderline(void *clientDrawingContext, FLOAT baselineOriginX, FLOAT baselineOriginY,
                        const DWRITE_UNDERLINE *underline,
                        IUnknown *clientDrawingEffect) noexcept override;
  HRESULT STDMETHODCALLTYPE DrawStrikethrough(void *clientDrawingContext, FLOAT baselineOriginX,
                            FLOAT baselineOriginY, const DWRITE_STRIKETHROUGH *strikethrough,
                            IUnknown *clientDrawingEffect) noexcept override;
  HRESULT STDMETHODCALLTYPE DrawInlineObject(void *clientDrawingContext, FLOAT originX, FLOAT originY,
                           IDWriteInlineObject *inlineObject, BOOL isSideways, BOOL isRightToLeft,
                           IUnknown *clientDrawingEffect) noexcept override;
  HRESULT STDMETHODCALLTYPE QueryInterface(const IID &riid, void **ppvObject) override;
  ULONG STDMETHODCALLTYPE AddRef() override;
  ULONG STDMETHODCALLTYPE Release() override;

 private:
  winrt::com_ptr<IDWriteFactory2> _dWriteFactory2;
  winrt::com_ptr<ID2D1Factory> _factory;
  winrt::com_ptr<ID2D1DeviceContext> _context;

  void FillRectangle(const D2D_RECT_F &rect, const TextRendererStyle &style, float opacity);

  /**
   * Draws the colored layers of the given glyph run (if it has any) and returns true.
   * Returns false if there are no colored layers.
   */
  bool DrawColoredLayers(float baselineOriginX, float baselineOriginY,
                         DWRITE_MEASURING_MODE &measuringMode, const DWRITE_GLYPH_RUN *glyphRun,
                         const TextRendererStyle &style,
                         float opacity);

  void DrawOutline(const TextRendererStyle &style, float x, float y,
                   const DWRITE_GLYPH_RUN *glyphRun,
                   float opacity);
};
