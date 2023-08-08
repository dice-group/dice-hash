cmake_minimum_required(VERSION 3.16)

project(
        dice-hash
        VERSION 0.4.3
        DESCRIPTION "dice-hash provides a framework to generate stable hashes. It provides state-of-the-art hash functions, supports STL containers out of the box and helps you to defines stable hashes for your own structs and classes."
        HOMEPAGE_URL "https://dice-group.github.io/dice-hash/")

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

OPTION(WITH_SODIUM "Enabled usage of libsodium for Blake2b, Blake2Xb and LtHash" ON)

OPTION(USE_CONAN "If available, use conan to retrieve dependencies." ON)
if (PROJECT_IS_TOP_LEVEL AND USE_CONAN AND (BUILD_TESTING OR WITH_SODIUM))
    include(cmake/conan_cmake.cmake)
    if (BUILD_TESTING)
        set(CONAN_DICE_HASH_WITH_TEST_DEPS "True")
    else()
        set(CONAN_DICE_HASH_WITH_TEST_DEPS "False")
    endif()

    if (WITH_SODIUM)
        set(CONAN_DICE_HASH_WITH_SODIUM "True")
    else()
        set(CONAN_DICE_HASH_WITH_SODIUM "False")
    endif()

    set(CONAN_OPTIONS "with_test_deps=${CONAN_DICE_HASH_WITH_TEST_DEPS};with_sodium=${CONAN_DICE_HASH_WITH_SODIUM};boost:header_only=True")
    install_packages_via_conan("${CMAKE_SOURCE_DIR}/conanfile.py" "${CONAN_OPTIONS}")
endif ()

if (WITH_SODIUM)
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
endif ()

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
        ${PROJECT_NAME}
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

if (WITH_SODIUM)
    target_link_libraries(${PROJECT_NAME} INTERFACE ${sodium_dep})
endif ()

include(cmake/install_interface_library.cmake)
install_interface_library(${PROJECT_NAME} "include")

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
    include(CTest)
    enable_testing()
    add_subdirectory(tests)
endif()

option(BUILD_EXAMPLES "Build example programs" OFF)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
