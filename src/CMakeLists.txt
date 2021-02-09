add_library(base-hash-wyhash STATIC wyhash/wyhash.cc)
target_include_directories(base-hash-wyhash PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wyhash>
        $<INSTALL_INTERFACE:${dice-hash_INSTALL_CMAKE_DIR}>
        )
add_library(base-hash-xxhash INTERFACE)
target_include_directories(base-hash-xxhash INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/xxhash>
        $<INSTALL_INTERFACE:${dice-hash_INSTALL_CMAKE_DIR}>
        )
add_library(base-hash-martinus INTERFACE)
target_include_directories(base-hash-martinus INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/martinus>
    $<INSTALL_INTERFACE:${dice-hash_INSTALL_CMAKE_DIR}>
    )

add_library(base-hashes INTERFACE)
target_link_libraries(base-hashes INTERFACE
        base-hash-wyhash
        base-hash-xxhash
        base-hash-martinus
        )
