#pragma once
#include <span>
#include <vector>

#include <sodium/randombytes.h>

#include "secret_key.hpp"

auto symenc(const symkey_t& key, std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>;

auto symdec(const symkey_t& key, std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>;

auto asymenc(const public_key_t& key, std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>;

auto asymdec(const private_key_t& key,
             std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>;

auto encrypt(const symkey_t& symkey,
             const public_key_t& pubkey,
             std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>;

auto decrypt(const symkey_t& symkey,
             const private_key_t& private_key,
             std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>;

inline auto generate_symkey() -> symkey_t
{
  symkey_t symkey {};
  randombytes_buf(symkey.data(), symkey.size());
  return symkey;
}