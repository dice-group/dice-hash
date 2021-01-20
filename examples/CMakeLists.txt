add_executable(basicUsage basicUsage.cpp)
target_link_libraries(basicUsage PRIVATE
        dice-hash::dice-hash
        )

add_executable(customType customType.cpp)
target_link_libraries(customType PRIVATE
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
