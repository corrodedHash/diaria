#pragma once
#include <filesystem>

#include "util.hpp"

void setup_db(const key_path_t& keypath);

void add_entry(const key_path_t& keypath,
               const std::filesystem::path& entrypath);

void read_entry(const key_path_t& keypath, const std::filesystem::path& entry);