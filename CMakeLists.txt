cmake_minimum_required(VERSION 3.24)

project(
        dice-hash
        VERSION 0.4.8
        DESCRIPTION "dice-hash provides a framework to generate stable hashes. It provides state-of-the-art hash functions, supports STL containers out of the box and helps you to defines stable hashes for your own structs and classes."
        HOMEPAGE_URL "https://dice-group.github.io/dice-hash/")
set(POBR_VERSION 1)  # Persisted Object Binary Representation Version

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

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/src/dice/hash/version.hpp)

OPTION(WITH_BLAKE "Enable usage of the external library sodium for Blake2b, Blake2Xb and LtHash support" OFF)

if (PROJECT_IS_TOP_LEVEL)
    if (BUILD_TESTING)
        set(CONAN_INSTALL_ARGS "${CONAN_INSTALL_ARGS};-o=&:with_test_deps=True")
    endif ()
    if (WITH_BLAKE)
        set(CONAN_INSTALL_ARGS "${CONAN_INSTALL_ARGS};-o=&:with_blake=True")
    endif ()
endif ()

if (WITH_BLAKE)
    find_package(sodium QUIET) # try canonical name first
    set(sodium_dep "sodium")
    if (NOT sodium_FOUND)
        find_package(libsodium QUIET) #  next try the name used by conan
        set(sodium_dep "libsodium::libsodium")
        if (NOT libsodium_FOUND)
            include(cmake/FindSodium.cmake) # finally try the FindSodium.cmake provided with this repo
            set(sodium_dep "sodium")
            if (NOT Sodium_FOUND)
                message(FATAL_ERROR "Sodium was not found.")
            endif ()
        endif ()
    endif ()

    find_package(highway REQUIRED)
endif ()

set(blake3_rev 95e42b84fc4709974c7b23c7ae885989ab36c31e)
set(blake3_files
        blake3.c
        blake3.h
        blake3-config.cmake.in
        blake3_avx2.c
        blake3_avx2_x86-64_unix.S
        blake3_avx2_x86-64_windows_gnu.S
        blake3_avx2_x86-64_windows_msvc.asm
        blake3_avx512.c
        blake3_avx512_x86-64_unix.S
        blake3_avx512_x86-64_windows_gnu.S
        blake3_avx512_x86-64_windows_msvc.asm
        blake3_dispatch.c
        blake3_impl.h
        blake3_neon.c
        blake3_portable.c
        blake3_sse2.c
        blake3_sse2_x86-64_unix.S
        blake3_sse2_x86-64_windows_gnu.S
        blake3_sse2_x86-64_windows_msvc.asm
        blake3_sse41.c
        blake3_sse41_x86-64_unix.S
        blake3_sse41_x86-64_windows_gnu.S
        blake3_sse41_x86-64_windows_msvc.asm
        libblake3.pc.in
        LICENSE
)

foreach(file ${blake3_files})
    file(DOWNLOAD "https://raw.githubusercontent.com/BLAKE3-team/BLAKE3/${blake3_rev}/${file}"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/dice/hash/blake/internal/blake3/${file}"
            TLS_VERIFY ON)
endforeach()
add_subdirectory(src/dice/hash/blake/internal/blake3)

if (WITH_BLAKE)
    add_library(${PROJECT_NAME}
            src/dice/hash/lthash/MathEngine_Hwy.cpp
    )
else()
    add_library(${PROJECT_NAME} INTERFACE)
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
        INTERFACE blake3
)

if (WITH_BLAKE)
    target_include_directories(
            ${PROJECT_NAME}
            PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    )
    target_link_libraries(${PROJECT_NAME}
            INTERFACE ${sodium_dep}
            PRIVATE highway::highway
    )
else()
    target_include_directories(
            ${PROJECT_NAME}
            INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    )
endif()

include(cmake/install_library.cmake)
install_cpp_library(${PROJECT_NAME} "src")

if(PROJECT_IS_TOP_LEVEL AND BUILD_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

option(BUILD_EXAMPLES "Build example programs" OFF)
if(PROJECT_IS_TOP_LEVEL AND BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
