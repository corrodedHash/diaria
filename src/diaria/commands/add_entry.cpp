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
#include <vector>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

#include "../commands.hpp"
#include "../util.hpp"
#include "common.hpp"
#include "crypto/entry.hpp"
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

auto create_entry_interactive(std::string_view cmdline)
    -> std::vector<unsigned char>
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

auto get_iso_timestamp_utc() -> std::string
{
  return std::format("{:%FT%H:%M:%S}", std::chrono::system_clock::now());
}
}  // namespace

void add_entry(const key_repo_t& keypath,
               const std::filesystem::path& entrypath,
               std::string_view cmdline,
               const std::optional<input_file_t>& maybe_input_file,
               const std::optional<output_file_t>& maybe_output_file)
{
  const auto load_input_file = [](const input_file_t& input_file)
  {
    std::ifstream stream(input_file.p, std::ios::in | std::ios::binary);
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
  const auto file_name = [&]()
  {
    if (maybe_output_file.has_value()) {
      auto output_path = maybe_output_file.value().p;
      if (output_path.is_absolute()) {
        return output_path;
      }
      std::filesystem::create_directories(entrypath);
      return (entrypath / output_path);
    }
    std::filesystem::create_directories(entrypath);

    return entrypath / std::format("{}.diaria", get_iso_timestamp_utc());
  }();
  std::ofstream entry_file(file_name.c_str(),
                           std::ios::out | std::ios::binary | std::ios::trunc);
  if (entry_file.fail()) {
    throw std::runtime_error(
        std::format("Could not open output file \"{}\"", file_name.c_str()));
  }
  entry_file.write(make_signed_char(encrypted.data()),
                   static_cast<std::streamsize>(encrypted.size()));

  if (entry_file.fail()) {
    throw std::runtime_error("Could not write to output file");
  }
}