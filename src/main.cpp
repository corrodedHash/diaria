#include <cstring>
#include <print>
#include <string_view>

#include "crypto/secret_key.hpp"

auto main() -> int
{
  auto [pk, sk] = generate_keypair();
  stored_secret_key::store(sk, "abc");
}
