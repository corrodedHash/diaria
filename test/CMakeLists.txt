# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location


find_package(Catch2 3 REQUIRED)

project(executableTests LANGUAGES CXX)

add_executable(unit_tests src/crypto_primitives_test.cpp src/entry_test.cpp src/private_key_test.cpp)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain crypto_lib diaria_lib)
target_compile_features(unit_tests PRIVATE cxx_std_23)

include(CTest)
include(Catch)
catch_discover_tests(unit_tests)


add_subdirectory("end-to-end")