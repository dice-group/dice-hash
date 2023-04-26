set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.3.2
        GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(Catch2)

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
include(Catch)

add_executable(tests_dice_hash TestDiceHash.cpp)
target_link_libraries(tests_dice_hash PRIVATE
        Catch2::Catch2 Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_dice_hash PROPERTIES CXX_STANDARD 20)
catch_discover_tests(tests_dice_hash)

add_executable(tests_Blake2xb TestBlake2xb.cpp)
target_link_libraries(tests_Blake2xb PRIVATE
        Catch2::Catch2 Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_Blake2xb PROPERTIES CXX_STANDARD 20)
catch_discover_tests(tests_Blake2xb)

add_executable(tests_LtHash_simple TestLtHash_simple.cpp)
target_link_libraries(tests_LtHash_simple PRIVATE
        Catch2::Catch2 Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_LtHash_simple PROPERTIES CXX_STANDARD 20)
catch_discover_tests(tests_LtHash_simple)

add_executable(tests_LtHash_SSE2 TestLtHash_SSE2.cpp)
target_link_libraries(tests_LtHash_SSE2 PRIVATE
        Catch2::Catch2 Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_LtHash_SSE2 PROPERTIES CXX_STANDARD 20)
catch_discover_tests(tests_LtHash_SSE2)

add_executable(tests_LtHash_AVX2 TestLtHash_AVX2.cpp)
target_link_libraries(tests_LtHash_AVX2 PRIVATE
        Catch2::Catch2 Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_LtHash_AVX2 PROPERTIES CXX_STANDARD 20)
catch_discover_tests(tests_LtHash_AVX2)

add_executable(benchmark_Blake2xb BenchmarkBlake2xb.cpp)
target_link_libraries(benchmark_Blake2xb PRIVATE
        Catch2::Catch2 Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(benchmark_Blake2xb PROPERTIES CXX_STANDARD 20)
catch_discover_tests(benchmark_Blake2xb)

add_executable(benchmark_LtHash_simple BenchmarkLtHash_simple.cpp)
target_link_libraries(benchmark_LtHash_simple PRIVATE
        Catch2::Catch2 Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(benchmark_LtHash_simple PROPERTIES CXX_STANDARD 20)
catch_discover_tests(benchmark_LtHash_simple)

add_executable(benchmark_LtHash_SSE2 BenchmarkLtHash_SSE2.cpp)
target_link_libraries(benchmark_LtHash_SSE2 PRIVATE
        Catch2::Catch2 Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(benchmark_LtHash_SSE2 PROPERTIES CXX_STANDARD 20)
catch_discover_tests(benchmark_LtHash_SSE2)

add_executable(benchmark_LtHash_AVX2 BenchmarkLtHash_AVX2.cpp)
target_link_libraries(benchmark_LtHash_AVX2 PRIVATE
        Catch2::Catch2 Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(benchmark_LtHash_AVX2 PROPERTIES CXX_STANDARD 20)
catch_discover_tests(benchmark_LtHash_AVX2)
