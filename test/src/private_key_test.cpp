#include <algorithm>
#include <cassert>
#include <print>

#include "crypto/secret_key.hpp"
auto main() -> int
{
  auto [pk, sk] = generate_keypair();
  auto stored = stored_secret_key::store(sk, "abc");
  auto extracted = stored.get_serialized_key();
  auto restored = stored_secret_key(extracted);
  auto restored_sk = restored.extract_key("abc");
  if (!std::ranges::equal(sk, restored_sk)) {
    const auto byte_printer = [](auto x) { std::print("{:02x}", x); };
    std::ranges::for_each(sk, byte_printer);
    std::print("\n");
    std::ranges::for_each(restored_sk, byte_printer);
    return 1;
  }
}
