set(DOXYGEN_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/docs")

# Needs clang
# add_compile_options(-Wdocumentation)

configure_file("docs/Doxyfile.in" "${PROJECT_BINARY_DIR}/docs/Doxyfile" @ONLY)
find_package(Doxygen)

add_custom_target(
    gendocs
    COMMAND doxygen
    WORKING_DIRECTORY docs/
    )