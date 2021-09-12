
#include <d3d.h>
#include "../../interop/string_interop.h"
#include "../../logging/Logger.h"
#include "../../utils.h"
#include "DrawingEngine.h"

NATIVE_API ApiBool DrawingEngine_Create(ID3D11Device *unownedD3dDevice,
                                        ApiBool debugDevice,
                                        DrawingEngine **engine,
                                        char16_t **error) noexcept {
  *error = nullptr;
  *engine = nullptr;

  unownedD3dDevice->AddRef();  // We're going to add-ref here
  winrt::com_ptr<ID3D11Device> d3dDevice;
  d3dDevice.attach(unownedD3dDevice);

  try {
    *engine = new DrawingEngine(d3dDevice, debugDevice);
    return true;
  } catch (const winrt::hresult_error &e) {
    *error = copyString(e.message().c_str());
    return false;
  }
}

NATIVE_API void DrawingEngine_Free(DrawingEngine *engine) noexcept {
  delete engine;
}

NATIVE_API void DrawingEngine_BeginDraw(DrawingEngine *engine) noexcept {
  engine->BeginDraw();
}

NATIVE_API ApiBool DrawingEngine_EndDraw(DrawingEngine *engine, char16_t **error) noexcept {
  try {
    engine->EndDraw();
    *error = nullptr;
    return true;
  } catch (const winrt::hresult_error &e) {
    *error = copyString(e.message().c_str());
    return false;
  }
}

NATIVE_API void DrawingEngine_PushClipRect(DrawingEngine *engine, float left, float top,
                                           float right, float bottom,
                                           ApiBool antiAliased) noexcept {
  engine->PushClipRect(left, top, right, bottom, antiAliased);
}

NATIVE_API void DrawingEngine_PopClipRect(DrawingEngine *engine) noexcept {
  engine->PopClipRect();
}

NATIVE_API ApiBool DrawingEngine_SetRenderTarget(DrawingEngine *engine,
                                                 ID3D11Texture2D *texture,
                                                 char16_t **error) noexcept {
  try {
    engine->SetRenderTarget(texture);
    *error = nullptr;
    return true;
  } catch (const winrt::hresult_error &e) {
    *error = copyString(e.message().c_str());
    return false;
  }
}

NATIVE_API void DrawingEngine_SetTransform(DrawingEngine *engine,
                                           D2D1_MATRIX_3X2_F *matrix) noexcept {
  engine->SetTransform(*matrix);
}

NATIVE_API void DrawingEngine_GetTransform(DrawingEngine *engine,
                                           D2D1_MATRIX_3X2_F *matrix) noexcept {
  engine->GetTransform(matrix);
}

NATIVE_API void DrawingEngine_SetCanvasSize(DrawingEngine *engine,
                                            float width,
                                            float height) noexcept {
  engine->SetCanvasSize(width, height);
}

NATIVE_API void DrawingEngine_GetCanvasScale(DrawingEngine *engine,
                                             float *width,
                                             float *height) noexcept {
  engine->GetCanvasScale(width, height);
}

NATIVE_API void DrawingEngine_AddFontFile(DrawingEngine *engine,
                                          const char *filename,
                                          const uint8_t *data,
                                          uint32_t dataLength) noexcept {
  std::string filenameCopy(filename);
  std::vector<uint8_t> dataCopy(data, data + dataLength);

  engine->AddFontFile(FontFile(std::move(filenameCopy), std::move(dataCopy)));
}

NATIVE_API ApiBool DrawingEngine_ReloadFontFamilies(DrawingEngine *engine,
                                                    char16_t **error) noexcept {
  *error = nullptr;

  try {
    engine->ReloadFontFamilies();
    return true;
  } catch (const winrt::hresult_error &e) {
    *error = copyString(e.message().c_str());
    return false;
  }
}

NATIVE_API int DrawingEngine_GetFontFamiliesCount(DrawingEngine *engine) noexcept {
  return engine->GetFontFamiliesCount();
}

NATIVE_API char16_t *DrawingEngine_GetFontFamilyName(DrawingEngine *engine, int index) noexcept {
  return copyString(engine->GetFontFamilyName(index));
}

NATIVE_API ApiBool DrawingEngine_CreateTextLayout(DrawingEngine *engine,
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

NATIVE_API void DrawingEngine_GetStructSizes(int *paragraphStylesSize,
                                             int *textStylesSize) noexcept {
  *paragraphStylesSize = sizeof(ParagraphStyle);
  *textStylesSize = sizeof(TextStyle);
}

NATIVE_API ApiBool DrawingEngine_RenderTextLayout(DrawingEngine *engine, TextLayout *layout,
                                                  float x, float y, float opacity,
                                                  char16_t **error) noexcept {
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

NATIVE_API ApiBool DrawingEngine_RenderBackgroundAndBorder(DrawingEngine *engine, float x, float y,
                                                           float width, float height,
                                                           const BackgroundAndBorderStyle &style,
                                                           char16_t **error) noexcept {
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
