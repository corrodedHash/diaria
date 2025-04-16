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

#include "crypto/safe_buffer.hpp"
#include "util/smart_fd.hpp"

namespace
{
auto build_argv(const std::string& cmdline)
{
  wordexp_t words {};
  // NOLINTNEXTLINE(hicpp-signed-bitwise)
  wordexp(cmdline.c_str(), &words, WRDE_NOCMD | WRDE_UNDEF | WRDE_SHOWERR);

  const auto word_span = std::span(words.we_wordv, words.we_wordc);
  const auto owned_word_span = word_span
      | std::ranges::views::transform([](auto word)
                                      { return std::string(word); });
  const auto result =
      owned_word_span | std::ranges::to<std::vector<std::string>>();

  wordfree(&words);
  return result;
}

[[nodiscard]]
auto replace_first(std::string_view input_string,
                   std::string_view to_replace,
                   std::string_view replace_with) -> std::string
{
  const std::size_t pos = input_string.find(to_replace);
  if (pos == std::string::npos) {
    return std::string(input_string);
  }
  auto owned_string = std::string(input_string);
  owned_string.replace(pos, to_replace.length(), replace_with);
  return owned_string;
}

void exec_cmdline(std::string_view cmdline)
{
  const auto words = build_argv(std::string(cmdline));
  const auto word_pointers = words
      | std::views::transform([](auto& word)
                              { return const_cast<char*>(word.data()); });
  auto argv = word_pointers | std::ranges::to<std::vector<char*>>();
  argv.push_back(nullptr);

  const auto exec_result = execvp(argv[0], argv.data());
  throw std::runtime_error(std::format("Error during exec: {}", exec_result));
}
}  // namespace

auto interactive_content_entry(std::string_view cmdline,
                               const std::filesystem::path& temp_file_dir)
    -> safe_vector<unsigned char>
{
  auto temp_file_path = temp_file_dir / "diaria_XXXXXX";
  const auto child_pid = fork();
  if (child_pid == 0) {
    const auto owned_cmdline =
        replace_first(cmdline, "%", temp_file_path.c_str());
    exec_cmdline(owned_cmdline);
  }

  int child_status {};
  waitpid(child_pid, &child_status, 0);

  if (child_status != 0) {
    std::println(stderr,
                 "Editor did not terminate successfully. Temporary entry still "
                 "stored at {}",
                 temp_file_path.c_str());
    throw std::runtime_error("Executing editor");
  }

  std::ifstream stream(temp_file_path, std::ios::in | std::ios::binary);
  safe_vector<unsigned char> contents((std::istreambuf_iterator<char>(stream)),
                                      std::istreambuf_iterator<char>());
  if (stream.fail()) {
    std::println(
        stderr,
        "Error reading diary file. Unencrypted entry is still stored at {}",
        temp_file_path.c_str());
    throw std::runtime_error("Reading temporary diary entry");
  }
  unlink(temp_file_path.c_str());
  return contents;
}

struct editor_args
{
  std::string_view cmdline;
  uid_t parent_uid;
  gid_t parent_gid;
  int tx_fd;
};
namespace
{
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

void set_uid_map(uid_t uid, gid_t gid)
{
  auto proc_path = std::filesystem::path {"/proc"} / "self";

  write_to_file(proc_path / "setgroups", "deny\n");
  write_to_file(proc_path / "gid_map", std::format("0 {} 1\n", gid));
  write_to_file(proc_path / "uid_map", std::format("0 {} 1\n", uid));
}

void write_all(int pipe_fd, std::span<unsigned char> content)
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

auto read_until_closed(int pipe_fd) -> safe_vector<unsigned char>
{
  constexpr int buffer_size = 4096;
  safe_array<unsigned char, buffer_size> buffer {};
  safe_vector<unsigned char> result {};
  ssize_t bytes_read {};

  while ((bytes_read = read(pipe_fd, buffer.data(), buffer.size())) > 0) {
    const auto bufferspan =
        std::span(buffer.begin(), static_cast<std::size_t>(bytes_read));
    // std::println("{} bytes read", bytes_read);
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
  set_uid_map(arg->parent_uid, arg->parent_gid);

  static constexpr std::string mount_dir {"/tmp/diaria"};

  std::error_code mkdir_err {};
  std::filesystem::create_directory(mount_dir, mkdir_err);
  if (mkdir_err && mkdir_err.value() != EEXIST) {
    throw std::runtime_error("Could not create temporary directory for entry");
  }

  const std::string_view mount_args;
  // TODO: This currently results in an invalid argument error from `mount
  // tmpfs` Prevent tmpfs from swapping const std::string_view mount_args
  // {"noswap"};
  if (mount("tmpfs", mount_dir.c_str(), "tmpfs", 0, mount_args.data()) == -1) {
    throw std::runtime_error(std::format(
        "Could not mount tmpfs; Errno {} [{}]", errno, strerror(errno)));
  }

  auto content = interactive_content_entry(arg->cmdline, mount_dir);

  // Unmount the tmpfs
  if (umount(mount_dir.c_str()) == -1) {
    throw std::runtime_error("Could not unmount tmpfs");
  }

  write_all(arg->tx_fd, content);
  close(arg->tx_fd);
  return 0;
}
}  // namespace

auto private_namespace_read(std::string_view cmdline)
    -> safe_vector<unsigned char>
{
  std::array<int, 2> pipefd {};  // File descriptors for the pipe

  if (pipe2(pipefd.data(), O_CLOEXEC) == -1) {
    throw std::runtime_error("Could not create unnamed pipe");
  }

  constexpr uint64_t stack_size {1024L * 1024};
  auto stack = std::make_unique<std::array<char, stack_size>>();
  char* stack_top = stack->data() + stack->size();

  editor_args args {.cmdline = cmdline,
                    .parent_uid = getuid(),
                    .parent_gid = getgid(),
                    .tx_fd = pipefd[1]};
  pid_t const pid = clone(editor_in_private_namespace,
                          stack_top,
                          CLONE_NEWNS | CLONE_NEWUSER | SIGCHLD,
                          &args);
  if (pid == -1) {
    throw std::runtime_error("Could not clone");
  }
  close(pipefd[1]);

  // std::println("Waiting for content...");
  auto content = read_until_closed(pipefd[0]);
  // std::println("Received content");

  // Wait for the child process to complete
  if (waitpid(pid, nullptr, 0) == -1) {
    throw std::runtime_error("Child process did not finish cleanly");
  }
  return content;
}
