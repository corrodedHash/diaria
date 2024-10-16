#include <array>
#include <chrono>
#include <cstddef>
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
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "./commands.hpp"

#include <fcntl.h>
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

auto build_argv(std::string_view cmdline)
{
  wordexp_t words {};
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  wordexp(cmdline.data(), &words, WRDE_NOCMD | WRDE_UNDEF | WRDE_SHOWERR);

  const auto word_span = std::span(words.we_wordv, words.we_wordc);
  const auto owned_word_span = word_span
      | std::ranges::views::transform([](auto word)
                                      { return std::string(word); });
  std::vector<std::string> result(std::begin(owned_word_span),
                                  std::end(owned_word_span));

  wordfree(&words);
  return result;
}

auto create_entry_interactive(std::string_view cmdline)
    -> std::vector<unsigned char>
{
  std::string temp_entry_path("/tmp/diaria_XXXXXX");
  int entry_fd(mkostemp(temp_entry_path.data(), O_CLOEXEC));
  auto child_pid = fork();
  if (child_pid == 0) {
    const auto words = build_argv(cmdline);
    std::vector<char*> argv {};
    for (const auto& word : words) {
      if (word == "%") {
        argv.push_back(temp_entry_path.data());
        continue;
      }
      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
      argv.push_back(const_cast<char*>(word.data()));
    }
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());
  }

  int child_status {};
  waitpid(child_pid, &child_status, 0);
  constexpr std::size_t buffersize = 512;
  std::array<unsigned char, buffersize> mybuffer {};

  ssize_t bytesread = 1;
  std::vector<unsigned char> contents {};

  while ((bytesread = read(entry_fd, mybuffer.data(), mybuffer.size())) > 0) {
    contents.insert(
        contents.end(), mybuffer.begin(), mybuffer.begin() + bytesread);
  }
  if (bytesread < 0) {
    std::print(
        stderr,
        "Error reading diary file. Unencrypted entry is still stored at {}",
        temp_entry_path);
    throw std::runtime_error("Could not read diary entry file");
  }
  unlink(temp_entry_path.c_str());
  return contents;
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

void add_entry(const key_repo_t& keypath,
               const std::filesystem::path& entrypath,
               std::string_view cmdline,
               std::optional<std::filesystem::path> maybe_input_file)
{
  const auto load_input_file = [](const std::filesystem::path& input_file)
  {
    std::ifstream stream(input_file, std::ios::in | std::ios::binary);
    if (stream.fail()) {
      throw std::runtime_error("Could not open input file");
    }
    std::vector<unsigned char> contents(
        (std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());
    return std::optional(contents);
  };
  const auto load_interactive_file = [&cmdline]()
  { return std::optional(create_entry_interactive(cmdline)); };
  const auto plaintext = maybe_input_file.and_then(load_input_file)
                             .or_else(load_interactive_file)
                             .value();

  const auto symkey = load_symkey(keypath.get_symkey_path());
  const auto pubkey = load_pubkey(keypath.get_pubkey_path());

  const auto encrypted =
      encrypt(symkey_span_t {symkey}, public_key_span_t {pubkey}, plaintext);
  const auto file_name =
      entrypath / std::format("{}.diaria", get_iso_timestamp_utc());
  std::filesystem::create_directories(entrypath);
  std::ofstream entry_file(file_name.c_str(),
                           std::ios::out | std::ios::binary | std::ios::trunc);
  if (entry_file.fail()) {
    throw std::runtime_error("Could not open output file");
  }
  entry_file.write(make_signed_char(encrypted.data()),
                   static_cast<std::streamsize>(encrypted.size()));
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
