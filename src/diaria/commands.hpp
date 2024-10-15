#pragma once
#include <filesystem>
#include <optional>
#include <string_view>

#include "util.hpp"

void setup_db(const key_path_t& keypath);

void add_entry(const key_path_t& keypath,
               const std::filesystem::path& entrypath,
               std::string_view cmdline,
               std::optional<std::filesystem::path> maybe_input_file);

void read_entry(const key_path_t& keypath,
                const std::filesystem::path& entry,
                const std::optional<std::filesystem::path>& output);