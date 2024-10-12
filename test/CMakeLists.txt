# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(executableTests LANGUAGES CXX)

# ---- Tests ----

add_executable(private_key_test src/private_key_test.cpp)
target_link_libraries(private_key_test PRIVATE crypto_lib)
target_compile_features(private_key_test PRIVATE cxx_std_23)
add_test(NAME private_key_test COMMAND private_key_test)

add_executable(entry_test src/entry_test.cpp)
target_link_libraries(entry_test PRIVATE crypto_lib)
target_compile_features(entry_test PRIVATE cxx_std_23)
add_test(NAME entry_test COMMAND entry_test)

add_executable(crypto_primitives_test src/crypto_primitives_test.cpp)
target_link_libraries(crypto_primitives_test PRIVATE crypto_lib)
target_compile_features(crypto_primitives_test PRIVATE cxx_std_23)
add_test(NAME crypto_primitives_test COMMAND crypto_primitives_test)



# ---- End-of-file commands ----

add_folders(Test)
