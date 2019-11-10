
#include "utils.h"

#include <cstdint>
#include <vector>

NATIVE_API uint32_t vector_get_size(std::vector<uint8_t> *vector) {
  if (vector->size() > std::numeric_limits<uint32_t>::max()) {
    std::abort();
  }
  return (uint32_t)vector->size();
}

NATIVE_API void *vector_get_data(std::vector<uint8_t> *vector) {
  return vector->data();
}
