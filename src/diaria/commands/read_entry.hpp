#pragma once
#include <filesystem>
#include <optional>

#include "diaria/key_management.hpp"

void read_entry(const key_repo_paths_t& keypath,
                const std::filesystem::path& entry,
                const std::optional<std::filesystem::path>& output);
