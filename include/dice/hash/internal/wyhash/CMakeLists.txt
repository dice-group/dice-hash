include(GNUInstallDirs)

# wyhash is vendored as a single header, so install_cpp_library's *.h/*.hpp filter
# would drop the notice next to it. Renamed on install because the FetchContent'ed
# dependencies drop their licenses into the same directory.
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses
        RENAME LICENSE-wyhash)