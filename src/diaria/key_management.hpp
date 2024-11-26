
#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "crypto/secret_key.hpp"

auto read_password() -> std::string;

auto load_symkey(const std::filesystem::path& file_path) -> symkey_t;

auto load_pubkey(const std::filesystem::path& file_path) -> public_key_t;

auto load_private_key(const std::filesystem::path& file_path,
                      std::string_view password) -> private_key_t;

struct key_repo_paths_t
{
  std::filesystem::path root;
  std::optional<std::string> private_key_password;
  [[nodiscard]] auto get_symkey_path() const { return root / "key.sym"; }
  [[nodiscard]] auto get_pubkey_path() const { return root / "key.pub"; }
  [[nodiscard]] auto get_private_key_path() const { return root / "key.key"; }
};

class entry_decryptor
{
  symkey_t symkey;
  private_key_t private_key;

public:
  entry_decryptor(symkey_t symkey, private_key_t private_key)
      : symkey(symkey)
      , private_key(private_key)
  {
  }
  [[nodiscard]] auto decrypt(std::span<const unsigned char> filebytes) const
      -> std::vector<unsigned char>;
};

class entry_encryptor
{
  symkey_t symkey;
  public_key_t public_key;

public:
  entry_encryptor(symkey_t symkey, public_key_t public_key)
      : symkey(symkey)
      , public_key(public_key)
  {
  }
  [[nodiscard]] auto encrypt(std::span<const unsigned char> filebytes) const
      -> std::vector<unsigned char>;
};