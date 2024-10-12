#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <print>
#include <stdexcept>
#include <string>
#include <string_view>

#include <fcntl.h>
#include <sodium/crypto_stream_xchacha20.h>
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
  std::filesystem::path diaria_path("/home/lukas/diaria");

  const auto [pk, sk] = generate_keypair();
  const auto symkey = generate_symkey();

  if (!std::filesystem::create_directories(diaria_path)) {
    throw std::runtime_error("Could not create directory");
  };

  const auto stored_key = stored_secret_key::store(std::span(sk), "abc");
  std::ofstream keyfile(diaria_path / "key.key",
                        std::ios::out | std::ios::binary | std::ios::trunc);
  keyfile.write(
      reinterpret_cast<const char*>(stored_key.get_serialized_key().data()),
      stored_key.get_serialized_key().size());

  std::ofstream pubkeyfile(diaria_path / "key.pub",
                           std::ios::out | std::ios::binary | std::ios::trunc);
  pubkeyfile.write(reinterpret_cast<const char*>(pk.data()), pk.size());

  std::ofstream symkeyfile(diaria_path / "key.symkey",
                           std::ios::out | std::ios::binary | std::ios::trunc);
  symkeyfile.write(reinterpret_cast<const char*>(symkey.data()), symkey.size());
}

auto create_entry_interactive()
{
  std::string temp_entry_path("/tmp/diaria_XXXXXX");
  const auto entry_fd = mkostemp(temp_entry_path.data(), O_CLOEXEC);
  const auto child_pid = fork();
  if (child_pid == 0) {
    std::string exe_name("vim");
    std::array<char*, 3> arguments = {
        exe_name.data(), temp_entry_path.data(), 0};
    execvp("vim", arguments.data());
  }

  int child_status {};
  waitpid(child_pid, &child_status, 0);
  unlink(temp_entry_path.c_str());

  return mmap_file(entry_fd);
}

auto load_symkey(const std::filesystem::path& diaria_path)
{
  symkey_t symkey {};
  std::ifstream symkey_file((diaria_path / "key.symkey").c_str(),
                            std::ios::in | std::ios::binary);
  symkey_file.read(reinterpret_cast<char*>(symkey.data()), symkey.size());
  return symkey;
}

auto load_pubkey(const std::filesystem::path& diaria_path)
{
  public_key_t pubkey {};
  std::ifstream pubkey_file((diaria_path / "key.pub").c_str(),
                            std::ios::in | std::ios::binary);
  pubkey_file.read(reinterpret_cast<char*>(pubkey.data()), pubkey.size());
  return pubkey;
}

auto load_private_key(const std::filesystem::path& diaria_path,
                      std::string_view password)
{
  stored_secret_key::serialized_key_t stored_private_key_raw {};
  std::ifstream private_key_file((diaria_path / "key.key").c_str(),
                                 std::ios::in | std::ios::binary);
  private_key_file.read(reinterpret_cast<char*>(stored_private_key_raw.data()),
                        stored_private_key_raw.size());
  stored_secret_key stored_private_key(stored_private_key_raw);
  return stored_private_key.extract_key(password);
}

auto get_iso_timestamp_utc() -> std::string
{
  // Get current time in UTC
  auto now = std::chrono::system_clock::now();

  // Convert to time_t for easy manipulation
  std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

  // Convert time_t to a tm structure for UTC time
  std::tm utc_tm = *std::gmtime(&now_time_t);

  // Format the timestamp using std::format
  return std::format("{:04}-{:02}-{:02}T{:02}:{:02}:{:02}",
                     utc_tm.tm_year + 1900,
                     utc_tm.tm_mon + 1,
                     utc_tm.tm_mday,
                     utc_tm.tm_hour,
                     utc_tm.tm_min,
                     utc_tm.tm_sec);
}

auto add_entry()
{
  std::filesystem::path diaria_path("/home/lukas/diaria");
  auto file_span = create_entry_interactive();

  auto symkey = load_symkey(diaria_path);
  auto pubkey = load_pubkey(diaria_path);
  auto encrypted = encrypt(symkey, pubkey, file_span.getSpan());

  std::ofstream entry_file(
      (diaria_path / std::format("{}.dia", get_iso_timestamp_utc())).c_str(),
      std::ios::out | std::ios::binary | std::ios::trunc);
  entry_file.write(reinterpret_cast<const char*>(encrypted.data()),
                   encrypted.size());
}

auto read_entry(const std::filesystem::path& entry)
{
  std::filesystem::path diaria_path("/home/lukas/diaria");

  std::string password;
  std::print("Enter password: ");
  std::getline(std::cin, password);
  const auto entry_fd = open(entry.c_str(), O_RDONLY | O_CLOEXEC);
  mmap_file file_span(entry_fd);

  auto symkey = load_symkey(diaria_path);

  auto private_key = load_private_key(diaria_path, password);
  auto decrypted = decrypt(symkey, private_key, file_span.getSpan());
  std::print("{}", std::string(decrypted.begin(), decrypted.end()));
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
  {
    std::string entry_path {};
    CLI::App* subcom_read =
        app.add_subcommand("read", "Read a diary entry")
            ->final_callback([&entry_path]() { read_entry(entry_path); });
    subcom_read->add_option("path", entry_path, "Path to entry")->required();
  }
  CLI11_PARSE(app, argc, argv);
  return 0;
}
