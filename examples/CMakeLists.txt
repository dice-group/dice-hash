set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(example_basicUsage basicUsage.cpp)
target_link_libraries(example_basicUsage PRIVATE
        dice-hash::dice-hash
        )

add_executable(example_policyUsage policyUsage.cpp)
target_link_libraries(example_policyUsage PRIVATE
        dice-hash::dice-hash
        )

add_executable(example_customType customType.cpp)
target_link_libraries(example_customType PRIVATE
        dice-hash::dice-hash
        )

add_executable(example_customPolicy customPolicy.cpp)
target_link_libraries(example_customPolicy PRIVATE
        dice-hash::dice-hash
        )

add_executable(example_customContainer customContainer.cpp)
target_link_libraries(example_customContainer PRIVATE
        dice-hash::dice-hash
        )

add_executable(example_usageForUnorderedSet usageForUnorderedSet.cpp)
target_link_libraries(example_usageForUnorderedSet PRIVATE
        dice-hash::dice-hash
        )

add_executable(example_combineHashes combineHashes.cpp)
target_link_libraries(example_combineHashes PRIVATE
        dice-hash::dice-hash
        )

if (WITH_SODIUM)
    add_executable(example_blake2b blake2b.cpp)
    target_link_libraries(example_blake2b PRIVATE
            dice-hash::dice-hash
            )

    add_executable(example_blake2xb blake2xb.cpp)
    target_link_libraries(example_blake2xb PRIVATE
            dice-hash::dice-hash
    )

    add_executable(example_ltHash ltHash.cpp)
    target_link_libraries(example_ltHash PRIVATE
            dice-hash::dice-hash
    )
endif ()

add_executable(example_blake3 blake3.cpp)
target_link_libraries(example_blake3 PRIVATE
        dice-hash::dice-hash
)
