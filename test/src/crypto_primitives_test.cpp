#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include "common.hpp"
#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"
using namespace std::literals;

#include "util.hpp"

TEST_CASE("Symmetric key encryption and decryption")

{
  auto symkey = generate_symkey();
  auto important_data = "This is a secret message"sv;
  auto important_data_span = std::span<const unsigned char>(
      make_unsigned_char(important_data.data()), important_data.size());
  auto enc = symenc(symkey_span_t {symkey}, important_data_span);
  auto dec = symdec(symkey_span_t {symkey}, enc);
  REQUIRE_THAT(dec, EqualsRangeMatcher(important_data_span));
}

TEST_CASE("Asymmetric key encryption and decryption")
{
  auto [pk, sk] = generate_keypair();

  auto important_data = "This is a secret message"sv;
  auto important_data_span = std::span<const unsigned char>(
      make_unsigned_char(important_data.data()), important_data.size());
  auto enc = asymenc(public_key_span_t {pk}, important_data_span);
  auto dec = asymdec(private_key_span_t {sk}, enc);
  REQUIRE_THAT(dec, EqualsRangeMatcher(important_data_span));
}

TEST_CASE("Entry encryption and decryption")
{
  auto [pk, sk] = generate_keypair();
  auto symkey = generate_symkey();

  auto important_data = "This is a secret message"sv;
  auto important_data_span = std::span<const unsigned char>(
      make_unsigned_char(important_data.data()), important_data.size());
  auto enc = encrypt(
      symkey_span_t {symkey}, public_key_span_t {pk}, important_data_span);
  auto dec = decrypt(symkey_span_t {symkey}, private_key_span_t {sk}, enc);
  REQUIRE_THAT(dec, EqualsRangeMatcher(important_data_span));
}
