#include "crypto/entry.hpp"

#include <catch2/catch_test_macros.hpp>

#include "common.hpp"
#include "crypto/secret_key.hpp"
#include "util.hpp"

TEST_CASE("Check that entry gets recorded and can be decrypted")
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
  REQUIRE_THAT(dec, equals_range(important_data_span));
}