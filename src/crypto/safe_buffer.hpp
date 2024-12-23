#include <cstddef>
#include <cstdlib>
#include <ranges>

auto diaria_sodium_malloc(std::size_t size) -> void*;
void diaria_sodium_free(void* pointer);

template<std::size_t Size>
class safe_array
{
  std::span<unsigned char, Size> buffer;

public:
  safe_array()
      : buffer(static_cast<unsigned char*>(diaria_sodium_malloc(Size)), Size)
  {
    if (buffer.data() == nullptr) {
      throw std::runtime_error("Could not allocate safe memory");
    }
  }
  ~safe_array() { diaria_sodium_free(buffer.data()); }
  safe_array(const safe_array&) = delete;
  auto operator=(const safe_array&) -> safe_array& = delete;
  safe_array(safe_array&& other) noexcept
      : buffer(other.buffer)
  {
    other.buffer = std::span<unsigned char, Size>(
        static_cast<unsigned char*>(nullptr), Size);
  }
  auto operator=(safe_array&& other) noexcept -> safe_array&
  {
    buffer = other.buffer;
    other.buffer = std::span<unsigned char, Size>(
        static_cast<unsigned char*>(nullptr), Size);
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

static_assert(std::ranges::range<safe_array<5>>);
static_assert(std::ranges::random_access_range<safe_array<5>>);
static_assert(std::ranges::sized_range<safe_array<5>>);
