cmake_minimum_required(VERSION 3.16)
project(stable-absl-hash)

set(CMAKE_CXX_STANDARD 20)

add_library(wy-hash STATIC wyhash.cc)
target_include_directories(wy-hash PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
