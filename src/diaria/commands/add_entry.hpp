#pragma once
#include <vector>

#include "diaria/command_types.hpp"
#include "diaria/key_management.hpp"

struct input_reader
{
  virtual auto get_plaintext() -> std::vector<unsigned char> = 0;
  virtual ~input_reader() = default;
};

struct file_input_reader final : input_reader
{
  input_file_t input_file;

  auto get_plaintext() -> std::vector<unsigned char> override;
};

struct editor_input_reader final : input_reader
{
  std::string_view cmdline;

  auto get_plaintext() -> std::vector<unsigned char> override;
};

struct entry_writer
{
  virtual auto write_entry(std::span<const unsigned char> ciphertext)
      -> void = 0;
  virtual ~entry_writer() = default;
};

struct file_entry_writer : entry_writer
{
protected:
  static void write_to_file(const std::filesystem::path& filename,
                            std::span<const unsigned char> data);
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

void add_entry(const key_repo_paths_t& keypath,
               std::unique_ptr<input_reader> input,
               std::unique_ptr<entry_writer> output);
