cmake_minimum_required(VERSION 3.13)
project(dice-hash VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

# installation directories
set(dice-hash_INSTALL_INCLUDE_DIR "include" CACHE STRING "The installation include directory")
set(dice-hash_INSTALL_CMAKE_DIR "share/dice-hash/cmake" CACHE STRING "The installation cmake directory")

add_library(dice-hash INTERFACE)
add_library(dice-hash::dice-hash ALIAS dice-hash)


target_include_directories(dice-hash INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${dice-hash_INSTALL_CMAKE_DIR}>
        )

option(DICE_HASH_BUILD_TESTS "Build test programs" OFF)
if (DICE_HASH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

# Make package findable
configure_file(cmake/dummy-config.cmake.in dice-hash-config.cmake @ONLY)

# Enable version checks in find_package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(dice-hash-config-version.cmake COMPATIBILITY SameMajorVersion)

# install and export target
install(TARGETS dice-hash EXPORT dice-hash-targets)

install(EXPORT dice-hash-targets
        FILE dice-hash-config.cmake
        NAMESPACE Dice::
        DESTINATION ${dice-hash_INSTALL_CMAKE_DIR}
        )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dice-hash-config-version.cmake DESTINATION ${dice-hash_INSTALL_CMAKE_DIR})
install(DIRECTORY include/ DESTINATION ${dice-hash_INSTALL_INCLUDE_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dice-hash-config-version.cmake DESTINATION ${dice-hash_INSTALL_CMAKE_DIR})
