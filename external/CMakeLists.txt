option(BUILD_STATIC_BINARY "Build diaria with statically linked libraries" OFF)
if (BUILD_STATIC_BINARY)
    include(ExternalProject)
    include(FetchContent)
    include(FindPkgConfig)
endif()

add_subdirectory(libsodium)
add_subdirectory(liblzma)