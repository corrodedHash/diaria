#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string_view>

#include "./repo.hpp"

#include <fcntl.h>

#include "./util.hpp"
#include "common.hpp"
#include "crypto/entry.hpp"
#include "mmap_file.hpp"

void dump_repo(const key_path_t& keypath,
               const repo_path_t& repo,
               const std::filesystem::path& target,
               std::string_view password)
{
  const auto private_key =
      load_private_key(keypath.get_private_key_path(), password);
  const auto symkey = load_symkey(keypath.get_symkey_path());

  std::filesystem::create_directories(target);

  for (const auto& entry :
       std::filesystem::directory_iterator(repo.repo)
           | std::ranges::views::filter([](auto entry)
                                        { return entry.is_regular_file(); })
           | std::ranges::views::filter(
               [](auto entry) {
                 return entry.path().filename().string().ends_with(".diaria");
               }))
  {
    const owned_fd entry_fd(entry);
    const owned_mmap file_span(entry_fd);

    const auto decrypted = decrypt(symkey, private_key, file_span.get_span());
    const auto output_file_name =
        entry.path().filename().replace_extension("txt");
    std::ofstream entry_file(
        (target / output_file_name).c_str(),
        std::ios::out | std::ios::binary | std::ios::trunc);
    entry_file.write(make_signed_char(decrypted.data()),
                     static_cast<std::streamsize>(decrypted.size()));
  }
}
void load_repo(const key_path_t& keypath,
               const repo_path_t& repo,
               const std::filesystem::path& source)
{
  const auto pubkey = load_pubkey(keypath.get_pubkey_path());
  const auto symkey = load_symkey(keypath.get_symkey_path());

  for (const auto& entry : std::filesystem::directory_iterator(source)
           | std::ranges::views::filter([](auto entry)
                                        { return entry.is_regular_file(); }))
  {
    const owned_fd entry_fd(entry);
    const owned_mmap file_span(entry_fd);

    const auto decrypted = encrypt(symkey, pubkey, file_span.get_span());
    const auto output_file_name =
        entry.path().filename().replace_extension("diaria");
    std::ofstream entry_file(
        (repo.repo / output_file_name).c_str(),
        std::ios::out | std::ios::binary | std::ios::trunc);
    entry_file.write(make_signed_char(decrypted.data()),
                     static_cast<std::streamsize>(decrypted.size()));
  }
}

void sync_repo_git(const repo_path_t& repo)
{
  auto workingdir = std::filesystem::current_path();
  std::filesystem::current_path(repo.repo);
  system("git add *.diaria");
  system("git commit -m \"Added entry\"");
  system("git push");
  system("git pull");
  std::filesystem::current_path(workingdir);
}

void sync_repo(const repo_path_t& repo)
{
  if (!std::filesystem::exists(repo.repo)) {
    throw std::runtime_error("Repository does not exist");
  }

  if (std::filesystem::exists(repo.repo / ".git")) {
    sync_repo_git(repo);
    return;
  }
  std::print(stderr, "No sync mechanism found in diaria repository. No action");
}
