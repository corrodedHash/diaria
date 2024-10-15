#include <algorithm>
#include <cassert>
#include <print>
#include <string_view>

#include "crypto/entry.hpp"

#include "common.hpp"
#include "crypto/secret_key.hpp"
#include "util.hpp"
auto main() -> int
{
  using namespace std::literals;
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
    std::print("\n");
    print_byte_range(dec);
    return 1;
  }
}
