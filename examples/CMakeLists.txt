set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(basicUsage basicUsage.cpp)
target_link_libraries(basicUsage PRIVATE
        dice-hash::dice-hash
        )

add_executable(policyUsage policyUsage.cpp)
target_link_libraries(policyUsage PRIVATE
        dice-hash::dice-hash
        )

add_executable(customType customType.cpp)
target_link_libraries(customType PRIVATE
        dice-hash::dice-hash
        )

add_executable(customPolicy customPolicy.cpp)
target_link_libraries(customPolicy PRIVATE
        dice-hash::dice-hash
        )

add_executable(customContainer customContainer.cpp)
target_link_libraries(customContainer PRIVATE
        dice-hash::dice-hash
        )

add_executable(usageForUnorderedSet usageForUnorderedSet.cpp)
target_link_libraries(usageForUnorderedSet PRIVATE
        dice-hash::dice-hash
        )

add_executable(combineHashes combineHashes.cpp)
target_link_libraries(combineHashes PRIVATE
        dice-hash::dice-hash
        )

if (WITH_SODIUM)
    add_executable(blake2b blake2b.cpp)
    target_link_libraries(blake2b PRIVATE
            dice-hash::dice-hash
            )

    add_executable(blake2xb blake2xb.cpp)
    target_link_libraries(blake2xb PRIVATE
            dice-hash::dice-hash
            )

    add_executable(ltHash ltHash.cpp)
    target_link_libraries(ltHash PRIVATE
            dice-hash::dice-hash
            )
endif ()

add_executable(blake3_example blake3.cpp)
target_link_libraries(blake3_example PRIVATE
        dice-hash::dice-hash
)
