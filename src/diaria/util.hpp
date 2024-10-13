
#pragma once
#include <filesystem>

#include "crypto/secret_key.hpp"

auto read_password() -> std::string;

auto load_symkey(const std::filesystem::path& file_path) -> symkey_t;

auto load_pubkey(const std::filesystem::path& file_path) -> public_key_t;

auto load_private_key(const std::filesystem::path& file_path,
                      std::string_view password) -> private_key_t;

struct key_path_t
{
  std::filesystem::path root;
  auto get_symkey_path() const { return root / "key.sym"; }
  auto get_pubkey_path() const { return root / "key.pub"; }
  auto get_private_key_path() const { return root / "key.key"; }
};