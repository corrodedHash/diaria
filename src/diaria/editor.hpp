#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

auto private_namespace_read(std::string_view cmdline)
    -> std::vector<unsigned char>;

void start_editor(std::string_view cmdline,
                  const std::filesystem::path& temp_entry_path);