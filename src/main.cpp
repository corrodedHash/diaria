#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include <CLI11/CLI11.hpp>
#include <pwd.h>
#include <unistd.h>

#include "diaria/commands.hpp"
#include "diaria/repo.hpp"
#include "diaria/util.hpp"

auto main(int argc, char** argv) -> int
{
  CLI::App app {"Diary entry manager"};
  argv = app.ensure_utf8(argv);
  app.require_subcommand(1);
  struct passwd* pw_entry = getpwuid(getuid());

  const std::filesystem::path homedir(pw_entry->pw_dir);
  auto* const xdg_data_home_raw = std::getenv("XDG_DATA_HOME");
  auto* const xdg_config_home_raw = std::getenv("XDG_CONFIG_HOME");
  const auto xdg_data_home = [&]()
  {
    if (xdg_data_home_raw == nullptr) {
      return homedir / ".local" / "share";
    }
    return std::filesystem::path(xdg_data_home_raw);
  }();
  const auto xdg_config_home = [&]()
  {
    if (xdg_config_home_raw == nullptr) {
      return homedir / ".config";
    }
    return std::filesystem::path(xdg_config_home_raw);
  }();
  // key_path_t keypath {xdg_data_home / "diaria"};
  // repo_path_t entrypath(xdg_data_home / "diaria" / "entries");
  key_path_t keypath {"/home/lukas/diaria"};
  repo_path_t repopath("/home/lukas/diaria/entries");
  const std::filesystem::path configpath(xdg_config_home / "diaria.toml");

  app.add_option(
      "--entries",
      [&repopath](auto paths)
      {
        const auto read_path = paths.at(0);
        repopath = repo_path_t(read_path);
        return true;
      },
      "Path to the entry repository");
  app.add_option(
      "--keys",
      [&keypath](auto paths)
      {
        const auto read_path = paths.at(0);
        keypath.root = read_path;
        return true;
      },
      "Path to the entry repository");
  app.set_config("--config", configpath.generic_string());
  app.add_subcommand("init", "Initialize the diaria database on this system")
      ->final_callback([&keypath]() { setup_db(keypath); });
  auto* subcom_add = app.add_subcommand("add", "Add a new diary entry");
  std::optional<std::filesystem::path> input_path {};
  std::string cmdline {"vim %"};
  subcom_add->add_option(
      "--input",
      [&input_path](const auto& input_files)
      {
        input_path = std::optional(std::filesystem::path(input_files[0]));
        return true;
      },
      "Non-interactively add an entry");
  subcom_add->add_option(
      "--editor",
      cmdline,
      "Use the editor commandline. Write a % separated by a space for the "
      "temporary file which will be written.");
  subcom_add->final_callback(
      [&keypath, &repopath, &cmdline, &input_path]()
      { add_entry(keypath, repopath.repo, cmdline, input_path); });
  std::filesystem::path read_entry_path {};
  CLI::App* subcom_read =
      app.add_subcommand("read", "Read a diary entry")
          ->final_callback([&keypath, &read_entry_path]()
                           { read_entry(keypath, read_entry_path); });
  subcom_read->add_option("path", read_entry_path, "Path to entry")
      ->check(CLI::ExistingFile)
      ->required();
  auto subcom_repo =
      std::make_shared<CLI::App>("Manage the entire repository", "repo");
  subcom_repo->require_subcommand(1);
  std::string dumped_repo_path {};
  CLI::App* subcom_repo_load = subcom_repo->add_subcommand(
      "load", "Load a dumped directory of cleartext into the repository");
  subcom_repo_load->add_option(
      "dumppath",
      dumped_repo_path,
      "Directory containing the cleartext entries to load");
  CLI::App* subcom_repo_dump = subcom_repo->add_subcommand(
      "dump", "Dump the entry files in the repository as cleartext");
  subcom_repo_dump->add_option("dumppath",
                               dumped_repo_path,
                               "Directory to store the cleartext entries in");

  subcom_repo_dump->final_callback(
      [&keypath, &repopath, &dumped_repo_path]()
      {
        const auto password = read_password();
        dump_repo(key_path_t {keypath}, repopath, dumped_repo_path, password);
      });
  subcom_repo_load->final_callback(
      [&keypath, &repopath, &dumped_repo_path]()
      { load_repo(key_path_t {keypath}, repopath, dumped_repo_path); });
  app.add_subcommand(subcom_repo);

  CLI::App* subcom_repo_sync = subcom_repo->add_subcommand(
      "sync", "Synchronize repository with configured remote server");
  subcom_repo_sync->final_callback([&repopath]() { sync_repo(repopath); });
  CLI11_PARSE(app, argc, argv);
  return 0;
}
