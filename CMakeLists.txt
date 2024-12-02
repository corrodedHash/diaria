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


include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Developer mode ----

if(executable_DEVELOPER_MODE)
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

# ---- Declare executable ----

add_executable(executable_exe src/main.cpp src/cli_commands.cpp)
add_executable(executable::exe ALIAS executable_exe)

set_property(TARGET executable_exe PROPERTY OUTPUT_NAME diaria)

target_compile_features(executable_exe PRIVATE cxx_std_23)

target_link_libraries(executable_exe PRIVATE crypto_lib diaria_lib)
target_include_directories(executable_exe SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/external)

configure_file(src/project_info.hpp.in generated/project_info.hpp @ONLY)
target_include_directories(executable_exe PRIVATE ${PROJECT_BINARY_DIR}/generated)

if (BUILD_STATIC_BINARY)
target_link_options(executable_exe PRIVATE -static-libgcc -static-libstdc++)
endif()


# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()


