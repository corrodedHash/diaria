

add_custom_target(
    coverage_raw
    COMMAND ${CMAKE_COMMAND} -E rm -Rf -- ${CMAKE_BINARY_DIR}/coverage_raw
    COMMAND ${CMAKE_COMMAND} -E env LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage_raw/coverage.%p.profraw ${CMAKE_CTEST_COMMAND}
    COMMENT "Testing to generate coverage traces"
)

add_custom_target(
    coverage_merge
    COMMAND sh -c "llvm-profdata merge --sparse ${CMAKE_BINARY_DIR}/coverage_raw/*.profraw -o ${CMAKE_BINARY_DIR}/coverage.profdata"
    DEPENDS coverage_raw
    BYPRODUCTS ${CMAKE_BINARY_DIR}/coverage.profdata
    COMMENT "Merging traces into profile"
)

add_custom_target(
    coverage
    DEPENDS coverage_merge
    COMMAND llvm-cov show --format=html ${CMAKE_BINARY_DIR}/src/cli/diaria --instr-profile ${CMAKE_BINARY_DIR}/coverage.profdata -o ${CMAKE_BINARY_DIR}/coverage_html
    COMMAND llvm-cov report ${CMAKE_BINARY_DIR}/src/cli/diaria --instr-profile ${CMAKE_BINARY_DIR}/coverage.profdata
    COMMENT "Generating coverage report"
    VERBATIM
)
