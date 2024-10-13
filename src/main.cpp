#include <cstdlib>
#include <memory>
#include <print>
#include <string>

#include "CLI11.hpp"
#include "diaria/lib.hpp"
#include "diaria/repo.hpp"
#include "diaria/util.hpp"

auto main(int argc, char** argv) -> int
{
  CLI::App app {"Diary entry manager"};
  argv = app.ensure_utf8(argv);
  app.require_subcommand(1);

  const key_path_t keypath {"/home/lukas/diaria"};
  const std::filesystem::path entrypath("/home/lukas/diaria/entries");

  auto* subcom_setup =
      app.add_subcommand("init",
                         "Initialize the diaria database on this system")
          ->final_callback(setup_db);
  CLI::App* subcom_add =
      app.add_subcommand("add", "Add a new diary entry")
          ->final_callback([&keypath, &entrypath]()
                           { add_entry(keypath, entrypath); });
  {
    std::string entry_path {};
    CLI::App* subcom_read =
        app.add_subcommand("read", "Read a diary entry")
            ->final_callback([&keypath, &entry_path]()
                             { read_entry(keypath, entry_path); });
    subcom_read->add_option("path", entry_path, "Path to entry")->required();
  }
  {
    auto subcom_repo =
        std::make_shared<CLI::App>("Manage the entire repository", "repo");
    subcom_repo->require_subcommand(1);
    std::string dumped_repo_path {};
    CLI::App* subcom_repo_load = subcom_repo->add_subcommand("load");
    subcom_repo_load->add_option(
        "dumppath",
        dumped_repo_path,
        "Directory containing the cleartext entries to load");
    CLI::App* subcom_repo_dump = subcom_repo->add_subcommand("dump");
    subcom_repo_dump->add_option("dumppath",
                                 dumped_repo_path,
                                 "Directory to store the cleartext entries in");

    subcom_repo_dump->final_callback(
        [&keypath, &entrypath, &dumped_repo_path]()
        {
          const auto password = read_password();
          dump_repo(
              key_path_t {keypath}, entrypath, dumped_repo_path, password);
        });
    subcom_repo_load->final_callback(
        [&keypath, &entrypath, &dumped_repo_path]()
        { load_repo(key_path_t {keypath}, dumped_repo_path, entrypath); });
    app.add_subcommand(subcom_repo);
  }

  CLI11_PARSE(app, argc, argv);
  return 0;
}
