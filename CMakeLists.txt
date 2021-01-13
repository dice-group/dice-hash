cmake_minimum_required(VERSION 3.13)
project(dice_hash VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

# installation directories
set(dice_hash_INSTALL_INCLUDE_DIR "include" CACHE STRING "The installation include directory")
set(dice_hash_INSTALL_CMAKE_DIR "share/dice_hash/cmake" CACHE STRING "The installation cmake directory")

add_library(dice_hash INTERFACE)
add_library(dice_hash::dice_hash ALIAS dice_hash)


target_include_directories(dice_hash INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${dice_hash_INSTALL_CMAKE_DIR}>
        )

option(dice_hash_BUILD_TESTS "Build test programs" OFF)
if (dice_hash_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

# Make package findable
configure_file(cmake/dummy-config.cmake.in dice_hash-config.cmake @ONLY)

# Enable version checks in find_package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(dice_hash-config-version.cmake COMPATIBILITY SameMajorVersion)

# install and export target
install(TARGETS dice_hash EXPORT dice_hash-targets)

install(EXPORT dice_hash-targets
        FILE dice_hash-config.cmake
        NAMESPACE Dice::
        DESTINATION ${dice_hash_INSTALL_CMAKE_DIR}
        )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dice_hash-config-version.cmake DESTINATION ${dice_hash_INSTALL_CMAKE_DIR})
install(DIRECTORY include/ DESTINATION ${dice_hash_INSTALL_INCLUDE_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dice_hash-config-version.cmake DESTINATION ${dice_hash_INSTALL_CMAKE_DIR})
