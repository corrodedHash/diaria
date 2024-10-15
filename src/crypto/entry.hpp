#pragma once
#include <span>
#include <vector>

#include <sodium/randombytes.h>

#include "secret_key.hpp"

auto symenc(symkey_span_t key, std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>;

auto symdec(symkey_span_t key, std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>;

auto asymenc(public_key_span_t key, std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>;

auto asymdec(private_key_span_t key, std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>;

auto encrypt(symkey_span_t symkey,
             public_key_span_t pubkey,
             std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>;

auto decrypt(symkey_span_t symkey,
             private_key_span_t private_key,
             std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>;

inline auto generate_symkey() -> symkey_t
{
  symkey_t symkey {};
  randombytes_buf(symkey.data(), symkey.size());
  return symkey;
}