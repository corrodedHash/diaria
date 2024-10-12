#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iostream>
#include <print>
#include <stdexcept>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>  // For fstat
#include <sys/wait.h>
#include <unistd.h>

#include "CLI11.hpp"
#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"
#include "mmap_file.hpp"

auto setup_db()
{
  const auto [pk, sk] = generate_keypair();
  const auto symkey = generate_symkey();

  if (!std::filesystem::create_directories("/home/lukas/diaria")) {
    throw std::runtime_error("Could not create directory");
  };

  const auto stored_key = stored_secret_key::store(sk, "abc");
  std::ofstream keyfile("/home/lukas/diaria/key.key",
                        std::ios::out | std::ios::binary | std::ios::trunc);
  keyfile.write(
      reinterpret_cast<const char*>(stored_key.get_serialized_key().data()),
      stored_key.get_serialized_key().size());

  std::ofstream pubkeyfile("/home/lukas/diaria/key.pub",
                           std::ios::out | std::ios::binary | std::ios::trunc);
  pubkeyfile.write(reinterpret_cast<const char*>(pk.data()), pk.size());

  std::ofstream symkeyfile("/home/lukas/diaria/key.symkey",
                           std::ios::out | std::ios::binary | std::ios::trunc);
  symkeyfile.write(reinterpret_cast<const char*>(symkey.data()), symkey.size());
}

auto add_entry()
{
  std::filesystem::path diaria_path("/home/lukas/diaria");

  std::string temp_entry_path("/tmp/diaria_XXXXXX");
  const auto entry_fd = mkostemp(temp_entry_path.data(), O_CLOEXEC);
  const auto child_pid = fork();
  if (child_pid == 0) {
    std::array<char*, 3> arguments = {"vim", temp_entry_path.data(), 0};
    execvp("vim", arguments.data());
  }

  int child_status {};
  waitpid(child_pid, &child_status, 0);
  unlink(temp_entry_path.c_str());

  mmap_file file_span(entry_fd);

  symkey_t symkey {};
  std::ifstream symkey_file((diaria_path / "key.symkey").c_str(),
                            std::ios::in | std::ios::binary);
  symkey_file.read(reinterpret_cast<char*>(symkey.data()), symkey.size());

  public_key_t pubkey {};
  std::ifstream pubkey_file((diaria_path / "key.pub").c_str(),
                            std::ios::in | std::ios::binary);
  pubkey_file.read(reinterpret_cast<char*>(pubkey.data()), pubkey.size());

  auto encrypted = encrypt(symkey, pubkey, file_span.getSpan());

  std::ofstream entry_file((diaria_path / "entry.dia").c_str(),
                           std::ios::out | std::ios::binary | std::ios::trunc);
  entry_file.write(reinterpret_cast<const char*>(encrypted.data()),
                   encrypted.size());
}

auto main(int argc, char** argv) -> int
{
  CLI::App app {"Diary entry manager"};
  argv = app.ensure_utf8(argv);
  app.require_subcommand();
  std::string filename = "default";
  app.add_option("-f,--file", filename, "A help string");

  auto subcom_setup =
      app.add_subcommand("init", "Initialize the diary database on this system")
          ->final_callback(setup_db);
  CLI::App* subcom_add = app.add_subcommand("add", "Add a new diary entry")
                             ->final_callback(add_entry);
  CLI::App* subcom_read =
      app.add_subcommand("read", "Read a diary entry")
          ->final_callback([]() { std::print("Hello {}\n", 1); });
  CLI11_PARSE(app, argc, argv);
  return 0;
}
