#pragma once
#include <chrono>
#include <filesystem>
#include <optional>
#include <string_view>

#include "command_types.hpp"
using time_point = std::chrono::utc_clock::time_point;

struct diaria_entry_path
{
  time_point entry_time;
  std::filesystem::path entry_path;
};

// Function to parse the timestamp from the filename
auto parse_timestamp(std::string_view filename) -> std::optional<time_point>;

auto list_entries(const repo_path_t& repo) -> std::vector<diaria_entry_path>;