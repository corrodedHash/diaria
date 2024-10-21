#pragma once
#include <filesystem>
#include <optional>
#include <string_view>

#include "util.hpp"

void setup_db(const key_repo_t& keypath);


struct input_file_t {
  std::filesystem::path p;
};
struct output_file_t {
  std::filesystem::path p;
};

void add_entry(const key_repo_t& keypath,
               const std::filesystem::path& entrypath,
               std::string_view cmdline,
               const std::optional<input_file_t>& maybe_input_file,
               const std::optional<output_file_t>& maybe_output_file);

void read_entry(const key_repo_t& keypath,
                const std::filesystem::path& entry,
                const std::optional<std::filesystem::path>& output);