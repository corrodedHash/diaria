#pragma once
#include <cstdlib>
#include <filesystem>
#include <print>

#include <pwd.h>
#include <unistd.h>

struct xdg_paths
{
  std::filesystem::path data_home;
  std::filesystem::path config_home;
  xdg_paths()
  {
    struct passwd* pw_entry = getpwuid(getuid());
    const std::filesystem::path homedir(pw_entry->pw_dir);
    auto* const xdg_data_home_raw = std::getenv("XDG_DATA_HOME");
    auto* const xdg_config_home_raw = std::getenv("XDG_CONFIG_HOME");
    data_home = [&]()
    {
      if (xdg_data_home_raw == nullptr) {
        return homedir / ".local" / "share";
      }
      return std::filesystem::path(xdg_data_home_raw);
    }();
    config_home = [&]()
    {
      if (xdg_config_home_raw == nullptr) {
        return homedir / ".config";
      }
      return std::filesystem::path(xdg_config_home_raw);
    }();
  }
};