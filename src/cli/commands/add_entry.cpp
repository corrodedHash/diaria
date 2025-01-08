#include <algorithm>
#include <cctype>
#include <chrono>
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
#include <span>
#include <stdexcept>
#include <string>

#include "./add_entry.hpp"

#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

#include "cli/command_types.hpp"
#include "cli/editor.hpp"
#include "cli/key_management.hpp"
#include "util/char.hpp"

namespace
{
auto get_iso_timestamp_utc() -> std::string
{
  return std::format("{:%FT%T}",
                     std::chrono::floor<std::chrono::seconds>(
                         std::chrono::system_clock::now()));
}
}  // namespace

auto file_input_reader::get_plaintext() -> safe_vector<unsigned char>
{
  std::ifstream stream(input_file.p, std::ios::in | std::ios::binary);
  if (stream.fail()) {
    throw std::runtime_error("Could not open input file");
  }
  safe_vector<unsigned char> contents((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());

  if (stream.fail()) {
    throw std::runtime_error("Could not read input file");
  }
  return contents;
}

auto editor_input_reader::get_plaintext() -> safe_vector<unsigned char>
{
  return interactive_content_entry(cmdline, std::filesystem::path {"/tmp"});
}

auto sandbox_editor_input_reader::get_plaintext() -> safe_vector<unsigned char>
{
  return private_namespace_read(cmdline);
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
  if (plaintext.empty()
      || std::ranges::all_of(plaintext, [](char c) { return std::isspace(c); }))
  {
    throw std::runtime_error(
        "Plaintext is empty or only contains whitespace; discarding");
  }
  const auto encrypted = keys->init().encrypt(plaintext);
  output->write_entry(encrypted);
}