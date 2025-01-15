#include "./safe_buffer.hpp"

#include <sodium/core.h>
#include <sodium/utils.h>

auto diaria_sodium_malloc(std::size_t size) -> void*
{
  if (sodium_init() < 0) {
    throw std::runtime_error("Could not initialize sodium secure memory");
  }
  return sodium_malloc(size);
}
void diaria_sodium_free(void* pointer)
{
  sodium_free(pointer);
}
