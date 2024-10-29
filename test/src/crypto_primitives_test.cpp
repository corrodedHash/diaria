#include <algorithm>
#include <iterator>
#include <numeric>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>

#include "common.hpp"
#include "crypto/compress.hpp"
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

TEST_CASE("Small compression and decompression")
{
  std::array<unsigned char, 6> input = {0xAC, 0X1D, 0XDE, 0XAD, 0XBE, 0XEF};
  decltype(input) copied_input {};
  std::copy(input.begin(), input.end(), copied_input.begin());

  auto c = compress(copied_input);
  auto d = decompress(c);
  REQUIRE_THAT(d, EqualsRange(input));
}
TEST_CASE("Large compression and decompression")
{
  std::vector<unsigned char> input;
  input.resize(100'000);
  std::ranges::iota(input, 0);
  REQUIRE(input[100] == 100);

  decltype(input) copied_input {};
  std::copy(input.begin(), input.end(), std::back_inserter(copied_input));

  auto c = compress(copied_input);
  auto d = decompress(c);
  REQUIRE_THAT(d, EqualsRange(input));
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
