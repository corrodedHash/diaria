include(FetchContent)

FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v5.0.0
  PATCH_COMMAND git apply ${CMAKE_CURRENT_SOURCE_DIR}/cmake_fix.patch || true
)

FetchContent_MakeAvailable(ftxui)
set(FTXUI_DEV_WARNINGS "NO")