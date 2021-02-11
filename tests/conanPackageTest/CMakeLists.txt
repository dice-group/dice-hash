cmake_minimum_required(VERSION 3.0)
project(include)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(dice-hash)
add_executable(main main.cpp)
target_link_libraries(main Dice::dice-hash)
