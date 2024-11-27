#include <cstdlib>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <print>
#include <string>
#include <utility>

#include <CLI11/CLI11.hpp>
#include <pwd.h>
#include <unistd.h>

#include "cli_commands.hpp"
#include "diaria/command_types.hpp"
#include "diaria/commands.hpp"

auto main(int argc, char** argv) -> int
{
  cli_commands::base base_command {};
  auto app = base_command.create_command();
  app->add_subcommand("init", "Initialize the diaria database on this system")
      ->final_callback(
          [&keyrepo = base_command.keyrepo, &password = base_command.password]()
          { setup_db(keyrepo, std::move(password)); });
  cli_commands::add add_command_data {};
  auto add_command = add_command_data.create_command(base_command);
  app->add_subcommand(add_command);

  std::filesystem::path read_entry_path {};
  std::optional<std::filesystem::path> read_output {};
  CLI::App* subcom_read =
      app->add_subcommand("read", "Read a diary entry")
          ->final_callback(
              [&keyrepo = base_command.keyrepo,
               &password = base_command.password,
               &read_entry_path,
               &read_output]()
              {
                read_entry(std::make_unique<file_entry_decryptor_initializer>(
                               std::move(password), keyrepo),
                           read_entry_path,
                           read_output);
              });
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
  CLI::App* subcom_repo_load = app->add_subcommand(
      "load", "Load a dumped directory of cleartext into the repository");
  subcom_repo_load
      ->add_option("dumppath",
                   dumped_repo_path,
                   "Directory containing the cleartext entries to load")
      ->required();
  CLI::App* subcom_repo_dump = app->add_subcommand(
      "dump", "Dump the entry files in the repository as cleartext");
  subcom_repo_dump
      ->add_option("dumppath",
                   dumped_repo_path,
                   "Directory to store the cleartext entries in")
      ->required();

  subcom_repo_dump->final_callback(
      [&keyrepo = base_command.keyrepo,
       &repopath = base_command.repopath,
       &password = base_command.password,
       &dumped_repo_path]()
      {
        dump_repo(std::make_unique<file_entry_decryptor_initializer>(
                      std::move(password), keyrepo),
                  repopath,
                  dumped_repo_path);
      });
  subcom_repo_load->final_callback(
      [&keyrepo = base_command.keyrepo,
       &repopath = base_command.repopath,
       &dumped_repo_path]()
      {
        load_repo(std::make_unique<file_entry_encryptor_initializer>(keyrepo),
                  repopath,
                  dumped_repo_path);
      });

  CLI::App* subcom_repo_sync = app->add_subcommand(
      "sync", "Synchronize repository with configured remote server");
  subcom_repo_sync->final_callback([&repopath = base_command.repopath]()
                                   { sync_repo(repopath); });

  CLI::App* subcom_repo_summarize = app->add_subcommand(
      "summarize",
      "Show some entries from different time intervals in the past");
  bool summarize_long {};
  subcom_repo_summarize->add_flag(
      "--long",
      summarize_long,
      "Do not press enter to advance to next entry, just print everything");
  subcom_repo_summarize->final_callback(
      [&keyrepo = base_command.keyrepo,
       &repopath = base_command.repopath,
       &password = base_command.password,
       &summarize_long]()
      {
        summarize_repo(std::make_unique<file_entry_decryptor_initializer>(
                           std::move(password), keyrepo),
                       repopath,
                       !summarize_long);
      });

  CLI::App* subcom_repo_stats =
      app->add_subcommand("stats", "Show stats of the repository");
  subcom_repo_stats->final_callback([&repopath = base_command.repopath]()
                                    { repo_stats(repopath); });
  try {
    CLI11_PARSE(*app, argc, argv);

  } catch (const std::exception& ex) {
    std::println(
        stderr, "An error occurred:\n {}\n{}", typeid(ex).name(), ex.what());
    std::exit(1);
  }
  return 0;
}
