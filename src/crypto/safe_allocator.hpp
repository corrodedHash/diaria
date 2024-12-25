
#include <cstddef>
#include <cstdlib>
#include <print>

auto diaria_sodium_malloc(std::size_t size) -> void*;
void diaria_sodium_free(void* pointer);

template<typename T>
struct sodium_allocator : public std::allocator<T>
{
  using value_type = T;

  sodium_allocator() = default;

  [[nodiscard]] auto allocate(std::size_t n) -> sodium_allocator::value_type*
  {
    if (n > std::numeric_limits<std::size_t>::max() / sizeof(value_type)) {
      throw std::bad_array_new_length();
    }
    auto* allocated_address =
        static_cast<value_type*>(diaria_sodium_malloc(n * sizeof(value_type)));
    if (allocated_address != nullptr) {
      return allocated_address;
    }

    throw std::bad_alloc();
  }
  void deallocate(sodium_allocator::value_type* pointer, std::size_t /*n*/)
  {
    diaria_sodium_free(pointer);
  }

  sodium_allocator(sodium_allocator&&) = default;
  auto operator=(const sodium_allocator&) -> sodium_allocator& = default;
  auto operator=(sodium_allocator&&) -> sodium_allocator& = default;
  constexpr sodium_allocator(const sodium_allocator&) noexcept = default;
  ~sodium_allocator() = default;

private:
};

template<class T, class U>
auto operator==(const sodium_allocator<T>& /*one*/,
                const sodium_allocator<U>& /*two*/) -> bool
{
  return true;
}

template<class T, class U>
auto operator!=(const sodium_allocator<T>& /*one*/,
                const sodium_allocator<U>& /*two*/) -> bool
{
  return false;
}
