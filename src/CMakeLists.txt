add_library(compiled_hashes STATIC wyhash/wyhash.cc)
target_compile_options(compiled_hashes PUBLIC -O2)
target_include_directories(compiled_hashes PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wyhash>
        $<INSTALL_INTERFACE:${dice-hash_INSTALL_INCLUDE_DIR}>
        )
