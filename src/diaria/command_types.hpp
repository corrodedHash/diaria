#pragma once
#include <filesystem>

#include "diaria/key_management.hpp"

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
  auto init() -> entry_encryptor override;
};

struct stored_password_entry_decryptor_initializer : entry_decryptor_initializer
{
  key_repo_paths_t paths;
  std::string password;
  auto init() -> entry_decryptor override;
};

struct prompt_password_entry_decryptor_initializer : entry_decryptor_initializer
{
  key_repo_paths_t paths;
  auto init() -> entry_decryptor override;
};