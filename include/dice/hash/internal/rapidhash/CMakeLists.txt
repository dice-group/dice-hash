cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

include(FetchContent)
# the handle is deliberately not named rapidhash: FetchContent declarations are keyed by
# name and the first one wins, so an enclosing project fetching its own rapidhash would
# silently substitute its revision for the one pinned below
FetchContent_Declare(
        dice_rapidhash
        GIT_REPOSITORY https://github.com/Nicoshev/rapidhash.git
        GIT_TAG bc4b4baa48a15ff52ff4725e1ccdcda62815221c  # tag rapidhash_v3
)

FetchContent_MakeAvailable(dice_rapidhash)

include(GNUInstallDirs)
add_library(rapidhash INTERFACE)
# SYSTEM keeps warnings from the upstream header out of warning-as-error builds
target_include_directories(rapidhash SYSTEM INTERFACE
        $<BUILD_INTERFACE:${dice_rapidhash_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# DiceHashPolicies.hpp includes rapidhash.h, so the header has to travel with
# the install tree and rapidhash has to be part of dice-hash's export set
install(TARGETS rapidhash EXPORT ${PROJECT_NAME}-targets)
install(FILES ${dice_rapidhash_SOURCE_DIR}/rapidhash.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${dice_rapidhash_SOURCE_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses
        RENAME LICENSE-rapidhash)
