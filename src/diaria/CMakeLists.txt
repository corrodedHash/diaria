# ---- Declare library ----

add_library(
    diaria_lib OBJECT
    key_management.cpp
    repo_management.cpp
    commands/summarize.cpp
    commands/repo.cpp
    commands/stats.cpp
    commands/add_entry.cpp
    commands/init.cpp
    commands/read_entry.cpp
)

target_include_directories(
    diaria_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

target_compile_features(diaria_lib PUBLIC cxx_std_23)
