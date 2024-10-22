#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <print>
#include <ranges>
#include <stdexcept>

#include <sodium/utils.h>

class safe_buffer
{
  char* buffer;
  std::size_t buffer_size;

public:
  explicit safe_buffer(const std::filesystem::path& path)
  {
    std::ifstream stream(path, std::ios::in | std::ios::binary);
    if (stream.fail()) {
      throw std::runtime_error(
          std::format("Could not open file {}", path.c_str()));
    }
    buffer_size = std::filesystem::file_size(path);
    buffer = static_cast<char*>(sodium_malloc(buffer_size));
    if (buffer == nullptr) {
      throw std::runtime_error("Could not allocate safe memory");
    }
    std::copy_n((std::istreambuf_iterator<char>(stream)), buffer_size, buffer);
    if (stream.fail()) {
      throw std::runtime_error(
          std::format("Could not read file {}", path.c_str()));
    }
    if (!stream.eof()) {
      throw std::runtime_error(
          std::format("Did not read entire file {}", path.c_str()));
    }
  }
  [[nodiscard]] auto begin() const { return buffer; }
  [[nodiscard]] auto end() const { return buffer + buffer_size; }
  [[nodiscard]]
  auto begin()
  {
    return buffer;
  }
  [[nodiscard]] auto end() { return buffer + buffer_size; }
};

static_assert(std::ranges::range<safe_buffer>);
static_assert(std::ranges::random_access_range<safe_buffer>);
static_assert(std::ranges::sized_range<safe_buffer>);
