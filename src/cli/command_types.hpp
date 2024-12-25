#pragma once
#include <filesystem>
#include <memory>
#include <utility>

#include "cli/key_management.hpp"

struct input_file_t
{
  std::filesystem::path p;
};

struct output_file_t
{
  std::filesystem::path p;
};

struct repo_path_t
{
  std::filesystem::path repo;
};

struct entry_decryptor_initializer
{
  virtual auto init() -> entry_decryptor = 0;
  virtual ~entry_decryptor_initializer() = default;
};

struct entry_encryptor_initializer
{
  virtual auto init() -> entry_encryptor = 0;
  virtual ~entry_encryptor_initializer() = default;
};

struct file_entry_encryptor_initializer : entry_encryptor_initializer
{
  key_repo_paths_t paths;
  explicit file_entry_encryptor_initializer(key_repo_paths_t in_paths)
      : paths(std::move(in_paths))
  {
  }
  auto init() -> entry_encryptor override;
};

struct password_provider
{
  virtual auto provide() -> safe_string = 0;
  virtual ~password_provider() = default;
};

struct stdin_password_provider : password_provider
{
  auto provide() -> safe_string override;
};
struct stored_password_provider : password_provider
{
  safe_string password;
  explicit stored_password_provider(safe_string in_password)
      : password(std::move(in_password))
  {
  }
  auto provide() -> safe_string override;
};
struct file_password_provider : password_provider
{
  std::filesystem::path password_file;
  explicit file_password_provider(std::filesystem::path in_password_file)
      : password_file(std::move(in_password_file))
  {
  }
  auto provide() -> safe_string override;
};

struct file_entry_decryptor_initializer : entry_decryptor_initializer
{
  std::unique_ptr<password_provider> pp;
  key_repo_paths_t paths;
  file_entry_decryptor_initializer(std::unique_ptr<password_provider>&& in_pp,
                                   key_repo_paths_t in_paths)
      : pp(std::move(in_pp))
      , paths(std::move(in_paths))
  {
  }
  auto init() -> entry_decryptor override;
};
