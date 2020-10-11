
#include <turbojpeg.h>

#include <cstdint>
#include <cstdlib>
#include "../game/utils.h"

enum class JpegPixelFormat : int { RGB = 0, BGR, RGBX, BGRX, XBGR, XRGB };

static TJPF ConvertPixelFormat(JpegPixelFormat format) {
  switch (format) {
    case JpegPixelFormat::RGB:
      return TJPF_RGB;
    case JpegPixelFormat::BGR:
      return TJPF_BGR;
    case JpegPixelFormat::RGBX:
      return TJPF_RGBX;
    case JpegPixelFormat::BGRX:
      return TJPF_BGRX;
    case JpegPixelFormat::XBGR:
      return TJPF_XBGR;
    case JpegPixelFormat::XRGB:
      return TJPF_XRGB;
    default:
      std::abort();
  }
}

NATIVE_API tjhandle Jpeg_CreateEncoder() { return tjInitTransform(); }

NATIVE_API uint32_t Jpeg_GetEncoderBufferSize(int width, int height) {
  return tjBufSize(width, height, TJSAMP_444);
}

NATIVE_API ApiBool Jpeg_Encode(tjhandle encoder, const uint8_t *pixelData,
                               int width, int pitch, int height,
                               JpegPixelFormat pixelFormat,
                               uint8_t *outputBuffer,
                               uint32_t *outputBufferSize, int quality) {
  auto pf = ConvertPixelFormat(pixelFormat);

  unsigned long outputBufSize = *outputBufferSize;
  auto result =
      tjCompress2(encoder, pixelData, width, pitch, height, pf, &outputBuffer,
                  &outputBufSize, TJSAMP_444, quality, TJFLAG_NOREALLOC);

  *outputBufferSize = outputBufSize;
  return result == 0;
}

NATIVE_API tjhandle Jpeg_CreateDecompressor() { return tjInitDecompress(); }

NATIVE_API ApiBool Jpeg_ReadHeader(tjhandle decoder, uint8_t *imageData,
                                   uint32_t imageDataSize, int *width,
                                   int *height) {
  return tjDecompressHeader(decoder, imageData, imageDataSize, width, height) ==
         0;
}

NATIVE_API ApiBool Jpeg_Decode(tjhandle decoder, uint8_t *imageData,
                               uint32_t imageDataSize, uint8_t *decodedData,
                               int width, int stride, int height,
                               JpegPixelFormat pixelFormat, int flags) {
  auto pf = ConvertPixelFormat(pixelFormat);
  return tjDecompress2(decoder, imageData, imageDataSize, decodedData, width,
                       stride, height, pf, flags) == 0;
}

NATIVE_API void Jpeg_Destroy(tjhandle handle) { tjDestroy(handle); }
