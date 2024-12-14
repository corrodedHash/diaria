cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/prelude.cmake)

project(
    diaria
    VERSION 0.2.3
    DESCRIPTION "Tool to manage diary entries"
    HOMEPAGE_URL "https://github.com/corrodedHash/diaria"
    LANGUAGES CXX
)
include(cmake/hardening.cmake)
include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)


# ---- Developer mode ----

if(DIARIA_DEVELOPER_MODE)
  if(NOT PROJECT_IS_TOP_LEVEL)
    message(
        AUTHOR_WARNING
        "Developer mode is intended for developers of executable"
    )
  endif()
  include(cmake/dev-mode.cmake)
endif()

# ---- Includes ----

configure_file(src/project_info.hpp.in generated/project_info.hpp @ONLY)
add_library(diaria_project_info INTERFACE)
target_include_directories(diaria_project_info INTERFACE ${PROJECT_BINARY_DIR}/generated)

add_subdirectory(external)
add_subdirectory(src)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()


