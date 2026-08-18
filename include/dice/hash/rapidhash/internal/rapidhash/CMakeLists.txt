cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

include(FetchContent)
FetchContent_Declare(
        rapidhash
        GIT_REPOSITORY https://github.com/Nicoshev/rapidhash.git
        GIT_TAG bc4b4baa48a15ff52ff4725e1ccdcda62815221c  # tag rapidhash_v3
)

FetchContent_MakeAvailable(rapidhash)

include(GNUInstallDirs)
add_library(rapidhash INTERFACE)
# SYSTEM keeps warnings from the upstream header out of warning-as-error builds
target_include_directories(rapidhash SYSTEM INTERFACE
        $<BUILD_INTERFACE:${rapidhash_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# DiceHashPolicies.hpp includes rapidhash.h, so the header has to travel with
# the install tree and rapidhash has to be part of dice-hash's export set
install(TARGETS rapidhash EXPORT ${PROJECT_NAME}-targets)
install(FILES ${rapidhash_SOURCE_DIR}/rapidhash.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${rapidhash_SOURCE_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses
        RENAME LICENSE-rapidhash)