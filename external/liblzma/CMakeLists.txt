
if (BUILD_STATIC_BINARY)
    ExternalProject_Add(
        liblzma_tarball
        URL https://github.com/tukaani-project/xz/archive/refs/tags/v5.6.3.tar.gz
        URL_HASH SHA1=4cfd7df8f4425e4ae592d4f13aa5a580616a8b70
        DOWNLOAD_EXTRACT_TIMESTAMP ON
        INSTALL_COMMAND cmake -E echo "Skipping install step."
    )

  add_library(liblzma STATIC IMPORTED GLOBAL)
  set_target_properties(liblzma PROPERTIES IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/liblzma_tarball-prefix/src/liblzma_tarball-build/liblzma.a)
else()
  add_library(liblzma INTERFACE)
  target_link_libraries(
    liblzma INTERFACE -llzma
  )
endif()