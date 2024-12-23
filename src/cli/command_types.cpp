#include <fstream>
#include <string>

#include "./command_types.hpp"

#include "util/char.hpp"
#include "crypto/secret_key.hpp"
#include "cli/key_management.hpp"

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

}  // namespace

auto file_entry_encryptor_initializer::init() -> entry_encryptor
{
  auto symkey = load_file<symkey_t>(paths.get_symkey_path());
  auto public_key = load_file<public_key_t>(paths.get_pubkey_path());
  return {std::move(symkey), public_key};
}

auto stored_password_provider::provide() -> std::string
{
  return password;
}

auto stdin_password_provider::provide() -> std::string
{
  return read_password();
}
auto file_password_provider::provide() -> std::string
{
  std::ifstream passfile(password_file);
  if (passfile.fail()) {
    throw std::runtime_error("Could not open password file");
  }
  std::string password {};
  std::getline(passfile, password);
  return password;
}

auto file_entry_decryptor_initializer::init() -> entry_decryptor
{
  auto symkey = load_file<symkey_t>(paths.get_symkey_path());
  auto private_key_raw = load_file<stored_secret_key::serialized_key_t>(
      paths.get_private_key_path());
  const stored_secret_key pkey(private_key_raw);
  auto private_key = pkey.extract_key(pp->provide());
  return {std::move(symkey), std::move(private_key)};
}