set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Catch2 REQUIRED)

include(CTest)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/contrib)
include(Catch)

add_executable(tests_dice_hash TestDiceHash.cpp)
target_link_libraries(tests_dice_hash PRIVATE
        Catch2::Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_dice_hash PROPERTIES CXX_STANDARD 20)
catch_discover_tests(tests_dice_hash)

# `TestInt128.cpp` is built once with `-std=c++20` and once with `-std=gnu++20`. The two builds are
# separate executables, so that each executable holds the hash functions of one mode only.
add_executable(tests_int128_strict TestInt128.cpp)
target_link_libraries(tests_int128_strict PRIVATE
        Catch2::Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_int128_strict PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS OFF)
target_compile_definitions(tests_int128_strict PRIVATE DICE_HASH_TEST_STRICT_MODE=1)
catch_discover_tests(tests_int128_strict TEST_SUFFIX " (c++20)")

add_executable(tests_int128_gnu TestInt128.cpp)
target_link_libraries(tests_int128_gnu PRIVATE
        Catch2::Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_int128_gnu PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS ON)
target_compile_definitions(tests_int128_gnu PRIVATE DICE_HASH_TEST_STRICT_MODE=0)
catch_discover_tests(tests_int128_gnu TEST_SUFFIX " (gnu++20)")

# compile-time checks of DiceHash::is_avalanching
add_executable(tests_dice_hash_is_avalanching TestDiceHashIsAvalanching.cpp)
target_link_libraries(tests_dice_hash_is_avalanching PRIVATE
        Catch2::Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_dice_hash_is_avalanching PROPERTIES CXX_STANDARD 20)
catch_discover_tests(tests_dice_hash_is_avalanching)

# statistical checks of DiceHash::is_avalanching. A separate executable, so that it builds
# and runs even if the compile-time checks fail.
add_executable(tests_dice_hash_avalanche TestDiceHashAvalanche.cpp)
target_link_libraries(tests_dice_hash_avalanche PRIVATE
        Catch2::Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_dice_hash_avalanche PROPERTIES CXX_STANDARD 20)
catch_discover_tests(tests_dice_hash_avalanche)

# The statistical checks are also built with `-std=gnu++20`. In this mode libstdc++ counts
# `__int128` as integral, and `wyhash` hashes it by its bytes as in `-std=c++20`.
add_executable(tests_dice_hash_avalanche_gnu TestDiceHashAvalanche.cpp)
target_link_libraries(tests_dice_hash_avalanche_gnu PRIVATE
        Catch2::Catch2WithMain
        dice-hash::dice-hash
        )
set_target_properties(tests_dice_hash_avalanche_gnu PROPERTIES CXX_STANDARD 20 CXX_EXTENSIONS ON)
target_compile_definitions(tests_dice_hash_avalanche_gnu PRIVATE DICE_HASH_TEST_GNU_MODE=1)
catch_discover_tests(tests_dice_hash_avalanche_gnu TEST_SUFFIX " (gnu++20)")

if (WITH_SODIUM)
    add_executable(tests_Blake2b TestBlake2b.cpp)
    target_link_libraries(tests_Blake2b PRIVATE
            Catch2::Catch2WithMain
            dice-hash::dice-hash
            )
    set_target_properties(tests_Blake2b PROPERTIES CXX_STANDARD 20)
    catch_discover_tests(tests_Blake2b)

    add_executable(tests_Blake2Xb TestBlake2Xb.cpp)
    target_link_libraries(tests_Blake2Xb PRIVATE
            Catch2::Catch2WithMain
            dice-hash::dice-hash
            )
    set_target_properties(tests_Blake2Xb PROPERTIES CXX_STANDARD 20)
    catch_discover_tests(tests_Blake2Xb)

    add_executable(tests_LtHash_simple TestLtHash_simple.cpp)
    target_link_libraries(tests_LtHash_simple PRIVATE
            Catch2::Catch2WithMain
            dice-hash::dice-hash
            )
    set_target_properties(tests_LtHash_simple PROPERTIES CXX_STANDARD 20)
    catch_discover_tests(tests_LtHash_simple)

    add_executable(tests_LtHash_Hwy TestLtHash_Hwy.cpp)
    target_link_libraries(tests_LtHash_Hwy PRIVATE
            Catch2::Catch2WithMain
            dice-hash::dice-hash
            highway::highway
            )
    set_target_properties(tests_LtHash_Hwy PROPERTIES CXX_STANDARD 20)
    catch_discover_tests(tests_LtHash_Hwy)

    add_executable(benchmark_Blake2xb BenchmarkBlake2xb.cpp)
    target_link_libraries(benchmark_Blake2xb PRIVATE
            Catch2::Catch2WithMain
            dice-hash::dice-hash
            )
    set_target_properties(benchmark_Blake2xb PROPERTIES CXX_STANDARD 20)
    catch_discover_tests(benchmark_Blake2xb)

    add_executable(benchmark_LtHash_simple BenchmarkLtHash_simple.cpp)
    target_link_libraries(benchmark_LtHash_simple PRIVATE
            Catch2::Catch2WithMain
            dice-hash::dice-hash
            )
    set_target_properties(benchmark_LtHash_simple PROPERTIES CXX_STANDARD 20)
    catch_discover_tests(benchmark_LtHash_simple)

    add_executable(benchmark_LtHash_Hwy BenchmarkLtHash_Hwy.cpp)
    target_link_libraries(benchmark_LtHash_Hwy PRIVATE
            Catch2::Catch2WithMain
            dice-hash::dice-hash
            )
    set_target_properties(benchmark_LtHash_Hwy PROPERTIES CXX_STANDARD 20)
    catch_discover_tests(benchmark_LtHash_Hwy)

    find_package(Metall REQUIRED)

    add_executable(TestLtHash_metall_phase1 TestLtHash_metall_phase1.cpp)
    target_link_libraries(TestLtHash_metall_phase1 PRIVATE
            dice-hash::dice-hash
            Metall::Metall
            )
    set_target_properties(TestLtHash_metall_phase1 PROPERTIES CXX_STANDARD 20)

    add_executable(TestLtHash_metall_phase2 TestLtHash_metall_phase2.cpp)
    target_link_libraries(TestLtHash_metall_phase2 PRIVATE
            dice-hash::dice-hash
            Metall::Metall
            )
    set_target_properties(TestLtHash_metall_phase2 PROPERTIES CXX_STANDARD 20)

    add_executable(tests_LtHash_metall TestLtHash_metall.cpp)
    add_dependencies(tests_LtHash_metall TestLtHash_metall_phase1 TestLtHash_metall_phase2)
    target_link_libraries(tests_LtHash_metall PRIVATE
            Catch2::Catch2WithMain
            dice-hash::dice-hash
            Metall::Metall
            )
    set_target_properties(tests_LtHash_metall PROPERTIES CXX_STANDARD 20)
    catch_discover_tests(tests_LtHash_metall)

    # gcc reports -Wmaybe-uninitialized inside the metall and boost headers when
    # they are inlined at -O2. The reports point into the dependencies, not into
    # this code.
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        foreach (metall_target TestLtHash_metall_phase1 TestLtHash_metall_phase2 tests_LtHash_metall)
            target_compile_options(${metall_target} PRIVATE -Wno-maybe-uninitialized)
        endforeach ()
    endif ()
endif ()
