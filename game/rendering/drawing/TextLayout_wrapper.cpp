
#include "../../interop/string_interop.h"
#include "../../logging/Logger.h"
#include "../../utils.h"
#include "TextLayout.h"

NATIVE_API ApiBool TextLayout_SetStyle(TextLayout *layout,
                                       uint32_t start,
                                       uint32_t length,
                                       TextStyleProperty properties,
                                       const TextStyle &style,
                                       char16_t **error) noexcept {
  *error = nullptr;

  try {
    layout->SetStyle(start, length, properties, style);
    return true;
  } catch (const winrt::hresult_error &e) {
    *error = copyString(e.message().c_str());
    return false;
  }
}

NATIVE_API void TextLayout_Free(TextLayout *layout) noexcept {
  delete layout;
}

struct Metrics {
  float left;
  float top;
  float width;
  float widthIncludingTrailingWhitespace;
  float height;
  float layoutWidth;
  float layoutHeight;
  int maxBidiReorderingDepth;
  int lineCount;
};

NATIVE_API void TextLayout_GetMetrics(TextLayout *layout, Metrics *metricsOut) noexcept {
  auto &metrics = layout->GetMetrics();

  metricsOut->left = metrics.left;
  metricsOut->top = metrics.top;
  metricsOut->width = metrics.width;
  metricsOut->widthIncludingTrailingWhitespace = metrics.widthIncludingTrailingWhitespace;
  metricsOut->height = metrics.height;
  metricsOut->layoutWidth = metrics.layoutWidth;
  metricsOut->layoutHeight = metrics.layoutHeight;
  metricsOut->maxBidiReorderingDepth = (int)metrics.maxBidiReorderingDepth;
  metricsOut->lineCount = (int)metrics.lineCount;
}

struct LineMetrics {
  int length;
  int trailingWhitespaceLength;
  int newlineLength;
  float height;
  float baseline;
  bool isTrimmed;
};

static void ConvertLineMetrics(DWRITE_LINE_METRICS *lineMetrics,
                               LineMetrics *lineMetricsOut,
                               uint32_t count) {
  for (auto i = 0u; i < count; i++) {
    auto &in = lineMetrics[i];
    auto &out = lineMetricsOut[i];

    out.length = (int)in.length;
    out.trailingWhitespaceLength = (int)in.trailingWhitespaceLength;
    out.newlineLength = (int)in.newlineLength;
    out.height = in.height;
    out.baseline = in.baseline;
    out.isTrimmed = in.isTrimmed;
  }
}

static constexpr uint32_t StackAllocatedLineMetrics = 100;

NATIVE_API ApiBool TextLayout_GetLineMetrics(TextLayout *layout,
                                             LineMetrics *lineMetricsOut,
                                             uint32_t lineMetricsSize,
                                             uint32_t *actualLineCount) noexcept {
  DWRITE_LINE_METRICS lineMetricsStack[StackAllocatedLineMetrics];

  // This is a way to get the actual line count
  if (lineMetricsSize == 0) {
    layout->GetLineMetrics(lineMetricsStack, 0, actualLineCount);
    return false;
  }

  // Use a stack allocated array if possible
  ApiBool result;
  if (lineMetricsSize <= StackAllocatedLineMetrics) {
    result = layout->GetLineMetrics(lineMetricsStack, lineMetricsSize, actualLineCount);

    auto linesToConvert = std::min<uint32_t>(lineMetricsSize, *actualLineCount);
    ConvertLineMetrics(lineMetricsStack, lineMetricsOut, linesToConvert);
  } else {
    std::vector<DWRITE_LINE_METRICS> lineMetricsHeap;
    lineMetricsHeap.resize(lineMetricsSize);
    result = layout->GetLineMetrics(&lineMetricsHeap[0], lineMetricsSize, actualLineCount);

    auto linesToConvert = std::min<uint32_t>(lineMetricsSize, *actualLineCount);
    ConvertLineMetrics(&lineMetricsHeap[0], lineMetricsOut, linesToConvert);
  }

  return result;
}

NATIVE_API ApiBool TextLayout_HitTestPoint(
    TextLayout *layout, float x, float y, int *start, int *length, ApiBool *trailingHit) noexcept {
  try {
    bool trailingHitBool = false;
    auto hit = layout->HitTestPoint(x, y, start, length, &trailingHitBool);
    *trailingHit = trailingHitBool;
    return hit;
  } catch (winrt::hresult_error &e) {
    Logger::Error(std::wstring(L"TextLayout_HitTestPoint failed: ") + e.message());
    return false;
  }
}

struct HitTestRect {
  float x;
  float y;
  float width;
  float height;
};

static void ConvertHitTestRects(DWRITE_HIT_TEST_METRICS *hitMetrics,
                                HitTestRect *rectsOut,
                                uint32_t count) {
  for (auto i = 0u; i < count; i++) {
    auto &in = hitMetrics[i];
    auto &out = rectsOut[i];

    out.x = in.left;
    out.y = in.top;
    out.width = in.width;
    out.height = in.height;
  }
}

NATIVE_API void TextLayout_HitTestTextPosition(TextLayout *layout,
                                               int textPosition,
                                               ApiBool afterPosition,
                                               HitTestRect *rect) noexcept {
  try {
    DWRITE_HIT_TEST_METRICS metrics{};
    layout->HitTestTextPosition(textPosition, afterPosition, &metrics);
    ConvertHitTestRects(&metrics, rect, 1);
  } catch (winrt::hresult_error &e) {
    *rect = {};
    Logger::Error(std::wstring(L"TextLayout_HitTestTextPosition failed: ") + e.message());
  }
}

static constexpr uint32_t StackAllocatedHitTestMetrics = 100;

NATIVE_API ApiBool TextLayout_HitTestTextRange(TextLayout *layout,
                                               int textPosition,
                                               int length,
                                               HitTestRect *rectsOut,
                                               uint32_t rectsCount,
                                               uint32_t *actualRectsCount) noexcept {
  DWRITE_HIT_TEST_METRICS metricsStack[StackAllocatedHitTestMetrics];

  // This is a way to get the actual line count
  if (rectsCount == 0) {
    layout->HitTestTextRange(textPosition, length, metricsStack, 0, actualRectsCount);
    return false;
  }

  // Use a stack allocated array if possible
  ApiBool result;
  if (rectsCount <= StackAllocatedHitTestMetrics) {
    result =
        layout->HitTestTextRange(textPosition, length, metricsStack, rectsCount, actualRectsCount);

    auto rectsToConvert = std::min<uint32_t>(rectsCount, *actualRectsCount);
    ConvertHitTestRects(metricsStack, rectsOut, rectsToConvert);
  } else {
    std::vector<DWRITE_HIT_TEST_METRICS> hitMetricsHeap;
    hitMetricsHeap.resize(rectsCount);
    result = layout->HitTestTextRange(textPosition, length, &hitMetricsHeap[0], rectsCount, actualRectsCount);

    auto rectsToConvert = std::min<uint32_t>(rectsCount, *actualRectsCount);
    ConvertHitTestRects(&hitMetricsHeap[0], rectsOut, rectsToConvert);
  }

  return result;
}

NATIVE_API ApiBool TextLayout_SetMaxWidth(TextLayout *layout, float maxWidth) noexcept {
  try {
    layout->SetMaxWidth(maxWidth);
    return true;
  } catch (const winrt::hresult_error &e) {
    return false;
  }
}

NATIVE_API ApiBool TextLayout_SetMaxHeight(TextLayout *layout, float maxHeight) noexcept {
  try {
    layout->SetMaxHeight(maxHeight);
    return true;
  } catch (const winrt::hresult_error &e) {
    return false;
  }
}
