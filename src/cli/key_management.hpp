
#pragma once
#include <filesystem>
#include <vector>

#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"

auto read_password() -> safe_string;

struct key_repo_paths_t
{
  std::filesystem::path root;
  [[nodiscard]] auto get_symkey_path() const { return root / "key.sym"; }
  [[nodiscard]] auto get_pubkey_path() const { return root / "key.pub"; }
  [[nodiscard]] auto get_private_key_path() const { return root / "key.key"; }
};

struct entry_decryptor
{
  symkey_t symkey;
  private_key_t private_key;

  [[nodiscard]] auto decrypt(std::span<const unsigned char> filebytes) const
      -> safe_vector<unsigned char>
  {
    return ::decrypt(
        symkey_span_t {symkey}, private_key_span_t {private_key}, filebytes);
  }
};

struct entry_encryptor
{
  symkey_t symkey;
  public_key_t public_key;

  [[nodiscard]] auto encrypt(std::span<const unsigned char> filebytes) const
      -> std::vector<unsigned char>
  {
    return ::encrypt(
        symkey_span_t {symkey}, public_key_span_t {public_key}, filebytes);
  }
};