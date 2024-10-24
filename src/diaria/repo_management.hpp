#pragma once
#include <chrono>
#include <filesystem>
#include <optional>
#include <string_view>

#include "diaria/commands.hpp"
using time_point = std::chrono::utc_clock::time_point;

// Function to parse the timestamp from the filename
auto parse_timestamp(std::string_view filename) -> std::optional<time_point>;

auto list_entries(const repo_path_t& repo)
    -> std::vector<std::pair<time_point, std::filesystem::path>>;