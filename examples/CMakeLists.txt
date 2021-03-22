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
