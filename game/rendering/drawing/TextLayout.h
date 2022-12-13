
#pragma once

#include <dwrite_2.h>
#include <winrt/base.h>

#include <utility>
#include <vector>

#include "Style.h"
#include "TextRendererStyle.h"

class TextRenderer;
class DrawingEngine;

class TextLayout {
 public:
  TextLayout(DrawingEngine &engine,
             const winrt::com_ptr<IDWriteTextLayout> &layout,
             const bool hangingIndent,
             const float indent,
             TextRendererStyle defaultStyle)
      : _engine(engine),
        _layout(layout),
        _layout1(layout.try_as<IDWriteTextLayout1>()),
        _layout2(layout.try_as<IDWriteTextLayout2>()),
        _hangingIndent(hangingIndent),
        _indent(indent),
        _defaultStyle(std::move(defaultStyle)) {}

  void Render(TextRenderer &renderer, float x, float y, float opacity);

  void SetStyle(uint32_t start,
                uint32_t length,
                TextStyleProperty properties,
                const TextStyle &style);

  const DWRITE_TEXT_METRICS &GetMetrics();

  /**
   * @return false if the given buffer is not large enough to hold the line metrics,
   * actualCount will be initialized to the correct size.
   */
  bool GetLineMetrics(DWRITE_LINE_METRICS *lineMetrics, uint32_t count, uint32_t *actualCount);

  void SetMaxWidth(float maxWidth);

  void SetMaxHeight(float maxHeight);

  bool HitTestPoint(float x, float y, int *position, int *length, bool *trailingHit);

  /**
   * @param textPosition
   * @param trailingHit Indicates that the returned position and metrics should be for text
   * after the position. If a font size change occurs at the given text position, this
   * determines whether the returned position will be for the smaller or larger font.
   */
  void HitTestTextPosition(uint32_t textPosition, bool afterPosition, DWRITE_HIT_TEST_METRICS *metrics);

  bool HitTestTextRange(uint32_t start,
                        uint32_t length,
                        DWRITE_HIT_TEST_METRICS *metrics,
                        uint32_t metricsCount,
                        uint32_t *actualMetricsCount);

 private:
  DrawingEngine &_engine;
  const bool _hangingIndent;
  const float _indent;
  bool _metricsDirty = true;
  TextRendererStyle _defaultStyle;
  winrt::com_ptr<IDWriteTextLayout> _layout;
  winrt::com_ptr<IDWriteTextLayout1> _layout1;
  winrt::com_ptr<IDWriteTextLayout2> _layout2;
  DWRITE_TEXT_METRICS _metrics{};
};
