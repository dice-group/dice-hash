cmake_minimum_required(VERSION 3.16)

project(
        dice-hash
        VERSION 0.4.3
        DESCRIPTION "dice-hash provides a framework to generate stable hashes. It provides state-of-the-art hash functions, supports STL containers out of the box and helps you to defines stable hashes for your own structs and classes."
        HOMEPAGE_URL "https://dice-group.github.io/dice-hash/")

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

include(cmake/FindSodium.cmake)

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
        ${PROJECT_NAME}
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(
        ${PROJECT_NAME}
        INTERFACE sodium)

set_target_properties(
        ${PROJECT_NAME} PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND NOT DEFINED DICE_HASH_CACHE_LINE_SIZE)
    execute_process(COMMAND getconf LEVEL1_DCACHE_LINESIZE OUTPUT_VARIABLE DICE_HASH_GET_CLSZ_OUT OUTPUT_STRIP_TRAILING_WHITESPACE RESULT_VARIABLE DICE_HASH_GET_CLSZ_RES)
    if(DICE_HASH_GET_CLSZ_RES STREQUAL 0)
        set(DICE_HASH_CACHE_LINE_SIZE ${DICE_HASH_GET_CLSZ_OUT})
    endif()

    unset(DICE_HASH_GET_CLSZ_OUT)
    unset(DICE_HASH_GET_CLSZ_RES)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND DEFINED DICE_HASH_CACHE_LINE_SIZE)
    target_compile_definitions(${PROJECT_NAME} INTERFACE DICE_HASH_CACHE_LINE_SIZE=${DICE_HASH_CACHE_LINE_SIZE})
endif()

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
