
#include "utils.h"

#include <cstdint>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_TGA
#define STBI_ONLY_BMP
#define STBI_ONLY_PNG
#include <stb_image.h>

NATIVE_API ApiBool Stb_BmpInfo(uint8_t *imageData, uint32_t imageDataSize,
                               int *width, int *height, ApiBool *hasAlpha) {
  stbi__context ctx;
  stbi__start_mem(&ctx, imageData, imageDataSize);

  int comp;
  if (stbi__bmp_info(&ctx, width, height, &comp)) {
    *hasAlpha = (comp == 4);
    return true;
  }

  return false;
}

NATIVE_API void *Stb_BmpDecode(uint8_t *imageData, uint32_t imageDataSize,
                               int *width, int *height, ApiBool *hasAlpha,
                               uint32_t *pixelDataSize) {
  stbi__context ctx;
  stbi__start_mem(&ctx, imageData, imageDataSize);

  int comp;
  stbi__result_info ri;
  auto data = stbi__bmp_load(&ctx, width, height, &comp, 4, &ri);
  *hasAlpha = (comp == 4);
  *pixelDataSize = (uint32_t)(*width * *height * 4);

  return data;
}

// Frees memory returned by Stb_BmpDecode
NATIVE_API void Stb_BmpFree(void *data) { STBI_FREE(data); }

//// PNG
NATIVE_API ApiBool Stb_PngInfo(uint8_t *imageData, uint32_t imageDataSize,
                               int *width, int *height, ApiBool *hasAlpha) {
  stbi__context ctx;
  stbi__start_mem(&ctx, imageData, imageDataSize);

  int comp;
  if (stbi__png_info(&ctx, width, height, &comp)) {
    *hasAlpha = (comp == 4);
    return true;
  }

  return false;
}

NATIVE_API void *Stb_PngDecode(uint8_t *imageData, uint32_t imageDataSize,
                               int *width, int *height, ApiBool *hasAlpha,
                               uint32_t *pixelDataSize) {
  stbi__context ctx;
  stbi__start_mem(&ctx, imageData, imageDataSize);

  int comp;
  stbi__result_info ri;
  auto data = stbi__png_load(&ctx, width, height, &comp, 4, &ri);
  *hasAlpha = (comp == 4);
  *pixelDataSize = (uint32_t)(*width * *height * 4);

  return data;
}

// Frees memory returned by Stb_PngDecode
NATIVE_API void Stb_PngFree(void *data) { STBI_FREE(data); }
