cmake_minimum_required(VERSION 3.14)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(cmake/prelude.cmake)

project(
    diaria
    VERSION 0.2.2
    DESCRIPTION "Tool to manage diary entries"
    HOMEPAGE_URL "https://github.com/corrodedHash/diaria"
    LANGUAGES CXX
)

add_library(diaria_hardening INTERFACE)
target_compile_definitions(diaria_hardening INTERFACE
  -D_GLIBCXX_ASSERTIONS=1)

target_compile_options(diaria_hardening INTERFACE
  -fstack-protector-strong
  -fcf-protection=full -fstack-clash-protection -Wall -Wextra -Wpedantic
  -Wconversion -Wsign-conversion -Wcast-qual -Wformat=2 -Wundef -Werror=float-equal
  -Wshadow -Wcast-align -Wunused -Wnull-dereference -Wdouble-promotion -Wimplicit-fallthrough
  -Wextra-semi -Woverloaded-virtual -Wnon-virtual-dtor -Wold-style-cast)

target_link_options(diaria_hardening INTERFACE
  -Wl,--allow-shlib-undefined,--as-needed,-z,noexecstack,-z,relro,-z,now,-z,nodlopen)

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

add_subdirectory(external)
add_subdirectory(src)

configure_file(src/project_info.hpp.in generated/project_info.hpp @ONLY)


# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()


