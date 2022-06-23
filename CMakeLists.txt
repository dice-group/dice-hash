cmake_minimum_required(VERSION 3.13)
project(dice-hash VERSION 0.3.0)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

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


add_library(dice-hash INTERFACE)
add_library(dice-hash::dice-hash ALIAS dice-hash)
add_library(Dice::hash ALIAS dice-hash)

target_include_directories(dice-hash INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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

install(TARGETS dice-hash
        EXPORT dice-hash-targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        )

write_basic_package_version_file(
        dice-hashConfigVersion.cmake
        VERSION ${PACKAGE_VERSION}
        COMPATIBILITY SameMinorVersion
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/dice-hash-config-version.cmake DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# see https://cmake.org/cmake/help/latest/guide/importing-exporting/index.html
install(EXPORT dice-hash-targets
        FILE dice-hash-config.cmake
        NAMESPACE Dice::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/dice-hash)

