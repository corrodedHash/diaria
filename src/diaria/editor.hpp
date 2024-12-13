#pragma once

#include <filesystem>
#include <string_view>
#include <vector>

auto private_namespace_read(std::string_view cmdline)
    -> std::vector<unsigned char>;

auto interactive_content_entry(std::string_view cmdline,
                               const std::filesystem::path& temp_file_dir)
    -> std::vector<unsigned char>;