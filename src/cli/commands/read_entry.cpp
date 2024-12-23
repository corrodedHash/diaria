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
#include <memory>
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

#include "cli/command_types.hpp"
#include "cli/key_management.hpp"
#include "util/char.hpp"

void read_entry(std::unique_ptr<entry_decryptor_initializer> keys,
                const std::filesystem::path& entry,
                const std::optional<std::filesystem::path>& output)
{
  std::ifstream stream(entry, std::ios::in | std::ios::binary);
  if (stream.fail()) {
    throw std::runtime_error("Could not open entry file");
  }
  std::vector<unsigned char> contents((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());

  const auto decrypted = keys->init().decrypt(contents);
  if (!output) {
    std::println("{}", std::string(decrypted.begin(), decrypted.end()));
    return;
  }
  std::ofstream output_stream(output.value(), std::ios::out);
  if (output_stream.fail()) {
    throw std::runtime_error("Could not open entry file");
  }
  output_stream.write(make_signed_char(decrypted.data()),
                      static_cast<std::streamsize>(decrypted.size()));
}