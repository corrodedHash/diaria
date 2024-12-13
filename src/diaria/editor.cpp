#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "./editor.hpp"

#include <fcntl.h>
#include <sched.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <wordexp.h>

#include "util/smart_fd.hpp"

auto build_argv(std::string_view cmdline)
{
  wordexp_t words {};
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  wordexp(cmdline.data(), &words, WRDE_NOCMD | WRDE_UNDEF | WRDE_SHOWERR);

  const auto word_span = std::span(words.we_wordv, words.we_wordc);
  const auto owned_word_span = word_span
      | std::ranges::views::transform([](auto word)
                                      { return std::string(word); });
  std::vector<std::string> result(std::begin(owned_word_span),
                                  std::end(owned_word_span));

  wordfree(&words);
  return result;
}

void replace_first(std::string& input_string,
                   std::string_view to_replace,
                   std::string_view replace_with)
{
  const std::size_t pos = input_string.find(to_replace);
  if (pos == std::string::npos) {
    return;
  }
  input_string.replace(pos, to_replace.length(), replace_with);
}

void start_editor(std::string_view cmdline,
                  const std::filesystem::path& temp_entry_path)
{
  auto owned_cmdline = std::string(cmdline);
  replace_first(owned_cmdline, "%", temp_entry_path.c_str());

  const auto words = build_argv(owned_cmdline);
  std::vector<char*> argv {};
  argv.reserve(words.size());
  for (const auto& word : words) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    argv.push_back(const_cast<char*>(word.data()));
  }
  argv.push_back(nullptr);
  const auto exec_result = execvp(argv[0], argv.data());
  throw std::runtime_error(
      std::format("Error during exec editor: {}", exec_result));
}

// Helper function to write to a file
void write_to_file(const std::filesystem::path& path, const std::string& data)
{
  smart_fd file_descriptor {open(path.c_str(), O_WRONLY | O_CLOEXEC)};
  if (file_descriptor.fd == -1) {
    throw std::runtime_error(std::format("Could not open {}", path.c_str()));
  }
  if (write(file_descriptor.fd, data.c_str(), data.size()) == -1) {
    throw std::runtime_error(
        std::format("Could not write to {}", path.c_str()));
  }
}

void set_uid_map()
{
  // Map root (inside the user namespace) to the current user
  uid_t uid = getuid();
  gid_t gid = getgid();

  auto proc_path = std::filesystem::path {"/proc"} / std::to_string(getpid());
  // Write UID map
  auto uid_map_path = proc_path / "uid_map";

  write_to_file(uid_map_path, std::format("{} {} 1\n", uid, uid));

  // Write GID map
  auto setgroups_path = proc_path / "setgroups";

  if (access(setgroups_path.c_str(), F_OK) == 0) {  // Check if setgroups exists
    write_to_file(setgroups_path, "deny\n");
  }

  auto gid_map_path = proc_path / "gid_map";

  write_to_file(gid_map_path, std::format("{} {} 1\n", gid, gid));
}

struct editor_args
{
  std::string_view cmdline;
};

auto editor_in_private_namespace(void* arg_raw) -> int
{
  auto* arg = static_cast<editor_args*>(arg_raw);
  set_uid_map();

  constexpr std::string_view mount_dir {"/tmp/myns_mount"};

  std::error_code mkdir_err {};
  std::filesystem::create_directory(mount_dir, mkdir_err);
  if (mkdir_err && mkdir_err.value() != EEXIST) {
    throw std::runtime_error("Could not create temporary directory for entry");
  }

  std::string mount_args = "noswap";
  if (mount("tmpfs", mount_dir.data(), "tmpfs", 0, mount_args.c_str()) == -1) {
    throw std::runtime_error("Could not mount tmpfs");
  }

  start_editor(arg->cmdline, std::filesystem::path {mount_dir} / "entry");

  // Unmount the tmpfs
  if (umount(mount_dir.data()) == -1) {
    throw std::runtime_error("Could not unmount tmpfs");
  }

  return 0;
}

auto private_namespace_read(std::string_view cmdline)
    -> std::vector<unsigned char>
{
  constexpr uint64_t stack_size(1024L * 1024);

  // Allocate stack for the child process

  auto stack = std::make_unique<std::array<char, stack_size>>();

  char* stack_top = stack->data() + stack->size();

  // Create child process with clone
  editor_args args {cmdline};
  pid_t pid = clone(editor_in_private_namespace,
                    stack_top,
                    CLONE_NEWNS | CLONE_NEWUSER | SIGCHLD,
                    &args);
  if (pid == -1) {
    throw std::runtime_error("Could not clone");
  }

  // Wait for the child process to complete
  if (waitpid(pid, nullptr, 0) == -1) {
    throw std::runtime_error("Child process did not finish cleanly");
  }
}
