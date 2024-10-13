#pragma once

#include <filesystem>

#include "util.hpp"

void dump_repo(const key_path_t& keypath,
               const std::filesystem::path& source,
               const std::filesystem::path& target,
               std::string_view password);
void load_repo(const key_path_t& keypath,
               const std::filesystem::path& source,
               const std::filesystem::path& target);