#pragma once
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "cli/command_types.hpp"
#include "crypto/safe_buffer.hpp"

struct input_reader
{
  virtual auto get_plaintext() -> safe_vector<unsigned char> = 0;
  virtual ~input_reader() = default;
};

struct file_input_reader final : input_reader
{
  explicit file_input_reader(input_file_t in_input_file)
      : input_file(std::move(in_input_file))
  {
  }
  input_file_t input_file;

  auto get_plaintext() -> safe_vector<unsigned char> override;
};

struct cmdline_input_reader : input_reader
{
  std::string_view cmdline;
  explicit cmdline_input_reader(std::string_view in_cmdline)
      : cmdline(in_cmdline)
  {
  }
};

struct editor_input_reader final : cmdline_input_reader
{
  explicit editor_input_reader(std::string_view in_cmdline)
      : cmdline_input_reader(in_cmdline)
  {
  }

  auto get_plaintext() -> safe_vector<unsigned char> override;
};

struct sandbox_editor_input_reader final : cmdline_input_reader
{
  explicit sandbox_editor_input_reader(std::string_view in_cmdline)
      : cmdline_input_reader(in_cmdline)
  {
  }

  auto get_plaintext() -> safe_vector<unsigned char> override;
};

struct entry_writer
{
  virtual auto write_entry(std::span<const unsigned char> ciphertext)
      -> void = 0;
  virtual ~entry_writer() = default;
};

struct file_entry_writer : entry_writer
{
};

struct repo_entry_writer final : file_entry_writer
{
  repo_path_t repo_path;

  auto write_entry(std::span<const unsigned char> ciphertext) -> void override;
};

struct outfile_entry_writer final : file_entry_writer
{
  output_file_t outfile;

  auto write_entry(std::span<const unsigned char> ciphertext) -> void override;
};

struct entry_recovery
{
  virtual auto handle(std::span<const unsigned char> plaintext,
                      std::optional<std::span<const unsigned char>> ciphertext)
      -> void = 0;
  virtual ~entry_recovery() = default;
};

void add_entry(std::unique_ptr<entry_encryptor_initializer> keys,
               std::unique_ptr<input_reader> input,
               std::unique_ptr<entry_writer> output);
