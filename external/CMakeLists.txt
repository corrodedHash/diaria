option(BUILD_STATIC_BINARY "Build diaria with statically linked libraries" OFF)
if (BUILD_STATIC_BINARY)
    include(ExternalProject)
endif()

add_subdirectory(libsodium)
add_subdirectory(liblzma)