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
  virtual ~input_reader(){}
};

struct file_input_reader final : input_reader
{
  input_file_t input_file;

  auto get_plaintext() -> std::vector<unsigned char> override;
  file_input_reader() = default;
  file_input_reader(const file_input_reader&) = default;
  file_input_reader(file_input_reader&&) = delete;
  file_input_reader& operator=(const file_input_reader&) = default;
  file_input_reader& operator=(file_input_reader&&) = delete;
  ~file_input_reader() override {}
};

struct editor_input_reader final : input_reader
{
  std::string_view cmdline;

  auto get_plaintext() -> std::vector<unsigned char> override;
  editor_input_reader() = default;
  editor_input_reader(const editor_input_reader&) = default;
  editor_input_reader(editor_input_reader&&) = delete;
  editor_input_reader& operator=(const editor_input_reader&) = default;
  editor_input_reader& operator=(editor_input_reader&&) = delete;
  ~editor_input_reader() override {}
};

void add_entry(const key_repo_t& keypath,
               const std::filesystem::path& entrypath,
               std::unique_ptr<input_reader> input,
               const std::optional<output_file_t>& maybe_output_file);

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