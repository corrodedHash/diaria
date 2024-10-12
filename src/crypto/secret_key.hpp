#pragma once

#include <array>
#include <string_view>
#include <utility>

#include <sodium/crypto_box_curve25519xchacha20poly1305.h>
#include <sodium/crypto_pwhash_scryptsalsa208sha256.h>
#include <sodium/crypto_secretbox_xchacha20poly1305.h>

using private_key_t =
    std::array<unsigned char,
               crypto_box_curve25519xchacha20poly1305_SECRETKEYBYTES>;
using public_key_t =
    std::array<unsigned char,
               crypto_box_curve25519xchacha20poly1305_PUBLICKEYBYTES>;
using symkey_t =
    std::array<unsigned char, crypto_secretbox_xchacha20poly1305_KEYBYTES>;
using salt_t =
    std::array<unsigned char, crypto_pwhash_scryptsalsa208sha256_SALTBYTES>;
using encrypted_private_key_t =
    std::array<unsigned char,
               crypto_box_curve25519xchacha20poly1305_SECRETKEYBYTES
                   + crypto_secretbox_xchacha20poly1305_MACBYTES>;

auto generate_keypair() -> std::pair<public_key_t, private_key_t>;

class stored_secret_key
{
public:
  using serialized_key_t =
      std::array<unsigned char,
                 crypto_pwhash_scryptsalsa208sha256_SALTBYTES
                     + crypto_secretbox_xchacha20poly1305_NONCEBYTES
                     + crypto_secretbox_xchacha20poly1305_MACBYTES
                     + crypto_box_curve25519xchacha20poly1305_SECRETKEYBYTES>;

private:
  serialized_key_t serialized_key;

public:
  auto get_serialized_key() const -> serialized_key_t { return serialized_key; }
  static auto store(private_key_t secret_key,
                    std::string_view password) -> stored_secret_key;
  explicit stored_secret_key(serialized_key_t data)
      : serialized_key(data)
  {
  }
  auto extract_key(std::string_view password) const -> private_key_t;
};
