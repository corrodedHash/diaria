# ---- Declare library ----

add_executable(diaria_tui
    main.cpp
    )

set_property(TARGET diaria_tui PROPERTY OUTPUT_NAME diaria-tui
 
  )

target_compile_features(diaria_tui PRIVATE cxx_std_23)

target_link_libraries(diaria_tui
    PRIVATE crypto_lib
    PRIVATE diaria_hardening
    PRIVATE diaria_project_info
    PRIVATE CLI11
    PRIVATE ftxui::screen
    PRIVATE ftxui::dom
    PRIVATE ftxui::component)



if (BUILD_STATIC_BINARY)
    target_link_options(diaria_tui PRIVATE -static-libgcc -static-libstdc++)
endif()

target_include_directories(
    diaria_tui ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

target_compile_features(diaria_tui PUBLIC cxx_std_23)
