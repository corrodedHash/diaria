#pragma once
#include <memory>
#include <CLI11/CLI11.hpp>

#include "cli/command_types.hpp"
#include "cli/key_management.hpp"
namespace cli_commands
{
struct base
{
  key_repo_paths_t keyrepo;
  repo_path_t repopath;
  std::filesystem::path configpath;
  std::unique_ptr<password_provider> password;

  base();

  [[nodiscard]] auto create_command() -> std::unique_ptr<CLI::App>;
};

struct add
{
  std::optional<input_file_t> input_path;
  std::optional<output_file_t> output_path;
  std::string cmdline {"vim %"};

  [[nodiscard]] auto create_command(base& base_command) -> CLI::App_p;
};
}  // namespace cli_commands
