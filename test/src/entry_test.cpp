#include <algorithm>
#include <cassert>
#include <print>
#include <string_view>

#include "crypto/entry.hpp"

#include "crypto/secret_key.hpp"
#include "util.hpp"
auto main() -> int
{
  using namespace std::literals;
  auto [pk, sk] = generate_keypair();
  auto symkey = generate_symkey();

  auto important_data = "This is a secret message"sv;
  auto important_data_span = std::span<const unsigned char>(
      reinterpret_cast<const unsigned char*>(important_data.data()),
      important_data.size());
  auto enc = encrypt(symkey, pk, important_data_span);
  auto dec = decrypt(symkey, sk, enc);
  if (!std::ranges::equal(dec, important_data_span)) {
    print_byte_range(important_data_span);
    std::print("\n");
    print_byte_range(dec);
    return 1;
  }
}
