if (BUILD_STATIC_BINARY)
    ExternalProject_Add(
        libsodium_tarball
        URL https://download.libsodium.org/libsodium/releases/libsodium-1.0.20-stable.tar.gz
        URL_HASH SHA1=36ae0c6c740e477606b277fe41b983dacb26f935
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        CONFIGURE_COMMAND ${CMAKE_CURRENT_BINARY_DIR}/libsodium_tarball-prefix/src/libsodium_tarball/configure
        INSTALL_COMMAND ""

        BUILD_COMMAND make
    )

    add_library(libsodium STATIC IMPORTED GLOBAL)
    set_target_properties(libsodium PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/libsodium_tarball-prefix/src/libsodium_tarball-build/src/libsodium/.libs/libsodium.a)
else()
    add_library(libsodium INTERFACE)
    target_link_libraries(
        libsodium INTERFACE -lsodium
    )
endif()