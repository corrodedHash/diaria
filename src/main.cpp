#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <print>
#include <stdexcept>
#include <string>

#include <CLI11/CLI11.hpp>
#include <pwd.h>
#include <unistd.h>

#include "diaria/commands.hpp"
#include "diaria/util.hpp"
#include "project_info.hpp"

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
  key_repo_t keyrepo {.root = xdg_data_home / "diaria",
                      .private_key_password = std::nullopt};
  repo_path_t repopath(xdg_data_home / "diaria" / "entries");
  const std::filesystem::path configpath(xdg_config_home / "diaria.toml");
  app.add_flag_callback(
      "-V,--version",
      []()
      {
        const auto version_string = std::format(
            "{}.{}.{}", version::major, version::minor, version::patch);
        std::print("Diaria {}\n", version_string);
        std::exit(0);
      });
  app.add_option("-e,--entries",
                 [&repopath](auto paths)
                 {
                   const auto read_path = paths.at(0);
                   repopath = repo_path_t(read_path);
                   return true;
                 })
      ->description("Path to the entry repository")
      ->default_str(repopath.repo);
  app.add_option("-k,--keys",
                 [&keyrepo](auto paths)
                 {
                   const auto read_path = paths.at(0);
                   keyrepo.root = read_path;
                   return true;
                 })
      ->description("Path to the entry repository")
      ->default_str(keyrepo.root);
  app.add_option("-p,--password",
                 [&keyrepo](auto paths)
                 {
                   const auto read_password = paths.at(0);
                   keyrepo.private_key_password = read_password;
                   return true;
                 })
      ->description("Password for unlocking the private key.");
  app.add_option("--password_file",
                 [&keyrepo](auto paths)
                 {
                   const auto read_password = paths.at(0);
                   std::ifstream password_file(read_password);
                   if (password_file.fail()) {
                     throw std::runtime_error("Could not open password file");
                   }
                   std::string password {};
                   std::getline(password_file, password);
                   keyrepo.private_key_password = std::optional(password);
                   return true;
                 })
      ->description("File to read for the password to unlock the private key.");
  app.set_config("-c,--config", configpath.generic_string());
  app.add_subcommand("init", "Initialize the diaria database on this system")
      ->final_callback([&keyrepo]() { setup_db(keyrepo); });
  auto* subcom_add = app.add_subcommand("add", "Add a new diary entry");
  std::optional<input_file_t> input_path {};
  std::optional<output_file_t> output_path {};
  std::string cmdline {"vim %"};
  subcom_add->add_option(
      "-i,--input",
      [&input_path](const auto& input_files)
      {
        input_path =
            std::optional(input_file_t {std::filesystem::path(input_files[0])});
        return true;
      },
      "Non-interactively add an entry");
  subcom_add->add_option(
      "-o,--output",
      [&output_path](const auto& output_files)
      {
        output_path = std::optional(
            output_file_t {std::filesystem::path(output_files[0])});
        return true;
      },
      "Output path of entry file. Leave empty to generate with timestamp.");
  subcom_add
      ->add_option(
          "--editor",
          cmdline,
          "Use the editor commandline. Write a % separated by a space for the "
          "temporary file which will be written.")
      ->default_str(cmdline);
  subcom_add->final_callback(
      [&keyrepo, &repopath, &cmdline, &input_path, &output_path]()
      { add_entry(keyrepo, repopath.repo, cmdline, input_path, output_path); });
  std::filesystem::path read_entry_path {};
  std::optional<std::filesystem::path> read_output {};
  CLI::App* subcom_read =
      app.add_subcommand("read", "Read a diary entry")
          ->final_callback(
              [&keyrepo, &read_entry_path, &read_output]()
              { read_entry(keyrepo, read_entry_path, read_output); });
  subcom_read
      ->add_option(
          "-o,--output",
          [&read_output](auto input)
          {
            if (input[0] == "-") {
              read_output = std::nullopt;
            } else {
              read_output = std::optional(input[0]);
            }
            return true;
          },
          "File to write output in")
      ->default_str("-");
  subcom_read->add_option("path", read_entry_path, "Path to entry")
      ->check(CLI::ExistingFile)
      ->required();
  std::string dumped_repo_path {};
  CLI::App* subcom_repo_load = app.add_subcommand(
      "load", "Load a dumped directory of cleartext into the repository");
  subcom_repo_load
      ->add_option("dumppath",
                   dumped_repo_path,
                   "Directory containing the cleartext entries to load")
      ->required();
  CLI::App* subcom_repo_dump = app.add_subcommand(
      "dump", "Dump the entry files in the repository as cleartext");
  subcom_repo_dump
      ->add_option("dumppath",
                   dumped_repo_path,
                   "Directory to store the cleartext entries in")
      ->required();

  subcom_repo_dump->final_callback(
      [&keyrepo, &repopath, &dumped_repo_path]()
      { dump_repo(key_repo_t {keyrepo}, repopath, dumped_repo_path); });
  subcom_repo_load->final_callback(
      [&keyrepo, &repopath, &dumped_repo_path]()
      { load_repo(key_repo_t {keyrepo}, repopath, dumped_repo_path); });

  CLI::App* subcom_repo_sync = app.add_subcommand(
      "sync", "Synchronize repository with configured remote server");
  subcom_repo_sync->final_callback([&repopath]() { sync_repo(repopath); });

  CLI::App* subcom_repo_summarize = app.add_subcommand(
      "summarize",
      "Show some entries from different time intervals in the past");
  bool summarize_long {};
  subcom_repo_summarize->add_flag(
      "--long",
      summarize_long,
      "Do not press enter to advance to next entry, just print everything");
  subcom_repo_summarize->final_callback(
      [&keyrepo, &repopath, &summarize_long]()
      { summarize_repo(keyrepo, repopath, !summarize_long); });
  CLI11_PARSE(app, argc, argv);
  return 0;
}
