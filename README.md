# dice-hash: A Hashing framework

dice-hash provides a framework to generate stable hashes. It provides state-of-the-art hash functions, supports STL containers out of the box and helps you to defines stable hashes for your own structs and classes. 

**🔋 batteries included:** dice-hash defines _policies_ to support different hash algorithms. It comes with predefined policies for three state-of-the-art hash functions:
- [XXH3](https://github.com/Cyan4973/xxHash)
- [wyhash](https://github.com/wangyi-fudan/wyhash)
- "martinus", the internal hash function from [robin-hood-hashing](https://github.com/martinus/robin-hood-hashing)

**📦 STL out of the box:** dice-hash supports many common STL types already: 
arithmetic types like `bool`, `int`, `double`, ... etc.; collections like `std::unordered_map/set`, `std::map/set`, `std::vector`, `std::tuple`, `std::pair`, `std::optional`, `std::variant`, `std::array` and; all combinations of them. 

**🔩 extensible:** dice-hash supports you with helper functions to define hashes for your own classes. Checkout [usage](#usage). 




## Requirements

A C++20 compatible compiler. Code was only tested on x86_64.

## Include it into your projects 

### CMake

add 
```cmake
FetchContent_Declare(
        dice-hash
        GIT_REPOSITORY https://github.com/dice-group/dice-hash.git
        GIT_TAG 0.4.0
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

To use it add `dice-hash/0.4.0@dice-group/stable` to the `[requires]` section of your conan file.

## build and run tests

```shell
#get it 
git clone https://github.com/dice-group/dice-hash.git
cd dice-hash
#build it
mkdir build
cd build
cmake -DBUILD_TESTING -DCMAKE_BUILD_TYPE=Release ..
make -j tests_dice_hash
./test/tests_dice_hash
```

## usage
You need to include a single header:
```c++
#include <dice/hash.hpp>
```

The hash is already defined for a lot of common types. In that case you can use the `DiceHash` just like `std::hash`.
```c++
dice::hash::DiceHash<int> hash;
hash(42);
```
[basicUsage](examples/basicUsage.cpp) is a run able example for this use-case.

If you need `DiceHash` to be able to work on your own types, you can specialize the `dice::hash::dice_hash_overload` template:
```c++
struct YourType{};
namespace dice::hash {
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
namespace dice::hash {
    template<> struct is_ordered_container<YourOwnOrderedContainer> : std::true_type {};
}
```
Now you can use `DiceHash` with your container.
__However__:
Your container __needs__ to have `begin`, `end` and `size` functions.
One simple example can be found [here](examples/customContainer.cpp).

If you want to use `DiceHash` in a different structure (like `std::unordered_map`), you will need to set `DiceHash` as the correct template parameter.
[This](examples/usageForUnorderedSet.cpp) is one example.
