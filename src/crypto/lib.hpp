#pragma once

#include <array>
#include <string_view>

#include <sodium.h>
#include <sodium/crypto_box_curve25519xchacha20poly1305.h>
#include <sodium/crypto_pwhash_scryptsalsa208sha256.h>
#include <sodium/crypto_secretbox_xchacha20poly1305.h>

class StoredSecretKey
{
  std::array<unsigned char,
             crypto_pwhash_scryptsalsa208sha256_SALTBYTES
                 + crypto_secretbox_xchacha20poly1305_NONCEBYTES
                 + crypto_secretbox_xchacha20poly1305_MACBYTES
                 + crypto_box_curve25519xchacha20poly1305_SECRETKEYBYTES>
      serialized_key;

  static auto generate(std::string_view password) -> StoredSecretKey;
};
