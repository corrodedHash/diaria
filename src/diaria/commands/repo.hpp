#pragma once
#include <filesystem>

#include "../command_types.hpp"
#include "../key_management.hpp"

void dump_repo(const key_repo_t& keypath,
               const repo_path_t& repo,
               const std::filesystem::path& target);

void load_repo(const key_repo_t& keypath,
               const repo_path_t& repo,
               const std::filesystem::path& source);

void sync_repo(const repo_path_t& repo);