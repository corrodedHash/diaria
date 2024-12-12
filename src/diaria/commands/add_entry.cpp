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
#include <memory>
#include <print>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "./add_entry.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

#include "common.hpp"
#include "diaria/command_types.hpp"
#include "diaria/key_management.hpp"

namespace
{
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

void replace_first(std::string& input_string,
                   std::string_view to_replace,
                   std::string_view replace_with)
{
  const std::size_t pos = input_string.find(to_replace);
  if (pos == std::string::npos) {
    return;
  }
  input_string.replace(pos, to_replace.length(), replace_with);
}

void start_editor(std::string_view cmdline,
                  const std::filesystem::path& temp_entry_path)
{
  auto owned_cmdline = std::string(cmdline);
  replace_first(owned_cmdline, "%", temp_entry_path.c_str());

  const auto words = build_argv(owned_cmdline);
  std::vector<char*> argv {};
  argv.reserve(words.size());
  for (const auto& word : words) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    argv.push_back(const_cast<char*>(word.data()));
  }
  argv.push_back(nullptr);
  const auto exec_result = execvp(argv[0], argv.data());
  throw std::runtime_error(
      std::format("Error during exec editor: {}", exec_result));
}

struct smart_fd
{
  explicit smart_fd(int new_fd)
      : fd(new_fd)
  {
  }
  smart_fd(const smart_fd&) = delete;
  smart_fd(smart_fd&&) = delete;
  auto operator=(const smart_fd&) -> smart_fd& = delete;
  auto operator=(smart_fd&&) -> smart_fd& = delete;
  int fd;
  ~smart_fd() { close(fd); }
};

auto get_iso_timestamp_utc() -> std::string
{
  return std::format("{:%FT%T}",
                     std::chrono::floor<std::chrono::seconds>(
                         std::chrono::system_clock::now()));
}
}  // namespace

auto file_input_reader::get_plaintext() -> std::vector<unsigned char>
{
  std::ifstream stream(input_file.p, std::ios::in | std::ios::binary);
  if (stream.fail()) {
    throw std::runtime_error("Could not open input file");
  }
  std::vector<unsigned char> contents((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());
  return contents;
}

auto private_namespace_read()  -> std::vector<unsigned char>{
  // clone();
  // CLONE_NEWNS, CLONE_NEWUSER
  // Map root to current user
  // Write to /proc/pid/uid_map
  // mount -t tmpfs -o noswap diaria_entry /tmp/diaria

}

auto editor_input_reader::get_plaintext() -> std::vector<unsigned char>
{
  std::string temp_entry_path("/tmp/diaria_XXXXXX");
  const smart_fd entry_fd {mkostemp(temp_entry_path.data(), O_CLOEXEC)};
  const auto child_pid = fork();
  if (child_pid == 0) {
    start_editor(cmdline, temp_entry_path);
  }

  int child_status {};
  waitpid(child_pid, &child_status, 0);

  std::ifstream stream(temp_entry_path, std::ios::in | std::ios::binary);
  std::vector<unsigned char> contents((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());
  if (stream.fail()) {
    std::println(
        stderr,
        "Error reading diary file. Unencrypted entry is still stored at {}",
        temp_entry_path);
    throw std::runtime_error("Could not read diary entry file");
  }
  if (contents.empty()) {
    std::println(stderr,
                 "Diary entry empty, not saving. Temporary file remains at {}",
                 temp_entry_path);
    throw std::runtime_error("Could not create diary entry file");
  }
  unlink(temp_entry_path.c_str());
  return contents;
}

void file_entry_writer::write_to_file(const std::filesystem::path& filename,
                                      std::span<const unsigned char> data)
{
  std::filesystem::create_directories(filename.parent_path());

  std::ofstream entry_file(filename.c_str(),
                           std::ios::out | std::ios::binary | std::ios::trunc);
  if (entry_file.fail()) {
    throw std::runtime_error(
        std::format("Could not open output file \"{}\"", filename.c_str()));
  }
  entry_file.write(make_signed_char(data.data()),
                   static_cast<std::streamsize>(data.size()));

  if (entry_file.fail()) {
    throw std::runtime_error("Could not write to output file");
  }
}

auto repo_entry_writer::write_entry(std::span<const unsigned char> ciphertext)
    -> void
{
  write_to_file(
      repo_path.repo / std::format("{}.diaria", get_iso_timestamp_utc()),
      ciphertext);
}

auto outfile_entry_writer::write_entry(
    std::span<const unsigned char> ciphertext) -> void
{
  write_to_file(outfile.p, ciphertext);
}

void add_entry(std::unique_ptr<entry_encryptor_initializer> keys,
               std::unique_ptr<input_reader> input,
               std::unique_ptr<entry_writer> output)
{
  const auto plaintext = input->get_plaintext();
  const auto encrypted = keys->init().encrypt(plaintext);
  output->write_entry(encrypted);
}