
#include <zlib-ng.h>
#include "../game/utils.h"

NATIVE_API ApiBool zlib_ng_uncompress(uint8_t *dest, size_t *destLen, const uint8_t *src,
                                      size_t *srcLen) {
  auto result = zng_uncompress2(dest, destLen, src, srcLen);
  switch (result) {
    case Z_OK:
      return 0;
    case Z_MEM_ERROR:
      return 1;
    case Z_BUF_ERROR:
      return 2;
    case Z_DATA_ERROR:
      return 3;
    default:
      return 4;
  }
}
