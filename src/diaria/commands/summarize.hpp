#pragma once
#include "diaria/command_types.hpp"
#include "diaria/key_management.hpp"

void summarize_repo(const key_repo_t& keypath,
                    const repo_path_t& repo,
                    bool paging);