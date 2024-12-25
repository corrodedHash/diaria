#pragma once
#include <cstddef>
#include <cstdlib>
#include <print>
#include <vector>

#include "safe_allocator.hpp"

template<typename T, std::size_t Size>
class safe_array
{
  std::span<T, Size> buffer;

public:
  safe_array()
      : buffer(sodium_allocator<T>().allocate(Size), Size)
  {
    if (buffer.data() == nullptr) {
      throw std::runtime_error("Could not allocate safe memory");
    }
  }
  ~safe_array() { sodium_allocator<T>().deallocate(buffer.data(), Size); }
  safe_array(const safe_array&) = delete;
  auto operator=(const safe_array&) -> safe_array& = delete;
  safe_array(safe_array&& other) noexcept
      : buffer(other.buffer)
  {
    other.buffer = std::span<T, Size>(static_cast<T*>(nullptr), Size);
  }
  auto operator=(safe_array&& other) noexcept -> safe_array&
  {
    buffer = other.buffer;
    other.buffer = std::span<T, Size>(static_cast<T*>(nullptr), Size);
    return *this;
  }

  [[nodiscard]] auto begin() const { return buffer.cbegin(); }
  [[nodiscard]] auto span() const { return buffer; }
  [[nodiscard]] auto end() const { return buffer.cend(); }
  [[nodiscard]] auto begin() { return buffer.begin(); }
  [[nodiscard]] auto end() { return buffer.end(); }
  [[nodiscard]] auto data() { return buffer.data(); }
  [[nodiscard]] auto data() const { return buffer.data(); }
  [[nodiscard]] auto size() const { return buffer.size(); }
};

constexpr size_t safe_array_test_length = 5;
static_assert(
    std::ranges::range<safe_array<unsigned char, safe_array_test_length>>);
static_assert(std::ranges::random_access_range<
              safe_array<unsigned char, safe_array_test_length>>);
static_assert(std::ranges::sized_range<
              safe_array<unsigned char, safe_array_test_length>>);

template<typename T>
using safe_vector = std::vector<T, sodium_allocator<T>>;
using safe_string =
    std::basic_string<char, std::char_traits<char>, std::allocator<char>>;