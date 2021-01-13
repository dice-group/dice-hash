# dice-hash: High-Performance Hash for Hash-Based Container

dice-hash is based on the hash from [robin-hood-hashing hash map/set](https://github.com/martinus/robin-hood-hashing) by [Martin Ankerl](https://martin.ankerl.com/). It performs well as hash function for most hash-based containers like `std::unordered_map/set` or [`tsl::sparse_map/set`](https://github.com/Tessil/sparse-map). 
dice-hash is an extended version with support for all kinds of fixed, ordered and unordered data structures from STL. 

## Requirements

A C++20 compatible compiler. Code was only tested on x86_64.

## Include it into your projects 

### CMake

add 
```cmake
FetchContent_Declare(
        dice-hash
        GIT_REPOSITORY https://github.com/dice-group/dice-hash.git
        GIT_TAG 0.1.0
        GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(dice-hash)
```

to your CMakeLists.txt

You can now add it to your target with:
```cmake
target_link_libraries(your_target
        dice-hash::dice-hash
        )
```

### conan
To use it with [conan](https://conan.io/) you need to add the repository:
```shell
conan remote add dice-group https://api.bintray.com/conan/dice-group/tentris
```

To use it add `dice-hash/0.1.0@dice-group/stable` to the `[requires]` section of your conan file.

## build and run tests

```shell
#get it 
git clone https://github.com/dice-group/dice-hash.git
cd dice-hash
#build it
mkdir build
cd build
cmake -DDICE_HASH_BUILD_TESTS -DCMAKE_BUILD_TYPE=Release ..
make -j tests_dice_hash
./test/tests_dice_hash
```