#pragma once
#include <span>
#include <vector>

#include <sodium/randombytes.h>

#include "secret_key.hpp"

auto symenc(array_to_const_span_t<symkey_t> key,
            std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>;

auto symdec(array_to_const_span_t<symkey_t> key,
            std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>;

auto asymenc(array_to_const_span_t<public_key_t> key,
             std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>;

auto asymdec(array_to_const_span_t<private_key_t> key,
             std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>;

auto encrypt(array_to_const_span_t<symkey_t> symkey,
             array_to_const_span_t<public_key_t> pubkey,
             std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>;

auto decrypt(array_to_const_span_t<symkey_t> symkey,
             array_to_const_span_t<private_key_t> private_key,
             std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>;

inline auto generate_symkey() -> symkey_t
{
  symkey_t symkey {};
  randombytes_buf(symkey.data(), symkey.size());
  return symkey;
}