#pragma once
#include <filesystem>

struct input_file_t
{
  std::filesystem::path p;
};

struct output_file_t
{
  std::filesystem::path p;
};

struct repo_path_t
{
  std::filesystem::path repo;
};
