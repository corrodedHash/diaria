# ---- Declare library ----

add_library(
    crypto_lib OBJECT
    secret_key.cpp
    entry.cpp
    compress.cpp
)

target_include_directories(
    crypto_lib ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)

target_link_libraries(
  crypto_lib PUBLIC libsodium liblzma INTERFACE diaria_hardening
)

target_compile_features(crypto_lib PUBLIC cxx_std_23)
