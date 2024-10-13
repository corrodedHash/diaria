#include <filesystem>
#include <fstream>

#include "repo.hpp"

#include "crypto/entry.hpp"
#include "mmap_file.hpp"
#include "util.hpp"
void dump_repo(const key_path_t& keypath,
               const std::filesystem::path& source,
               const std::filesystem::path& target,
               std::string_view password)
{
  const auto private_key =
      load_private_key(keypath.get_private_key_path(), password);
  const auto symkey = load_symkey(keypath.get_symkey_path());

  std::filesystem::create_directories(target);

  for (const auto& entry : std::filesystem::directory_iterator(source)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto input_file_name = entry.path().filename().string();

    if (!input_file_name.ends_with(".diaria")) {
      continue;
    }

    const auto entry_fd = open(entry.path().c_str(), O_RDONLY | O_CLOEXEC);
    const mmap_file file_span(entry_fd);

    const auto decrypted = decrypt(symkey, private_key, file_span.getSpan());
    const auto output_file_name =
        entry.path().filename().replace_extension("txt");
    std::ofstream entry_file(
        (target / output_file_name).c_str(),
        std::ios::out | std::ios::binary | std::ios::trunc);
    entry_file.write(reinterpret_cast<const char*>(decrypted.data()),
                     decrypted.size());
  }
}
void load_repo(const key_path_t& keypath,
               const std::filesystem::path& source,
               const std::filesystem::path& target)
{
  const auto pubkey = load_pubkey(keypath.get_pubkey_path());
  const auto symkey = load_symkey(keypath.get_symkey_path());

  for (const auto& entry : std::filesystem::directory_iterator(source)) {
    if (!entry.is_regular_file()) {
      continue;
    }
    const auto input_file_name = entry.path().filename().string();

    const auto entry_fd = open(entry.path().c_str(), O_RDONLY | O_CLOEXEC);
    const mmap_file file_span(entry_fd);

    const auto decrypted = encrypt(symkey, pubkey, file_span.getSpan());
    const auto output_file_name =
        entry.path().filename().replace_extension("diaria");
    std::ofstream entry_file(
        (target / output_file_name).c_str(),
        std::ios::out | std::ios::binary | std::ios::trunc);
    entry_file.write(reinterpret_cast<const char*>(decrypted.data()),
                     decrypted.size());
  }
}