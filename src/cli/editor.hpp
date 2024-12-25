#pragma once

#include <filesystem>
#include <string_view>

#include "crypto/safe_buffer.hpp"

auto private_namespace_read(std::string_view cmdline)
    -> safe_vector<unsigned char>;

auto interactive_content_entry(std::string_view cmdline,
                               const std::filesystem::path& temp_file_dir)
    -> safe_vector<unsigned char>;