#include <array>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <print>
#include <span>
#include <stdexcept>
#include <string>

#include "./commands.hpp"

#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include "./util.hpp"
#include "common.hpp"
#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"
#include "mmap_file.hpp"

void setup_db(const key_path_t& keypath)
{
  const auto [pk, sk] = generate_keypair();
  const auto symkey = generate_symkey();

  if (!std::filesystem::create_directories(keypath.root)) {
    throw std::runtime_error("Could not create directory");
  };
  const auto password = read_password();
  const auto stored_key = stored_secret_key::store(std::span(sk), password);
  std::ofstream keyfile(keypath.get_private_key_path(),
                        std::ios::out | std::ios::binary | std::ios::trunc);
  keyfile.write(
      make_signed_char(stored_key.get_serialized_key().data()),
      static_cast<std::streamsize>(stored_key.get_serialized_key().size()));

  std::ofstream pubkeyfile(keypath.get_pubkey_path(),
                           std::ios::out | std::ios::binary | std::ios::trunc);
  pubkeyfile.write(make_signed_char(pk.data()),
                   static_cast<std::streamsize>(pk.size()));

  std::ofstream symkeyfile(keypath.get_symkey_path(),
                           std::ios::out | std::ios::binary | std::ios::trunc);
  symkeyfile.write(make_signed_char(symkey.data()), symkey.size());
}

auto create_entry_interactive()
{
  std::string temp_entry_path("/tmp/diaria_XXXXXX");
  const auto entry_fd = mkostemp(temp_entry_path.data(), O_CLOEXEC);
  const auto child_pid = fork();
  if (child_pid == 0) {
    std::string exe_name("vim");
    std::array<char*, 3> arguments = {
        exe_name.data(), temp_entry_path.data(), 0};
    execvp("vim", arguments.data());
  }

  int child_status {};
  waitpid(child_pid, &child_status, 0);
  unlink(temp_entry_path.c_str());

  return mmap_file(entry_fd);
}

auto get_iso_timestamp_utc() -> std::string
{
  // Get current time in UTC
  auto now = std::chrono::system_clock::now();

  // Convert to time_t for easy manipulation
  std::time_t const now_time_t = std::chrono::system_clock::to_time_t(now);

  // Convert time_t to a tm structure for UTC time
  std::tm utc_tm = *std::gmtime(&now_time_t);

  constexpr int year = 1900;
  // Format the timestamp using std::format
  return std::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}",
                     utc_tm.tm_year + year,
                     utc_tm.tm_mon + 1,
                     utc_tm.tm_mday,
                     utc_tm.tm_hour,
                     utc_tm.tm_min,
                     utc_tm.tm_sec);
}

void add_entry(const key_path_t& keypath,
               const std::filesystem::path& entrypath)
{
  const auto file_span = create_entry_interactive();

  const auto symkey = load_symkey(keypath.get_symkey_path());
  const auto pubkey = load_pubkey(keypath.get_pubkey_path());

  const auto encrypted = encrypt(symkey, pubkey, file_span.get_span());
  const auto file_name =
      entrypath / std::format("{}.diaria", get_iso_timestamp_utc());
  std::ofstream entry_file(file_name.c_str(),
                           std::ios::out | std::ios::binary | std::ios::trunc);
  entry_file.write(make_signed_char(encrypted.data()),
                   static_cast<std::streamsize>(encrypted.size()));
}

void read_entry(const key_path_t& keypath, const std::filesystem::path& entry)
{
  const auto entry_fd = open(entry.c_str(), O_RDONLY | O_CLOEXEC);
  const mmap_file file_span(entry_fd);
  const auto symkey = load_symkey(keypath.get_symkey_path());

  const auto password = read_password();
  const auto private_key =
      load_private_key(keypath.get_private_key_path(), password);
  const auto decrypted = decrypt(symkey, private_key, file_span.get_span());
  std::print("{}", std::string(decrypted.begin(), decrypted.end()));
}
