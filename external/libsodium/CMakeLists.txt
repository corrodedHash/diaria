if (BUILD_STATIC_BINARY)
    FetchContent_Declare(libsodium_tarball
        URL https://download.libsodium.org/libsodium/releases/libsodium-1.0.20-stable.tar.gz
	# Package changes, we cannot used hash to verify
	# URL_HASH SHA1=c79ff91090e116e46e53d2989c701a30d808c025
        DOWNLOAD_EXTRACT_TIMESTAMP ON
    )

    FetchContent_MakeAvailable(libsodium_tarball)

    # Configure
    execute_process(
        COMMAND ${libsodium_tarball_SOURCE_DIR}/configure --prefix ${libsodium_tarball_BINARY_DIR}/dist
        WORKING_DIRECTORY ${libsodium_tarball_BINARY_DIR}
    )

    add_custom_target(
        libsodium_compile
        make
        WORKING_DIRECTORY ${libsodium_tarball_BINARY_DIR}
    )
    add_custom_target(
        libsodium_install
        make install
        DEPENDS libsodium_compile
        WORKING_DIRECTORY ${libsodium_tarball_BINARY_DIR}
        BYPRODUCTS ${libsodium_tarball_BINARY_DIR}/dist/lib/libsodium.a
    )

    add_library(libsodium STATIC IMPORTED GLOBAL)
    add_dependencies(libsodium libsodium_install)
    set_target_properties(libsodium PROPERTIES IMPORTED_LOCATION ${libsodium_tarball_BINARY_DIR}/dist/lib/libsodium.a)
else()
    add_library(libsodium INTERFACE)
    target_link_libraries(
        libsodium INTERFACE -lsodium
    )
endif()
