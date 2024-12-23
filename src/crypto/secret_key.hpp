#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

#include <sodium/crypto_box_curve25519xchacha20poly1305.h>
#include <sodium/crypto_pwhash_scryptsalsa208sha256.h>
#include <sodium/crypto_secretbox_xchacha20poly1305.h>

#include "safe_buffer.hpp"

template<class X>
struct array_to_const_span;
template<class X, std::size_t Size>
struct array_to_const_span<std::array<X, Size>>
{
  using result_t = std::span<const X, Size>;
};
template<std::size_t Size>
struct array_to_const_span<safe_array<Size>>
{
  using result_t = std::span<const unsigned char, Size>;
};
template<class X>
using array_to_const_span_t = array_to_const_span<X>::result_t;

using private_key_t =
    safe_array<crypto_box_curve25519xchacha20poly1305_SECRETKEYBYTES>;
using public_key_t =
    std::array<unsigned char,
               crypto_box_curve25519xchacha20poly1305_PUBLICKEYBYTES>;
using symkey_t = safe_array<crypto_secretbox_xchacha20poly1305_KEYBYTES>;

struct symkey_span_t
{
  array_to_const_span_t<symkey_t> element;
  explicit symkey_span_t(const symkey_t& s)
      : element(s.span())
  {
  }
};
struct public_key_span_t
{
  array_to_const_span_t<public_key_t> element;
};
struct private_key_span_t
{
  array_to_const_span_t<private_key_t> element;
  explicit private_key_span_t(const private_key_t& s)
      : element(s.span())
  {
  }
};

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
  [[nodiscard]] auto get_serialized_key() const -> serialized_key_t
  {
    return serialized_key;
  }
  static auto store(array_to_const_span_t<private_key_t> secret_key,
                    std::string_view password) -> stored_secret_key;
  explicit stored_secret_key(serialized_key_t data)
      : serialized_key(data)
  {
  }
  [[nodiscard]] auto extract_key(std::string_view password) const
      -> private_key_t;
};
