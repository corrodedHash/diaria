#include <algorithm>
#include <cassert>
#include <print>
#include <stdexcept>
#include <string_view>

#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"
using namespace std::literals;

auto test_sym()
{
  auto symkey = generate_symkey();
  auto important_data = "This is a secret message"sv;
  auto important_data_span = std::span<const unsigned char>(
      reinterpret_cast<const unsigned char*>(important_data.data()),
      important_data.size());
  auto enc = symenc(symkey, important_data_span);
  auto dec = symdec(symkey, enc);
  if (!std::ranges::equal(dec, important_data_span)) {
    const auto byte_printer = [](auto x) { std::print(stderr, "{:02x}", x); };
    std::ranges::for_each(important_data_span, byte_printer);
    std::print(stderr, "\n");
    std::ranges::for_each(dec, byte_printer);
    throw std::runtime_error("Symmetric test failed");
  }
}

auto test_asym()
{
  auto [pk, sk] = generate_keypair();

  auto important_data = "This is a secret message"sv;
  auto important_data_span = std::span<const unsigned char>(
      reinterpret_cast<const unsigned char*>(important_data.data()),
      important_data.size());
  auto enc = asymenc(pk, important_data_span);
  auto dec = asymdec(sk, enc);
  if (!std::ranges::equal(dec, important_data_span)) {
    const auto byte_printer = [](auto x) { std::print(stderr, "{:02x}", x); };
    std::ranges::for_each(important_data_span, byte_printer);
    std::print(stderr, "\n");
    std::ranges::for_each(dec, byte_printer);
    throw std::runtime_error("Asymmetric test failed");
  }
}

auto main() -> int
{
  test_sym();
  test_asym();
}
