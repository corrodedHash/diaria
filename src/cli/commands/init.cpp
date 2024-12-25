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
#include <memory>
#include <print>
#include <ranges>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "./init.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

#include "cli/command_types.hpp"
#include "cli/key_management.hpp"
#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"
#include "util/char.hpp"

void setup_db(const key_repo_paths_t& keypath,
              std::unique_ptr<password_provider> password)
{
  const auto [pk, sk] = generate_keypair();
  const auto symkey = generate_symkey();

  std::error_code dir_creation_error {};
  std::filesystem::create_directories(keypath.root, dir_creation_error);
  if (dir_creation_error) {
    throw std::runtime_error(std::format("Could not create directories: {}",
                                         dir_creation_error.message()));
  };
  const auto stored_key =
      stored_secret_key::store(sk.span(), password->provide());
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
  symkeyfile.write(make_signed_char(symkey.data()),
                   static_cast<std::streamsize>(symkey.size()));
  std::print("Created key repository at {}\n", keypath.root.c_str());
}