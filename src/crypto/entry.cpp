#include <algorithm>
#include <array>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

#include "entry.hpp"

#include <sodium/crypto_box_curve25519xchacha20poly1305.h>
#include <sodium/crypto_scalarmult.h>
#include <sodium/crypto_secretbox_xchacha20poly1305.h>
#include <sodium/randombytes.h>

#include "compress.hpp"
#include "crypto/secret_key.hpp"

auto symenc(symkey_span_t key, std::span<const unsigned char> plaintext)
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
                                              key.element.data())
      != 0)
  {
    throw std::invalid_argument("Symmetric encryption failed");
  }
  return output;
}

auto symdec(symkey_span_t key, std::span<const unsigned char> ciphertext)
    -> safe_vector<unsigned char>
{
  auto nonce = std::ranges::subrange(
      ciphertext.begin(),
      ciphertext.begin() + crypto_secretbox_xchacha20poly1305_NONCEBYTES);
  auto text = std::ranges::subrange(
      ciphertext.begin() + crypto_secretbox_xchacha20poly1305_NONCEBYTES,
      ciphertext.end());
  safe_vector<unsigned char> plaintext(
      text.size() - crypto_secretbox_xchacha20poly1305_MACBYTES);
  if (crypto_secretbox_xchacha20poly1305_open_easy(plaintext.data(),
                                                   text.data(),
                                                   text.size(),
                                                   nonce.data(),
                                                   key.element.data())
      != 0)
  {
    throw std::invalid_argument("Symmetric decryption failed");
  }
  return plaintext;
}

auto asymenc(public_key_span_t key, std::span<const unsigned char> plaintext)
    -> std::vector<unsigned char>
{
  std::vector<unsigned char> output(
      plaintext.size() + crypto_box_curve25519xchacha20poly1305_SEALBYTES, 0);
  if (crypto_box_curve25519xchacha20poly1305_seal(
          output.data(), plaintext.data(), plaintext.size(), key.element.data())
      != 0)
  {
    throw std::invalid_argument("Asymmetric encryption failed");
  }
  return output;
}

auto asymdec(private_key_span_t key, std::span<const unsigned char> ciphertext)
    -> safe_vector<unsigned char>
{
  safe_vector<unsigned char> output(
      ciphertext.size() - crypto_box_curve25519xchacha20poly1305_SEALBYTES, 0);
  public_key_t pubkey;
  crypto_scalarmult_base(pubkey.data(), key.element.data());
  if (crypto_box_curve25519xchacha20poly1305_seal_open(output.data(),
                                                       ciphertext.data(),
                                                       ciphertext.size(),
                                                       pubkey.data(),
                                                       key.element.data())
      != 0)
  {
    throw std::invalid_argument("Asymmetric decryption failed");
  }
  return output;
}

constexpr std::array<unsigned char, 6> magictag = {
    'D', 'I', 'A', 'R', 'I', 'A'};

constexpr unsigned char current_diaria_version = 0;
auto encrypt(symkey_span_t symkey,
             public_key_span_t pubkey,
             std::span<const unsigned char> filebytes)
    -> std::vector<unsigned char>
{
  auto compressed = compress(filebytes);
  auto asymmetric_encrypted = asymenc(pubkey, compressed);
  auto symmetric_encrypted = symenc(symkey, asymmetric_encrypted);
  symmetric_encrypted.insert(symmetric_encrypted.begin(),
                             current_diaria_version);
  static_assert(std::cbegin(magictag) != nullptr);

  /* -Wnull-dereference gets confused here */
  // symmetric_encrypted.insert(
  //     symmetric_encrypted.begin(), std::cbegin(magictag),
  //     std::cend(magictag));

  std::ranges::copy(magictag, std::back_inserter(symmetric_encrypted));
  std::ranges::rotate(std::begin(symmetric_encrypted),
                      std::end(symmetric_encrypted) - magictag.size(),
                      std::end(symmetric_encrypted));
  return symmetric_encrypted;
}
auto decrypt(symkey_span_t symkey,
             private_key_span_t private_key,
             std::span<const unsigned char> filebytes)
    -> safe_vector<unsigned char>
{
  if (!std::equal(magictag.begin(), magictag.end(), filebytes.begin())) {
    throw std::runtime_error("Decrypting file which is not a diaria entry");
  }
  const auto version = filebytes.subspan(magictag.size())[0];
  if (version > current_diaria_version) {
    throw std::runtime_error("Unknown diaria entry version");
  }

  auto plaintext = filebytes.subspan(magictag.size() + 1);
  auto symmetric_decrypted = symdec(symkey, plaintext);
  auto asymmetric_decrypted = asymdec(private_key, symmetric_decrypted);
  auto decompressed = decompress(asymmetric_decrypted);
  return decompressed;
}
