cmake_minimum_required(VERSION 3.16)
project(PackageTest CXX)

find_package(dice-hash REQUIRED)

add_executable(example example.cpp)

target_link_libraries(example PRIVATE
        dice-hash::dice-hash
        )

set_target_properties(
        example PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)
