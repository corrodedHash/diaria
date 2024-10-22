#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "./commands.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

#include "./util.hpp"
#include "common.hpp"
#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"

void setup_db(const key_repo_t& keypath)
{
  const auto [pk, sk] = generate_keypair();
  const auto symkey = generate_symkey();

  std::error_code dir_creation_error {};
  std::filesystem::create_directories(keypath.root, dir_creation_error);
  if (dir_creation_error) {
    throw std::runtime_error(std::format("Could not create directories: {}",
                                         dir_creation_error.message()));
  };
  const auto password =
      keypath.private_key_password
          .or_else([]() { return std::optional(read_password()); })
          .value();
  const auto stored_key = stored_secret_key::store(std::span(sk), password);
  std::ofstream keyfile(keypath.get_private_key_path(),
                        std::ios::out | std::ios::binary | std::ios::trunc);
  if (keyfile.fail()) {
    throw std::runtime_error("Could not open keyfile");
  }
  keyfile.write(
      make_signed_char(stored_key.get_serialized_key().data()),
      static_cast<std::streamsize>(stored_key.get_serialized_key().size()));

  std::ofstream pubkeyfile(keypath.get_pubkey_path(),
                           std::ios::out | std::ios::binary | std::ios::trunc);
  if (pubkeyfile.fail()) {
    throw std::runtime_error("Could not open pubkeyfile");
  }
  pubkeyfile.write(make_signed_char(pk.data()),
                   static_cast<std::streamsize>(pk.size()));

  std::ofstream symkeyfile(keypath.get_symkey_path(),
                           std::ios::out | std::ios::binary | std::ios::trunc);
  if (symkeyfile.fail()) {
    throw std::runtime_error("Could not open symkeyfile");
  }
  symkeyfile.write(make_signed_char(symkey.data()), symkey.size());
  std::print("Created key repository at {}\n", keypath.root.c_str());
}

void read_entry(const key_repo_t& keypath,
                const std::filesystem::path& entry,
                const std::optional<std::filesystem::path>& output)
{
  std::ifstream stream(entry, std::ios::in | std::ios::binary);
  if (stream.fail()) {
    throw std::runtime_error("Could not open entry file");
  }
  std::vector<unsigned char> contents((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());
  const auto symkey = load_symkey(keypath.get_symkey_path());

  const auto password =
      keypath.private_key_password
          .or_else([]() { return std::optional(read_password()); })
          .value();
  ;
  const auto private_key =
      load_private_key(keypath.get_private_key_path(), password);
  const auto decrypted = decrypt(
      symkey_span_t {symkey}, private_key_span_t {private_key}, contents);
  if (output) {
    std::ofstream output_stream(output.value(), std::ios::out);
    if (output_stream.fail()) {
      throw std::runtime_error("Could not open entry file");
    }
    output_stream.write(make_signed_char(decrypted.data()),
                        static_cast<std::streamsize>(decrypted.size()));
  } else {
    std::print("{}\n", std::string(decrypted.begin(), decrypted.end()));
  }
}
