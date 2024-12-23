#include <cstddef>
#include <cstdlib>
#include <ranges>
#include <stdexcept>

#include <sodium/utils.h>

template<std::size_t Size>
class safe_array
{
  std::span<unsigned char, Size> buffer;

public:
  explicit safe_array()
      : buffer(static_cast<unsigned char*>(sodium_malloc(Size)), Size)
  {
    if (buffer.data() == nullptr) {
      throw std::runtime_error("Could not allocate safe memory");
    }
  }

  safe_array(const safe_array&) = delete;
  auto operator=(const safe_array&) -> safe_array& = delete;
  safe_array(safe_array&&) = default;
  auto operator=(safe_array&&) -> safe_array& = default;

  ~safe_array() { sodium_free(buffer.data()); }

  [[nodiscard]] auto begin() const { return buffer.cbegin(); }
  [[nodiscard]] auto span() const { return buffer; }
  [[nodiscard]] auto end() const { return buffer.cend(); }
  [[nodiscard]] auto begin() { return buffer.begin(); }
  [[nodiscard]] auto end() { return buffer.end(); }
  [[nodiscard]] auto data() { return buffer.data(); }
  [[nodiscard]] auto data() const { return buffer.data(); }
  [[nodiscard]] auto size() const { return buffer.size(); }
};

static_assert(std::ranges::range<safe_array<5>>);
static_assert(std::ranges::random_access_range<safe_array<5>>);
static_assert(std::ranges::sized_range<safe_array<5>>);
