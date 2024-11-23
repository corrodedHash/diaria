#pragma once
#include <filesystem>
#include <optional>

#include "commands/add_entry.hpp"  // IWYU pragma: export
#include "commands/repo.hpp"  // IWYU pragma: export
#include "commands/stats.hpp"  // IWYU pragma: export
#include "commands/summarize.hpp"  // IWYU pragma: export
#include "key_management.hpp"

void setup_db(const key_repo_t& keypath);

void read_entry(const key_repo_t& keypath,
                const std::filesystem::path& entry,
                const std::optional<std::filesystem::path>& output);
