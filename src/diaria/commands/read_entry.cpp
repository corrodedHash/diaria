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
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

#include "./read_entry.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

#include "common.hpp"
#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"
#include "diaria/key_management.hpp"

void read_entry(const key_repo_paths_t& keypath,
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