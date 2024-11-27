#pragma once
#include "diaria/command_types.hpp"
#include "diaria/key_management.hpp"

void setup_db(const key_repo_paths_t& keypath,
              std::unique_ptr<password_provider> password);
