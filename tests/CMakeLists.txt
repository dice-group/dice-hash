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

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
include(Catch)

add_executable(tests_dice_hash TestDiceHash.cpp)
target_link_libraries(tests_dice_hash PRIVATE
        Catch2::Catch2
        dice-hash::dice-hash
        )
catch_discover_tests(tests_dice_hash)

add_executable(tests_Blake2xb TestBlake2xb.cpp)
target_link_libraries(tests_Blake2xb PRIVATE
        Catch2::Catch2
        dice-hash::dice-hash
        )
catch_discover_tests(tests_Blake2xb)

add_executable(tests_LtHash_simple TestLtHash_simple.cpp)
target_link_libraries(tests_LtHash_simple PRIVATE
        Catch2::Catch2
        dice-hash::dice-hash
        )
catch_discover_tests(tests_LtHash_simple)

add_executable(tests_LtHash_SSE2 TestLtHash_SSE2.cpp)
target_link_libraries(tests_LtHash_SSE2 PRIVATE
        Catch2::Catch2
        dice-hash::dice-hash
        )
catch_discover_tests(tests_LtHash_SSE2)

add_executable(tests_LtHash_AVX2 TestLtHash_AVX2.cpp)
target_link_libraries(tests_LtHash_AVX2 PRIVATE
        Catch2::Catch2
        dice-hash::dice-hash
        )
catch_discover_tests(tests_LtHash_AVX2)
