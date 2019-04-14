
#include <turbojpeg.h>

#include "utils.h"
#include <cstdint>

NATIVE_API tjhandle Jpeg_CreateDecompressor()
{
	return tjInitDecompress();
}

NATIVE_API ApiBool Jpeg_ReadHeader(tjhandle decoder, uint8_t *imageData, uint32_t imageDataSize, int* width, int *height)
{
	return tjDecompressHeader(decoder, imageData, imageDataSize, width, height) == 0;
}

NATIVE_API ApiBool Jpeg_Decode(tjhandle decoder, uint8_t *imageData, uint32_t imageDataSize, uint8_t *decodedData, int width, int stride, int height, TJPF pixelFormat, int flags)
{
	return tjDecompress2(decoder, imageData, imageDataSize, decodedData, width, stride, height, pixelFormat, flags) == 0;
}

NATIVE_API void Jpeg_DestroyDecompressor(tjhandle decoder)
{
	tjDestroy(decoder);
}
