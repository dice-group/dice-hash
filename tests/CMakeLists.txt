set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v2.13.9
        GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(Catch2)

add_executable(tests_dice_hash TestDiceHash.cpp)
target_link_libraries(tests_dice_hash PRIVATE
        Catch2::Catch2
        dice-hash::dice-hash
        )

add_executable(tests_lthash TestLtHash.cpp)
target_link_libraries(tests_lthash PRIVATE
        Catch2::Catch2
        dice-hash::dice-hash
        )

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
include(Catch)
catch_discover_tests(tests_dice_hash)
