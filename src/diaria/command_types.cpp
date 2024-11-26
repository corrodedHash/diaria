#include <fstream>
#include <tuple>

#include "./command_types.hpp"

#include "common.hpp"
#include "crypto/secret_key.hpp"
#include "diaria/key_management.hpp"

namespace
{
template<typename T>
auto load_file(const std::filesystem::path& file_path) -> T
{
  T key {};
  std::ifstream key_file((file_path).c_str(), std::ios::in | std::ios::binary);
  key_file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
  key_file.read(make_signed_char(key.data()), key.size());
  return key;
}

auto load_decrypt_files(const key_repo_paths_t& repo)
{
  auto symkey = load_file<symkey_t>(repo.get_symkey_path());
  auto private_key = load_file<stored_secret_key::serialized_key_t>(
      repo.get_private_key_path());
  return std::make_tuple(symkey, private_key);
}
}  // namespace

auto file_entry_encryptor_initializer::init() -> entry_encryptor
{
  auto symkey = load_file<symkey_t>(paths.get_symkey_path());
  auto public_key = load_file<public_key_t>(paths.get_pubkey_path());
  return {symkey, public_key};
}

auto stored_password_entry_decryptor_initializer::init() -> entry_decryptor
{
  auto [symkey, private_key_raw] = load_decrypt_files(paths);
  stored_secret_key pkey(private_key_raw);
  auto private_key = pkey.extract_key(password);
  return {symkey, private_key};
}

auto prompt_password_entry_decryptor_initializer::init() -> entry_decryptor
{
  auto [symkey, private_key_raw] = load_decrypt_files(paths);
  auto password = read_password();
  stored_secret_key pkey(private_key_raw);
  auto private_key = pkey.extract_key(password);
  return {symkey, private_key};
}