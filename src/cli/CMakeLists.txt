# ---- Declare library ----



add_executable(diaria_cli
    cli_commands.cpp
    command_types.cpp
    commands/add_entry.cpp
    commands/init.cpp
    commands/read_entry.cpp
    commands/repo.cpp
    commands/stats.cpp
    commands/summarize.cpp
    editor.cpp
    key_management.cpp
    main.cpp
    repo_management.cpp
    )

set_property(TARGET diaria_cli PROPERTY OUTPUT_NAME diaria)

target_compile_features(diaria_cli PRIVATE cxx_std_23)

target_link_libraries(diaria_cli PRIVATE crypto_lib INTERFACE diaria_hardening)
target_include_directories(diaria_cli SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/external)
target_include_directories(diaria_cli PRIVATE ${PROJECT_BINARY_DIR}/generated)


if (BUILD_STATIC_BINARY)
    target_link_options(diaria_cli PRIVATE -static-libgcc -static-libstdc++)
endif()

target_include_directories(
    diaria_cli ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

target_compile_features(diaria_cli PUBLIC cxx_std_23)
