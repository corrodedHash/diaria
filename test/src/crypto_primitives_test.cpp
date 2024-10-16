#include <algorithm>
#include <cassert>
#include <print>
#include <stdexcept>
#include <string_view>

#include "common.hpp"
#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"
using namespace std::literals;

#include "util.hpp"

auto test_sym()
{
  auto symkey = generate_symkey();
  auto important_data = "This is a secret message"sv;
  auto important_data_span = std::span<const unsigned char>(
      make_unsigned_char(important_data.data()), important_data.size());
  auto enc = symenc(symkey_span_t {symkey}, important_data_span);
  auto dec = symdec(symkey_span_t {symkey}, enc);
  if (!std::ranges::equal(dec, important_data_span)) {
    print_byte_range(important_data_span);
    std::print(stderr, "\n");
    print_byte_range(dec);
    throw std::runtime_error("Symmetric test failed");
  }
}

auto test_asym()
{
  auto [pk, sk] = generate_keypair();

  auto important_data = "This is a secret message"sv;
  auto important_data_span = std::span<const unsigned char>(
      make_unsigned_char(important_data.data()), important_data.size());
  auto enc = asymenc(public_key_span_t {pk}, important_data_span);
  auto dec = asymdec(private_key_span_t {sk}, enc);
  if (!std::ranges::equal(dec, important_data_span)) {
    print_byte_range(important_data_span);
    std::print(stderr, "\n");
    print_byte_range(dec);
    throw std::runtime_error("Asymmetric test failed");
  }
}

auto test_complete()
{
  auto [pk, sk] = generate_keypair();
  auto symkey = generate_symkey();

  auto important_data = "This is a secret message"sv;
  auto important_data_span = std::span<const unsigned char>(
      make_unsigned_char(important_data.data()), important_data.size());
  auto enc = encrypt(
      symkey_span_t {symkey}, public_key_span_t {pk}, important_data_span);
  auto dec = decrypt(symkey_span_t {symkey}, private_key_span_t {sk}, enc);
  if (!std::ranges::equal(dec, important_data_span)) {
    print_byte_range(important_data_span);
    std::print(stderr, "\n");
    print_byte_range(dec);
    throw std::runtime_error("Asymmetric test failed");
  }
}

auto main() -> int
{
  test_sym();
  test_asym();
  test_complete();
}
