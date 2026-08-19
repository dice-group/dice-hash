cmake_minimum_required(VERSION 3.24)

project(
        dice-hash
        VERSION 0.5.1
        DESCRIPTION "dice-hash provides a framework to generate stable hashes. It provides state-of-the-art hash functions, supports STL containers out of the box and helps you to defines stable hashes for your own structs and classes."
        HOMEPAGE_URL "https://dice-group.github.io/dice-hash/")
set(POBR_VERSION 2)  # Persisted Object Binary Representation Version

include(cmake/boilerplate_init.cmake)
boilerplate_init()

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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/include/dice/hash/version.hpp)

OPTION(WITH_SODIUM "Enable usage of the external library sodium for Blake2b, Blake2Xb and LtHash support" OFF)

if (PROJECT_IS_TOP_LEVEL)
    if (BUILD_TESTING)
        set(CONAN_INSTALL_ARGS "${CONAN_INSTALL_ARGS};-o=&:with_test_deps=True")
        # metall consumes boost headers only. Compiling boost is slow, and its b2
        # build fails with clang on aarch64.
        set(CONAN_INSTALL_ARGS "${CONAN_INSTALL_ARGS};-o=boost/*:header_only=True")
    endif ()
    if (WITH_SODIUM)
        set(CONAN_INSTALL_ARGS "${CONAN_INSTALL_ARGS};-o=&:with_sodium=True")
    endif ()
endif ()

if (WITH_SODIUM)
    find_package(libsodium REQUIRED)
    find_package(highway REQUIRED)
endif ()

# rapidhash consists of a single header and ships no CMakeLists.txt, so fetch it
# ourselves. An enclosing project may provide rapidhash itself, in which case its
# target wins and that project is also responsible for exporting it.
if (NOT TARGET rapidhash AND NOT TARGET rapidhash::rapidhash)
    add_subdirectory(include/dice/hash/internal/rapidhash)
endif ()

# link against the namespaced name regardless of who provides the target
if (NOT TARGET rapidhash::rapidhash)
    get_target_property(rapidhash_alias rapidhash ALIASED_TARGET)
    if (rapidhash_alias)
        add_library(rapidhash::rapidhash ALIAS ${rapidhash_alias})
    else ()
        add_library(rapidhash::rapidhash ALIAS rapidhash)
    endif ()
    unset(rapidhash_alias)
endif ()

find_package(xxHash REQUIRED)

# the official Blake3 implementation ships no usable CMakeLists.txt, so build it
# ourselves. An enclosing project may provide blake3 itself, in which case its target
# wins and that project is also responsible for exporting it, putting blake3.h on the
# include path and shipping the Blake3 licenses.
if (NOT TARGET blake3 AND NOT TARGET BLAKE3::blake3)
    add_subdirectory(include/dice/hash/blake/internal/blake3)
endif ()

# link against the namespaced name regardless of who provides the target. find_package(blake3)
# only defines BLAKE3::blake3, so linking the bare name would silently degrade to -lblake3.
if (NOT TARGET BLAKE3::blake3)
    get_target_property(blake3_alias blake3 ALIASED_TARGET)
    if (blake3_alias)
        add_library(BLAKE3::blake3 ALIAS ${blake3_alias})
    else ()
        add_library(BLAKE3::blake3 ALIAS blake3)
    endif ()
    unset(blake3_alias)
endif ()

# install rules for the licenses of the header-only vendored dependencies
add_subdirectory(include/dice/hash/internal/wyhash)
add_subdirectory(include/dice/hash/internal/robinhood)

if (WITH_SODIUM)
    add_library(${PROJECT_NAME}
            include/dice/hash/lthash/MathEngine_Hwy.cpp
    )

    target_include_directories(
            ${PROJECT_NAME}
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
    target_link_libraries(${PROJECT_NAME}
            PUBLIC
            libsodium::libsodium
            BLAKE3::blake3
            rapidhash::rapidhash
            xxHash::xxhash
            PRIVATE
            highway::highway
    )
else()
    add_library(${PROJECT_NAME} INTERFACE)

    target_include_directories(
            ${PROJECT_NAME}
            INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    )
    target_link_libraries(${PROJECT_NAME}
            INTERFACE
            BLAKE3::blake3
            rapidhash::rapidhash
            xxHash::xxhash
    )
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

include(cmake/install_library.cmake)
install_cpp_library(${PROJECT_NAME} "include")

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

option(BUILD_EXAMPLES "Build example programs" OFF)
if(PROJECT_IS_TOP_LEVEL AND BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
