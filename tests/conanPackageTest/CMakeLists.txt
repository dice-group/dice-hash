cmake_minimum_required(VERSION 3.13)
project(conan-test-project)

include(${CMAKE_BINARY_DIR}/conan_paths.cmake)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(dice-hash REQUIRED)
add_executable(test_dice_hash_conan main.cpp)
target_link_libraries(test_dice_hash_conan Dice::dice-hash)
