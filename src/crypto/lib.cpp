#include <algorithm>
#include <ranges>

#include "./lib.hpp"
auto StoredSecretKey::generate(std::string_view password) -> StoredSecretKey
{
  std::array<unsigned char,
             crypto_box_curve25519xchacha20poly1305_PUBLICKEYBYTES>
      recipient_pk {};
  std::array<unsigned char,
             crypto_box_curve25519xchacha20poly1305_SECRETKEYBYTES>
      recipient_sk {};
  if (crypto_box_curve25519xchacha20poly1305_keypair(recipient_pk.begin(),
                                                     recipient_sk.begin())
      != 0)
  {
  }

  std::array<unsigned char, crypto_pwhash_scryptsalsa208sha256_SALTBYTES>
      salt {};
  std::array<unsigned char, crypto_secretbox_xchacha20poly1305_KEYBYTES> key {};

  randombytes_buf(salt.begin(), salt.size());

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
    /* out of memory */
  }

  std::array<unsigned char, crypto_secretbox_xchacha20poly1305_NONCEBYTES>
      nonce {};
  randombytes_buf(nonce.begin(), nonce.size());
  std::array<unsigned char,
             crypto_box_curve25519xchacha20poly1305_SECRETKEYBYTES
                 + crypto_secretbox_xsalsa20poly1305_MACBYTES>
      output {};

  if (crypto_secretbox_xchacha20poly1305_easy(output.begin(),
                                              recipient_sk.begin(),
                                              recipient_sk.size(),
                                              nonce.begin(),
                                              key.begin())
      != 0)
  {
  }

  auto q = {
      std::ranges::subrange(salt.begin(), salt.end()),
      std::ranges::subrange(nonce.begin(), nonce.end()),
      std::ranges::subrange(key.begin(), key.end()),
  };
  StoredSecretKey result {};
  std::ranges::copy(q | std::ranges::views::join,
                    result.serialized_key.begin());
  return result;
}
