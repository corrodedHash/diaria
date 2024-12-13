#pragma once
#include "cli/command_types.hpp"
#include "cli/key_management.hpp"

void setup_db(const key_repo_paths_t& keypath,
              std::unique_ptr<password_provider> password);
