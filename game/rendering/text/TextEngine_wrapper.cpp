
#include "../../interop/string_interop.h"
#include "../../utils.h"
#include "TextEngine.h"

NATIVE_API ApiBool TextEngine_Create(ID2D1RenderTarget *renderTargetPtr,
                                     TextEngine **engine,
                                     char16_t **error) noexcept {
  *error = nullptr;
  *engine = nullptr;

  renderTargetPtr->AddRef();  // We're going to add-ref here
  winrt::com_ptr<ID2D1RenderTarget> renderTarget;
  renderTarget.attach(renderTargetPtr);

  try {
    *engine = new TextEngine(renderTarget);
    return true;
  } catch (const winrt::hresult_error &e) {
    *error = copyString(e.message().c_str());
    return false;
  }
}

NATIVE_API void TextEngine_Free(TextEngine *engine) noexcept {
  delete engine;
}

NATIVE_API void TextEngine_AddFontFile(TextEngine *engine,
                                       const char *filename,
                                       const uint8_t *data,
                                       uint32_t dataLength) noexcept {
  std::string filenameCopy(filename);
  std::vector<uint8_t> dataCopy(data, data + dataLength);

  engine->AddFontFile(FontFile(std::move(filenameCopy), std::move(dataCopy)));
}

NATIVE_API ApiBool TextEngine_ReloadFontFamilies(TextEngine *engine, char16_t **error) noexcept {
  *error = nullptr;

  try {
    engine->ReloadFontFamilies();
    return true;
  } catch (const winrt::hresult_error &e) {
    *error = copyString(e.message().c_str());
    return false;
  }
}

NATIVE_API int TextEngine_GetFontFamiliesCount(TextEngine *engine) noexcept {
  return engine->GetFontFamiliesCount();
}

NATIVE_API char16_t *TextEngine_GetFontFamilyName(TextEngine *engine, int index) noexcept {
  return copyString(engine->GetFontFamilyName(index));
}

NATIVE_API ApiBool TextEngine_CreateTextLayout(TextEngine *engine,
                                               const ParagraphStyle &paragraphStyle,
                                               const TextStyle &textStyle,
                                               const wchar_t *text,
                                               uint32_t textLength,
                                               float maxWidth,
                                               float maxHeight,
                                               TextLayout **textLayout,
                                               char16_t **error) noexcept {
  *error = nullptr;
  *textLayout = nullptr;

  if (!textStyle.FontFace) {
    *error = copyString(L"FontFace must be set.");
    return false;
  }

  try {
    *textLayout =
        engine->CreateTextLayout(paragraphStyle, textStyle, text, textLength, maxWidth, maxHeight);
    return true;
  } catch (const winrt::hresult_error &e) {
    *error = copyString(e.message().c_str());
    return false;
  }
}

NATIVE_API void TextEngine_GetStructSizes(int *paragraphStylesSize, int *textStylesSize) noexcept {
  *paragraphStylesSize = sizeof(ParagraphStyle);
  *textStylesSize = sizeof(TextStyle);
}

NATIVE_API ApiBool TextEngine_RenderTextLayout(
    TextEngine *engine, TextLayout *layout, float x, float y, float opacity, char16_t **error) noexcept {
  if (!engine || !layout) {
    *error = copyString(L"Null-parameter");
    return false;
  }

  *error = nullptr;
  try {
    engine->RenderTextLayout(x, y, *layout, opacity);
    return true;
  } catch (const winrt::hresult_error &e) {
    *error = copyString(e.message().c_str());
    return false;
  } catch (...) {
    *error = copyString(L"Unknown Error");
    return false;
  }
}

NATIVE_API ApiBool TextEngine_RenderBackgroundAndBorder(
    TextEngine *engine, float x, float y, float width, float height,
    const BackgroundAndBorderStyle &style, char16_t **error) noexcept {
  if (!engine) {
    *error = copyString(L"Null-parameter");
    return false;
  }

  *error = nullptr;
  try {
    engine->RenderBackgroundAndBorder(x, y, width, height, style);
    return true;
  } catch (const winrt::hresult_error &e) {
    *error = copyString(e.message().c_str());
    return false;
  } catch (...) {
    *error = copyString(L"Unknown Error");
    return false;
  }
}
