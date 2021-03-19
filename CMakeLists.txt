cmake_minimum_required(VERSION 3.13)
project(dice-hash VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

# set gcc-10 and clang-10 as minimum versions see
# https://stackoverflow.com/questions/14933172/how-can-i-add-a-minimum-compiler-version-requisite#14934542
set(MIN_COMPILER_VERSION_GCC "10.0.0")
set(MIN_COMPILER_VERSION_CLANG "10.0.0")
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MIN_COMPILER_VERSION_GCC)
        MESSAGE(FATAL_ERROR "Insufficient gcc version. \
        Your version is ${CMAKE_CXX_COMPILER_VERSION} and ${MIN_COMPILER_VERSION_GCC} is needed.")
    endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0.0")
        MESSAGE(FATAL_ERROR "Insufficient clang version. \
        Your version is ${CMAKE_CXX_COMPILER_VERSION} and ${MIN_COMPILER_VERSION_CLANG} is needed.")
    endif()
else()
    MESSAGE(WARNING "Could not verify that your compiler (${CMAKE_CXX_COMPILER}) supports all needed features.")
endif()

# make march available from outside
if(DEFINED ${DICE_HASH_MARCH})
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=${DICE_HASH_MARCH}")
endif()
# general optimizations
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fomit-frame-pointer -momit-leaf-frame-pointer")
else ()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fomit-frame-pointer")
endif ()

# include subdirectories
add_subdirectory(src)

# installation directories
set(dice-hash_INSTALL_INCLUDE_DIR "include" CACHE STRING "The installation include directory")
set(dice-hash_INSTALL_SRC_DIR "lib" CACHE STRING "The installation src directory")
set(dice-hash_INSTALL_CMAKE_DIR "share/dice-hash/cmake" CACHE STRING "The installation cmake directory")

# generate (interface) library
add_library(dice-hash INTERFACE)
add_library(dice-hash::dice-hash ALIAS dice-hash)
target_link_libraries(dice-hash INTERFACE compiled_hashes)
target_include_directories(dice-hash INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${dice-hash_INSTALL_CMAKE_DIR}>
        )

# non essential code build
option(DICE_HASH_BUILD_TESTS "Build test programs" OFF)
if (DICE_HASH_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
option(DICE_HASH_BUILD_EXAMPLES "Build example programs" OFF)
if (DICE_HASH_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif ()

# Make package findable
configure_file(cmake/dummy-config.cmake.in dice-hash-config.cmake @ONLY)

# Enable version checks in find_package
include(CMakePackageConfigHelpers)
write_basic_package_version_file(dice-hash-config-version.cmake COMPATIBILITY SameMajorVersion)

# install and export target
install(TARGETS
        dice-hash
        compiled_hashes
        EXPORT dice-hash-targets)
install(TARGETS compiled_hashes DESTINATION ${dice_hash_INSTALL_SRC_DIR})

install(EXPORT dice-hash-targets
        FILE dice-hash-config.cmake
        NAMESPACE Dice::
        DESTINATION ${dice-hash_INSTALL_CMAKE_DIR}
        )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dice-hash-config-version.cmake DESTINATION ${dice-hash_INSTALL_CMAKE_DIR})
install(DIRECTORY include/ DESTINATION ${dice-hash_INSTALL_INCLUDE_DIR})

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dice-hash-config-version.cmake DESTINATION ${dice-hash_INSTALL_CMAKE_DIR})