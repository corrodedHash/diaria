#pragma once
#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <span>  // For std::span
#include <stdexcept>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>  // For fstat
#include <unistd.h>

struct owned_fd
{
  int fd {};

  owned_fd(const owned_fd&) = delete;
  owned_fd(owned_fd&& other) noexcept
      : fd(other.fd)
  {
    other.fd = -1;
  }
  auto operator=(const owned_fd&) -> owned_fd& = delete;
  auto operator=(owned_fd&& other) noexcept -> owned_fd&
  {
    fd = other.fd;
    other.fd = -1;
    return *this;
  }
  explicit owned_fd(int my_fd)
      : fd(my_fd)
  {
  }
  explicit owned_fd(const std::filesystem::path& path)
      // NOLINTNEXTLINE(hicpp-vararg, cppcoreguidelines-pro-type-vararg)
      : fd(open(path.c_str(), O_RDONLY | O_CLOEXEC))
  {
  }
  ~owned_fd()
  {
    if (fd >= 0) {
      close(fd);
    }
  }
};

class owned_mmap
{
  void* mapped_region;
  size_t size;

public:
  owned_mmap(const owned_mmap&) = delete;
  owned_mmap(owned_mmap&& other) noexcept
      : mapped_region(other.mapped_region)
      , size(other.size)
  {
    other.mapped_region = nullptr;
    other.size = 0;
  }
  auto operator=(const owned_mmap&) -> owned_mmap& = delete;
  auto operator=(owned_mmap&& other) noexcept -> owned_mmap&
  {
    mapped_region = other.mapped_region;
    size = other.size;
    other.mapped_region = nullptr;
    other.size = 0;
    return *this;
  }
  explicit owned_mmap(const owned_fd& fides)
  {
    struct stat file_stat
    {
    };
    if (fstat(fides.fd, &file_stat) == -1) {
      throw std::runtime_error("Failed to get file size");
    }
    assert(file_stat.st_size >= 0);
    size = static_cast<size_t>(file_stat.st_size);
    mapped_region = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fides.fd, 0);
    if (mapped_region == MAP_FAILED) {
      auto* error_message = strerror(errno);
      throw std::runtime_error(
          std::format("Failed to mmap file:\n{}", error_message));
    }
  }
  [[nodiscard]] auto get_span() const
  {
    return std::span<const unsigned char>(
        static_cast<unsigned char*>(mapped_region), size);
  }
  ~owned_mmap()
  {
    if (mapped_region != MAP_FAILED && mapped_region != nullptr) {
      munmap(mapped_region, size);
    }
  }
};

struct mmap_file
{
  owned_fd fd;
  owned_mmap mmap;
};