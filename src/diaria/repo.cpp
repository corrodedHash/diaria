#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string_view>
#include <vector>

#include "./repo.hpp"

#include "./util.hpp"
#include "common.hpp"
#include "crypto/entry.hpp"
#include "crypto/secret_key.hpp"

namespace views = std::ranges::views;

void dump_repo(const key_repo_t& keypath,
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
           | views::filter([](auto entry) { return entry.is_regular_file(); })
           | views::filter(
               [](auto entry) {
                 return entry.path().filename().string().ends_with(".diaria");
               }))
  {
    std::ifstream stream(entry.path(), std::ios::in | std::ios::binary);
    if (stream.fail()) {
      throw std::runtime_error("Could not open entry file");
    }
    std::vector<unsigned char> contents(
        (std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());

    const auto decrypted = decrypt(
        symkey_span_t {symkey}, private_key_span_t {private_key}, contents);
    const auto output_file_name =
        entry.path().filename().replace_extension("txt");
    std::ofstream entry_file(
        (target / output_file_name).c_str(),
        std::ios::out | std::ios::binary | std::ios::trunc);
    if (entry_file.fail()) {
      throw std::runtime_error(std::format("Could not open output file: {}",
                                           output_file_name.c_str()));
    }
    entry_file.write(make_signed_char(decrypted.data()),
                     static_cast<std::streamsize>(decrypted.size()));
  }
}
void load_repo(const key_repo_t& keypath,
               const repo_path_t& repo,
               const std::filesystem::path& source)
{
  const auto pubkey = load_pubkey(keypath.get_pubkey_path());
  const auto symkey = load_symkey(keypath.get_symkey_path());
  std::filesystem::create_directories(repo.repo);

  for (const auto& entry : std::filesystem::directory_iterator(source)
           | views::filter([](auto entry) { return entry.is_regular_file(); }))
  {
    std::ifstream stream(entry.path(), std::ios::in | std::ios::binary);
    if (stream.fail()) {
      throw std::runtime_error("Could not open entry file");
    }
    std::vector<unsigned char> contents(
        (std::istreambuf_iterator<char>(stream)),
        std::istreambuf_iterator<char>());

    const auto decrypted =
        encrypt(symkey_span_t {symkey}, public_key_span_t {pubkey}, contents);
    const auto output_file_name =
        entry.path().filename().replace_extension("diaria");
    std::ofstream entry_file(
        (repo.repo / output_file_name).c_str(),
        std::ios::out | std::ios::binary | std::ios::trunc);
    if (entry_file.fail()) {
      throw std::runtime_error(std::format("Could not open output file: {}",
                                           output_file_name.c_str()));
    }
    entry_file.write(make_signed_char(decrypted.data()),
                     static_cast<std::streamsize>(decrypted.size()));
  }
}

void sync_repo_git(const repo_path_t& repo)
{
  auto workingdir = std::filesystem::current_path();
  std::filesystem::current_path(repo.repo);
  // NOLINTBEGIN(cert-env33-c)
  // TODO: Use libgit2 maybe, or drop git synchronization for something more fit
  // to the task
  std::system("git add *.diaria");
  std::system("git commit -m \"Added entry\"");
  std::system("git push");
  std::system("git pull");
  // NOLINTEND(cert-env33-c)
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
