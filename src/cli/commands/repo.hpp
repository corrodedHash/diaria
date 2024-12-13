#pragma once
#include <filesystem>

#include "../command_types.hpp"

void dump_repo(std::unique_ptr<entry_decryptor_initializer> keys,
               const repo_path_t& repo,
               const std::filesystem::path& target);

void load_repo(std::unique_ptr<entry_encryptor_initializer> keys,
               const repo_path_t& repo,
               const std::filesystem::path& source);

void sync_repo(const repo_path_t& repo);