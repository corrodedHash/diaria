#pragma once
#include <memory>

#include "diaria/command_types.hpp"

void summarize_repo(std::unique_ptr<entry_decryptor_initializer> keys,
                    const repo_path_t& repo,
                    bool paging);