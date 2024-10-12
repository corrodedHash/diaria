cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    diaria
    VERSION 0.1.0
    DESCRIPTION "Tool to manage diary entries"
    HOMEPAGE_URL "https://github.com/corrodedHash/diaria"
    LANGUAGES CXX
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

add_subdirectory(src/crypto)

# ---- Declare executable ----

add_executable(executable_exe src/main.cpp src/mmap_file.cpp)
add_executable(executable::exe ALIAS executable_exe)

set_property(TARGET executable_exe PROPERTY OUTPUT_NAME executable)

target_compile_features(executable_exe PRIVATE cxx_std_23)

target_link_libraries(executable_exe PRIVATE crypto_lib)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT executable_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of executable"
  )
endif()

include(cmake/dev-mode.cmake)
