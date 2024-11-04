#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <string_view>

#include <unistd.h>

#include "common.hpp"
#include "crypto/secret_key.hpp"

auto read_password() -> std::string
{
  return {getpass("Enter password: ")};
}

auto load_symkey(const std::filesystem::path& file_path) -> symkey_t
{
  symkey_t symkey {};
  std::ifstream symkey_file((file_path).c_str(),
                            std::ios::in | std::ios::binary);
  symkey_file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
  symkey_file.read(make_signed_char(symkey.data()), symkey.size());
  return symkey;
}

auto load_pubkey(const std::filesystem::path& file_path) -> public_key_t
{
  public_key_t pubkey {};
  std::ifstream pubkey_file((file_path).c_str(),
                            std::ios::in | std::ios::binary);
  pubkey_file.exceptions(std::ifstream::badbit | std::ifstream::failbit);

  pubkey_file.read(make_signed_char(pubkey.data()), pubkey.size());
  return pubkey;
}

auto load_private_key(const std::filesystem::path& file_path,
                      std::string_view password) -> private_key_t
{
  stored_secret_key::serialized_key_t stored_private_key_raw {};
  std::ifstream private_key_file((file_path).c_str(),
                                 std::ios::in | std::ios::binary);
  private_key_file.exceptions(std::ifstream::badbit | std::ifstream::failbit);
  private_key_file.read(make_signed_char(stored_private_key_raw.data()),
                        stored_private_key_raw.size());
  stored_secret_key const stored_private_key(stored_private_key_raw);
  return stored_private_key.extract_key(password);
}
