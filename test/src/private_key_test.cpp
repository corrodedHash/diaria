#include <algorithm>
#include <cassert>
#include <print>

#include "crypto/secret_key.hpp"
#include "util.hpp"
auto main() -> int
{
  auto [pk, sk] = generate_keypair();
  auto stored = stored_secret_key::store(sk, "abc");
  auto extracted = stored.get_serialized_key();
  auto restored = stored_secret_key(extracted);
  auto restored_sk = restored.extract_key("abc");
  if (!std::ranges::equal(sk, restored_sk)) {
    print_byte_range(sk);
    std::print("\n");
    print_byte_range(restored_sk);
    return 1;
  }
}
