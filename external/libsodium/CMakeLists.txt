if (BUILD_STATIC_BINARY)
    ExternalProject_Add(libsodium_tarball
        URL https://download.libsodium.org/libsodium/releases/libsodium-1.0.20-stable.tar.gz
	# Package changes, we cannot used hash to verify
	# URL_HASH SHA1=c79ff91090e116e46e53d2989c701a30d808c025
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix <INSTALL_DIR>
    )

    ExternalProject_Get_Property(libsodium_tarball INSTALL_DIR)

    add_library(libsodium STATIC IMPORTED GLOBAL)
    add_dependencies(libsodium libsodium_tarball)
    set_target_properties(libsodium PROPERTIES IMPORTED_LOCATION ${INSTALL_DIR}/lib/libsodium.a)
else()
    add_library(libsodium INTERFACE)
    target_link_libraries(
        libsodium INTERFACE -lsodium
    )
endif()
