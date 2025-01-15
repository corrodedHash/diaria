#include <memory>

#include "./cli_commands.hpp"

#include "cli/command_types.hpp"
#include "cli/commands/add_entry.hpp"
#include "project_info.hpp"
#include "xdg_paths.hpp"

namespace cli_commands
{
base::base()
{
  const xdg_paths base_paths {};
  keyrepo = {.root = base_paths.data_home / "diaria"};
  repopath = {base_paths.data_home / "diaria" / "entries"};
  configpath = base_paths.config_home / "diaria.toml";
  password = std::make_unique<stdin_password_provider>();
}

[[nodiscard]] auto base::create_command() -> std::unique_ptr<CLI::App>
{
  auto app = std::make_unique<CLI::App>("Diary entry manager");
  app->require_subcommand(1);

  app->add_flag_callback(
      "-V,--version",
      []()
      {
        const auto version_string = std::format(
            "{}.{}.{}", version::major, version::minor, version::patch);
        std::print("Diaria {}\n", version_string);
        std::exit(0);
      });
  app->add_option("-e,--entries",
                  [&repopath = repopath](auto paths)
                  {
                    const auto& read_path = paths.at(0);
                    repopath = repo_path_t(read_path);
                    return true;
                  })
      ->description("Path to the entry repository")
      ->default_str(repopath.repo);
  app->add_option("-k,--keys",
                  [&keyrepo = keyrepo](auto paths)
                  {
                    const auto& read_path = paths.at(0);
                    keyrepo.root = read_path;
                    return true;
                  })
      ->description("Path to the entry repository")
      ->default_str(keyrepo.root);
  app->add_option(
         "-p,--password",
         [&password = password](auto paths)
         {
           const auto& read_password = paths.at(0);
           password = std::make_unique<stored_password_provider>(read_password);
           return true;
         })
      ->description("Password for unlocking the private key.");
  app->add_option("--password_file",
                  [&password = password](auto paths)
                  {
                    const auto& read_password = paths.at(0);
                    password =
                        std::make_unique<file_password_provider>(read_password);
                    return true;
                  })
      ->description("File to read for the password to unlock the private key.");
  app->set_config("-c,--config", configpath.generic_string());
  return app;
}

auto add::create_command(base& base_command) -> CLI::App_p
{
  auto subcom_add = std::make_unique<CLI::App>("Add a new diary entry", "add");
  subcom_add->add_option(
      "-i,--input",
      [&input_path = input_path](const auto& input_files)
      {
        input_path =
            std::optional(input_file_t {std::filesystem::path(input_files[0])});
        return true;
      },
      "Non-interactively add an entry");
  subcom_add->add_option(
      "-o,--output",
      [&output_path = output_path](const auto& output_files)
      {
        output_path = std::optional(
            output_file_t {std::filesystem::path(output_files[0])});
        return true;
      },
      "Output path of entry file. Leave empty to generate with timestamp.");
  subcom_add
      ->add_option("--editor",
                   cmdline,
                   "Use the editor commandline. Write a % separated by a "
                   "space for the "
                   "temporary file which will be written.")
      ->default_str(cmdline);
  subcom_add->add_flag("--no-sandbox",
                       no_sandbox,
                       "Disable mount namespace sandboxing of editor");
  const std::function<void()> add_callback = [&keyrepo = base_command.keyrepo,
                                              &repopath = base_command.repopath,
                                              &cmdline = cmdline,
                                              no_sandbox = no_sandbox,
                                              &input_path = input_path,
                                              &output_path = output_path]()
  {
    std::unique_ptr<input_reader> input;
    if (input_path) {
      input = std::make_unique<file_input_reader>(*input_path);
    } else {
      if (no_sandbox) {
        input = std::make_unique<editor_input_reader>(cmdline);
      } else {
        input = std::make_unique<sandbox_editor_input_reader>(cmdline);
      }
    }
    std::unique_ptr<entry_writer> output;
    if (output_path) {
      auto bla = outfile_entry_writer {};
      bla.outfile = *output_path;
      output = std::make_unique<outfile_entry_writer>(bla);
    } else {
      auto bla = repo_entry_writer {};
      bla.repo_path = repopath;
      output = std::make_unique<repo_entry_writer>(bla);
    }
    add_entry(std::make_unique<file_entry_encryptor_initializer>(keyrepo),
              std::move(input),
              std::move(output));
  };
  subcom_add->final_callback(add_callback);
  return subcom_add;
}
}  // namespace cli_commands
