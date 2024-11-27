#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <print>
#include <ranges>
#include <stdexcept>
#include <vector>

#include "./repo.hpp"

#include <sodium.h>

#include "common.hpp"
#include "diaria/command_types.hpp"

namespace views = std::ranges::views;

void dump_repo(std::unique_ptr<entry_decryptor_initializer> keys,
               const repo_path_t& repo,
               const std::filesystem::path& target)
{
  const auto decryptor = keys->init();
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

    const auto decrypted = decryptor.decrypt(contents);
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
void load_repo(std::unique_ptr<entry_encryptor_initializer> keys,
               const repo_path_t& repo,
               const std::filesystem::path& source)
{
  const auto encryptor = keys->init();
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

    const auto decrypted = encryptor.encrypt(contents);
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
