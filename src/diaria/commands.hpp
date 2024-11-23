#pragma once
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include "key_management.hpp"

void setup_db(const key_repo_t& keypath);

struct input_file_t
{
  std::filesystem::path p;
};
struct output_file_t
{
  std::filesystem::path p;
};

struct repo_path_t
{
  std::filesystem::path repo;
};

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
  virtual auto write_entry(std::span<const unsigned char> ciphertext) -> void = 0;
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

void add_entry(const key_repo_t& keypath,
               std::unique_ptr<input_reader> input,
               std::unique_ptr<entry_writer> output);

void read_entry(const key_repo_t& keypath,
                const std::filesystem::path& entry,
                const std::optional<std::filesystem::path>& output);

void dump_repo(const key_repo_t& keypath,
               const repo_path_t& repo,
               const std::filesystem::path& target);

void load_repo(const key_repo_t& keypath,
               const repo_path_t& repo,
               const std::filesystem::path& source);

void sync_repo(const repo_path_t& repo);

void summarize_repo(const key_repo_t& keypath,
                    const repo_path_t& repo,
                    bool paging);

void repo_stats(const repo_path_t& repo);