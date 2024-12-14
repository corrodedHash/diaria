# ---- Declare library ----

add_executable(diaria_tui
    main.cpp
    )

set_property(TARGET diaria_tui PROPERTY OUTPUT_NAME diaria-tui
 
  )

target_compile_features(diaria_tui PRIVATE cxx_std_23)

target_link_libraries(diaria_tui
    PRIVATE crypto_lib
    INTERFACE diaria_hardening
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component)

target_include_directories(diaria_tui SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/external)
target_include_directories(diaria_tui PRIVATE ${PROJECT_BINARY_DIR}/generated)


if (BUILD_STATIC_BINARY)
    target_link_options(diaria_tui PRIVATE -static-libgcc -static-libstdc++)
endif()

target_include_directories(
    diaria_tui ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

target_compile_features(diaria_tui PUBLIC cxx_std_23)
