#!/bin/bash

rm -r build 2> /dev/null
mkdir build && cd build
conan install ..
CC=gcc-10 CXX=g++-10 cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_paths.cmake
make