FROM ubuntu:focal AS builder
ARG DEBIAN_FRONTEND=noninteractive

#install needed packages
RUN apt-get -qq update && \
    apt-get -qq install -y git make cmake clang-10 libstdc++-10-dev

#create folder structure and copy needed code
RUN mkdir dice-hash
WORKDIR dice-hash
COPY cmake cmake
COPY include include
COPY tests tests
COPY CMakeLists.txt CMakeLists.txt

#compile
ENV CC="clang-10" CXX="clang++-10"
RUN mkdir build && cd build && cmake -DDICE_HASH_BUILD_TESTS=ON .. && make -j

#make accessable
ENTRYPOINT ["build/tests/tests_dice_hash"]