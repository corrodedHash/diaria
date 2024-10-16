#pragma once

#include <filesystem>
#include <string_view>

#include "util.hpp"

struct repo_path_t
{
  std::filesystem::path repo;
};
void dump_repo(const key_repo_t& keypath,
               const repo_path_t& repo,
               const std::filesystem::path& target,
               std::string_view password);
void load_repo(const key_repo_t& keypath,
               const repo_path_t& repo,
               const std::filesystem::path& source);

void sync_repo(const repo_path_t& repo);