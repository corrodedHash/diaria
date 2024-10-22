# ---- Declare library ----

add_library(
    diaria_lib OBJECT
    commands.cpp repo.cpp util.cpp
    commands/add_entry.cpp
    repo/summarize.cpp
)

target_include_directories(
    diaria_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

target_compile_features(diaria_lib PUBLIC cxx_std_23)
