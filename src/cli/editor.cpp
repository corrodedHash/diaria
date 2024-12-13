#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <memory>
#include <print>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "./editor.hpp"

#include <fcntl.h>
#include <linux/capability.h>
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
  const auto result =
      owned_word_span | std::ranges::to<std::vector<std::string>>();

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

void exec_cmdline(std::string_view cmdline)
{
  const auto words = build_argv(cmdline);
  const auto word_pointers = words
      | std::views::transform([](auto& word)
                              { return const_cast<char*>(word.data()); });
  auto argv = word_pointers | std::ranges::to<std::vector<char*>>();
  argv.push_back(nullptr);

  const auto exec_result = execvp(argv[0], argv.data());
  throw std::runtime_error(std::format("Error during exec: {}", exec_result));
}

auto interactive_content_entry(std::string_view cmdline,
                               const std::filesystem::path& temp_file_dir)
    -> std::vector<unsigned char>
{
  auto temp_file_path = temp_file_dir / "diaria_XXXXXX";
  const auto child_pid = fork();
  if (child_pid == 0) {
    auto owned_cmdline = std::string(cmdline);
    replace_first(owned_cmdline, "%", temp_file_path.c_str());
    exec_cmdline(owned_cmdline);
  }

  int child_status {};
  waitpid(child_pid, &child_status, 0);

  std::ifstream stream(temp_file_path, std::ios::in | std::ios::binary);
  std::vector<unsigned char> contents((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());
  if (stream.fail()) {
    std::println(
        stderr,
        "Error reading diary file. Unencrypted entry is still stored at {}",
        temp_file_path.c_str());
    throw std::runtime_error("Could not read diary entry file");
  }
  if (contents.empty()) {
    std::println(stderr,
                 "Diary entry empty, not saving. Temporary file remains at {}",
                 temp_file_path.c_str());
    throw std::runtime_error("Could not create diary entry file");
  }
  unlink(temp_file_path.c_str());
  return contents;
}

void write_to_file(const std::filesystem::path& path, const std::string& data)
{
  const smart_fd file_descriptor {
      open(path.c_str(), O_WRONLY | O_NOCTTY | O_CLOEXEC)};
  if (file_descriptor.fd == -1) {
    throw std::runtime_error(std::format("Could not open {}", path.c_str()));
  }
  if (write(file_descriptor.fd, data.c_str(), data.size()) == -1) {
    throw std::runtime_error(std::format("Could not write to {}; Errno {} [{}]",
                                         path.c_str(),
                                         errno,
                                         strerror(errno)));
  }
}

void set_uid_map()
{
  uid_t uid = getuid();
  gid_t gid = getgid();

  auto proc_path = std::filesystem::path {"/proc"} / "self";

  auto setgroups_path = proc_path / "setgroups";

  write_to_file(setgroups_path, "deny\n");

  auto gid_map_path = proc_path / "gid_map";
  write_to_file(gid_map_path, std::format("0 {} 1\n", gid));

  auto uid_map_path = proc_path / "uid_map";
  write_to_file(uid_map_path, std::format("0 {} 1\n", uid));
}

struct editor_args
{
  std::string_view cmdline;
  int tx_fd;
};

void write_to_pipe(int pipe_fd, std::span<unsigned char> content)
{
  auto left_to_write = std::span(content);
  while (!left_to_write.empty()) {
    const ssize_t written =
        write(pipe_fd, left_to_write.data(), left_to_write.size());
    if (written == -1) {
      throw std::runtime_error("Writing to pipe");
    }
    left_to_write = left_to_write.subspan(static_cast<std::uint64_t>(written));
  }
}

auto read_from_pipe(int pipe_fd) -> std::vector<unsigned char>
{
  std::array<unsigned char, 4096> buffer {};
  std::vector<unsigned char> result {};
  ssize_t bytes_read {};

  while ((bytes_read = read(pipe_fd, buffer.data(), buffer.size())) > 0) {
    const auto bufferspan =
        std::span(buffer.begin(), static_cast<std::size_t>(bytes_read));

    result.insert(result.end(), bufferspan.begin(), bufferspan.end());
  }

  if (bytes_read == -1) {
    throw std::runtime_error("Reading from pipe");
  }
  return result;
}

auto editor_in_private_namespace(void* arg_raw) -> int
{
  auto* arg = static_cast<editor_args*>(arg_raw);
  set_uid_map();

  constexpr std::string_view mount_dir {"/tmp/diaria"};

  std::error_code mkdir_err {};
  std::filesystem::create_directory(mount_dir, mkdir_err);
  if (mkdir_err && mkdir_err.value() != EEXIST) {
    throw std::runtime_error("Could not create temporary directory for entry");
  }

  const std::string mount_args = "noswap";
  if (mount("tmpfs", mount_dir.data(), "tmpfs", 0, mount_args.c_str()) == -1) {
    throw std::runtime_error("Could not mount tmpfs");
  }

  auto content = interactive_content_entry(arg->cmdline, mount_dir);

  // Unmount the tmpfs
  if (umount(mount_dir.data()) == -1) {
    throw std::runtime_error("Could not unmount tmpfs");
  }

  write_to_pipe(arg->tx_fd, content);

  return 0;
}

auto private_namespace_read(std::string_view cmdline)
    -> std::vector<unsigned char>
{
  std::array<int, 2> pipefd {};  // File descriptors for the pipe

  if (pipe2(pipefd.data(), O_CLOEXEC) == -1) {
    throw std::runtime_error("Could not create unnamed pipe");
  }

  constexpr uint64_t stack_size {1024L * 1024};
  auto stack = std::make_unique<std::array<char, stack_size>>();
  char* stack_top = stack->data() + stack->size();

  editor_args args {cmdline, pipefd[1]};
  pid_t const pid = clone(editor_in_private_namespace,
                          stack_top,
                          CLONE_NEWNS | CLONE_NEWUSER | SIGCHLD,
                          &args);
  if (pid == -1) {
    throw std::runtime_error("Could not clone");
  }

  auto content = read_from_pipe(pipefd[0]);

  // Wait for the child process to complete
  if (waitpid(pid, nullptr, 0) == -1) {
    throw std::runtime_error("Child process did not finish cleanly");
  }
  return content;
}
