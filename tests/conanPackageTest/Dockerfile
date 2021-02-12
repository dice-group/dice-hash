FROM ubuntu:focal AS builder
ARG DEBIAN_FRONTEND=noninteractive

#install needed packages
RUN apt-get -qq update && \
    apt-get -qq install -y git make cmake clang-10 libstdc++-10-dev
ENV CC="clang-10" CXX="clang++-10"

# install conan
RUN apt-get -qq install -y python3-pip python3-setuptools python3-wheel
RUN pip3 install conan && \
    conan user && \
    conan profile new --detect default

# create local conan package
RUN mkdir dice-hash
WORKDIR /dice-hash
COPY cmake cmake
COPY src src
COPY include include
COPY tests tests
COPY CMakeLists.txt CMakeLists.txt
COPY LICENSE LICENSE
COPY README.md README.md
COPY conanfile.py conanfile.py
RUN conan create . "dice-hash/100@dice-group/stable" --build missing

# create local project that uses the conan package
WORKDIR /test_conan
COPY tests/conanPackageTest/conanfile.txt conanfile.txt
COPY tests/conanPackageTest/main.cpp main.cpp
COPY tests/conanPackageTest/CMakeLists.txt CMakeLists.txt
WORKDIR /test_conan/build
RUN conan install .. --build=missing
RUN cmake -DCMAKE_BUILD_TYPE=Release ..
RUN make test_dice_hash_conan
RUN ./test_dice_hash_conan