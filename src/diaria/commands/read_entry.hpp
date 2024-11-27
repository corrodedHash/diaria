#pragma once
#include <filesystem>
#include <optional>

#include "diaria/command_types.hpp"

void read_entry(std::unique_ptr<entry_decryptor_initializer> keys,
                const std::filesystem::path& entry,
                const std::optional<std::filesystem::path>& output);
