
#pragma once
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "crypto/secret_key.hpp"

auto read_password() -> std::string;

auto load_symkey(const std::filesystem::path& file_path) -> symkey_t;

auto load_pubkey(const std::filesystem::path& file_path) -> public_key_t;

auto load_private_key(const std::filesystem::path& file_path,
                      std::string_view password) -> private_key_t;

struct key_repo_t
{
  std::filesystem::path root;
  std::optional<std::string> private_key_password;
  [[nodiscard]] auto get_symkey_path() const { return root / "key.sym"; }
  [[nodiscard]] auto get_pubkey_path() const { return root / "key.pub"; }
  [[nodiscard]] auto get_private_key_path() const { return root / "key.key"; }
};

