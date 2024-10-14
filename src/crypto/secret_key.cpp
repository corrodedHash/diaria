#include <algorithm>
#include <array>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "secret_key.hpp"

#include <sodium/crypto_pwhash_scryptsalsa208sha256.h>
#include <sodium/crypto_secretbox_xchacha20poly1305.h>
#include <sodium/crypto_box_curve25519xchacha20poly1305.h>
#include <sodium/randombytes.h>

namespace
{

template<class Salt>
auto derive_key(std::string_view password, const Salt& salt) -> symkey_t
{
  symkey_t key {};

  if (crypto_pwhash_scryptsalsa208sha256(
          key.begin(),
          key.size(),
          password.begin(),
          password.length(),
          salt.begin(),
          crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE,
          crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE)
      != 0)
  {
    throw std::invalid_argument(
        "Key derivation failed, probably out of memory");
  }
  return key;
}
}  // namespace

auto stored_secret_key::store(array_to_const_span_t<private_key_t> secret_key,
                              std::string_view password) -> stored_secret_key
{
  salt_t salt {};

  randombytes_buf(salt.data(), salt.size());

  auto key = derive_key(password, salt);

  std::array<unsigned char, crypto_secretbox_xchacha20poly1305_NONCEBYTES>
      nonce {};
  randombytes_buf(nonce.data(), nonce.size());
  encrypted_private_key_t output {};

  if (crypto_secretbox_xchacha20poly1305_easy(output.data(),
                                              secret_key.data(),
                                              secret_key.size(),
                                              nonce.data(),
                                              key.data())
      != 0)
  {
    throw std::invalid_argument("Key encryption failed");
  }

  auto output_parts = {
      std::ranges::subrange(salt.begin(), salt.end()),
      std::ranges::subrange(nonce.begin(), nonce.end()),
      std::ranges::subrange(output.begin(), output.end()),
  };
  stored_secret_key::serialized_key_t serialized_key {};
  std::ranges::copy(output_parts | std::ranges::views::join,
                    serialized_key.begin());

  return stored_secret_key(serialized_key);
}

auto stored_secret_key::extract_key(std::string_view password) const
    -> private_key_t
{
  auto salt = std::ranges::subrange(
      serialized_key.begin(),
      serialized_key.begin() + crypto_pwhash_scryptsalsa208sha256_SALTBYTES);
  auto nonce = std::ranges::subrange(
      serialized_key.begin() + crypto_pwhash_scryptsalsa208sha256_SALTBYTES,
      serialized_key.begin() + crypto_pwhash_scryptsalsa208sha256_SALTBYTES
          + crypto_secretbox_xchacha20poly1305_NONCEBYTES);
  auto ciphertext = std::ranges::subrange(
      serialized_key.begin() + crypto_pwhash_scryptsalsa208sha256_SALTBYTES
          + crypto_secretbox_xchacha20poly1305_NONCEBYTES,
      serialized_key.end());
  auto key = derive_key(password, salt);
  private_key_t private_key {};
  if (crypto_secretbox_xchacha20poly1305_open_easy(private_key.data(),
                                                   ciphertext.data(),
                                                   ciphertext.size(),
                                                   nonce.data(),
                                                   key.data())
      != 0)
  {
    throw std::invalid_argument("Key decryption failed");
  }
  return private_key;
}

auto generate_keypair() -> std::pair<public_key_t, private_key_t>
{
  public_key_t recipient_pk {};
  private_key_t recipient_sk {};
  if (crypto_box_curve25519xchacha20poly1305_keypair(recipient_pk.data(),
                                                     recipient_sk.data())
      != 0)
  {
  }
  return std::make_pair(recipient_pk, recipient_sk);
}
