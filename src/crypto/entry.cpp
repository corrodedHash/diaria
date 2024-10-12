#include <algorithm>
#include <ranges>
#include <vector>

#include "entry.hpp"

#include <sodium/crypto_box_curve25519xchacha20poly1305.h>
#include <sodium/crypto_scalarmult.h>
#include <sodium/crypto_secretbox_xchacha20poly1305.h>

#include "crypto/secret_key.hpp"

auto symenc(const symkey_t& key, std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>
{
  std::array<unsigned char, crypto_secretbox_xchacha20poly1305_NONCEBYTES>
      nonce {};
  randombytes_buf(nonce.data(), nonce.size());

  std::vector<unsigned char> output(
      plaintext.size() + crypto_secretbox_xchacha20poly1305_MACBYTES
          + nonce.size(),
      0);
  std::ranges::copy(nonce, output.begin());
  auto symmetric_encrypted =
      std::ranges::subrange(output.begin() + nonce.size(), output.end());

  if (crypto_secretbox_xchacha20poly1305_easy(symmetric_encrypted.data(),
                                              plaintext.data(),
                                              plaintext.size(),
                                              nonce.data(),
                                              key.data())
      != 0)
  {
    throw std::invalid_argument("Symmetric encryption failed");
  }
  return output;
}

auto symdec(const symkey_t& key, std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>
{
  auto nonce = std::ranges::subrange(
      ciphertext.begin(),
      ciphertext.begin() + crypto_secretbox_xchacha20poly1305_NONCEBYTES);
  auto text = std::ranges::subrange(
      ciphertext.begin() + crypto_secretbox_xchacha20poly1305_NONCEBYTES,
      ciphertext.end());
  auto plaintext = std::vector<unsigned char>(
      text.size() - crypto_secretbox_xchacha20poly1305_MACBYTES);
  if (crypto_secretbox_xchacha20poly1305_open_easy(
          plaintext.data(), text.data(), text.size(), nonce.data(), key.data())
      != 0)
  {
    throw std::invalid_argument("Symmetric decryption failed");
  }
  return plaintext;
}

auto asymenc(const public_key_t& key, std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>
{
  std::vector<unsigned char> output(
      plaintext.size() + crypto_box_curve25519xchacha20poly1305_SEALBYTES, 0);
  if (crypto_box_curve25519xchacha20poly1305_seal(
          output.data(), plaintext.data(), plaintext.size(), key.data())
      != 0)
  {
    throw std::invalid_argument("Asymmetric encryption failed");
  }
  return output;
}

auto asymdec(const private_key_t& key,
             std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>
{
  std::vector<unsigned char> output(
      ciphertext.size() - crypto_box_curve25519xchacha20poly1305_SEALBYTES, 0);
  public_key_t pk;
  crypto_scalarmult_base(pk.data(), key.data());
  if (crypto_box_curve25519xchacha20poly1305_seal_open(output.data(),
                                                       ciphertext.data(),
                                                       ciphertext.size(),
                                                       pk.data(),
                                                       key.data())
      != 0)
  {
    throw std::invalid_argument("Asymmetric decryption failed");
  }
  return output;
}

auto encrypt(const symkey_t& symkey,
             const public_key_t& pubkey,
             std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>
{
  auto symmetric_encrypted = symenc(symkey, plaintext);
  auto asymmetric_encrypted = asymenc(pubkey, symmetric_encrypted);
  return asymmetric_encrypted;
}
auto decrypt(const symkey_t& symkey,
             const private_key_t& private_key,
             std::span<const unsigned char> ciphertext)
    -> std::vector<unsigned char>
{
  auto asymmetric_decrypted = asymdec(private_key, ciphertext);
  auto symmetric_decrypted = symdec(symkey, asymmetric_decrypted);
  return symmetric_decrypted;
}