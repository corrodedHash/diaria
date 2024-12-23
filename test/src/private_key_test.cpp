#include <catch2/catch_test_macros.hpp>

#include "crypto/secret_key.hpp"
#include "util.hpp"

TEST_CASE("Private key serialization")
{
  auto [pk, sk] = generate_keypair();
  auto stored = stored_secret_key::store(sk.span(), "abc");
  auto extracted = stored.get_serialized_key();
  auto restored = stored_secret_key(extracted);
  auto restored_sk = restored.extract_key("abc");
  REQUIRE_THAT(sk, equals_range(restored_sk));
}
