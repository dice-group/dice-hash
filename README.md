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
        GIT_TAG 0.2.0
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
conan remote add dice-group https://conan.dice-research.org/artifactory/api/conan/tentris
```

To use it add `dice-hash/0.2.0@dice-group/stable` to the `[requires]` section of your conan file.

To activate march=native, you can additionally add `dice-hash:march=native` to the `[options]` section. 

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

`-march=native` is activated by the additional CMake option: `-DDICE_HASH_MARCH=native`

## usage
The hash is already defined for a lot of common types. In that case you can use the `DiceHash` just like `std::hash`.
```c++
Dice::hash::DiceHash<int> hash;
hash(42);
```
[basicUsage](examples/basicUsage.cpp) is a run able example for this use-case.

If you need `DiceHash` to be able to work on your own types, you can specialize the `Dice::hash::dice_hash_overload` template:
```c++
struct YourType{};
namespace Dice::hash {
    template <typename Policy>
    struct dice_hash_overload<Policy, YourType> {
        static std::size_t dice_hash(YourType const& x) noexcept {
            return 42;
        }
    };
}
```
[Here](examples/customType.cpp) is an compilable example. 

If you want to combine the hash of two or more objects you can use the
`hash_combine` or `hash_invertible_combine` function.
These are part of the Policy, however they can be called via the DiceHash object.
An example can be seen [here](examples/combineHashes.cpp).

If your own type is a container type, there is an easier and faster way to define the hash for you.
There are the two typetraits `is_ordered_container` and `is_unordered_container`.
You just need to set these typetraits for your own type, and the hash will automatically loop over the entries and hash them.
```c++
struct YourOwnOrderedContainer{...};
namespace Dice::hash {
    template<> struct is_ordered_container<YourOwnOrderedContainer> : std::true_type {};
}
```
Now you can use `DiceHash` with your container.
__However__:
Your container __needs__ to have `begin`, `end` and `size` functions.
One simple example can be found [here](examples/customContainer.cpp).

If you want to use `DiceHash` in a different structure (like `std::unordered_map`), you will need to set `DiceHash` as the correct template parameter.
[This](examples/usageForUnorderedSet.cpp) is one example.
