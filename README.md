# dice-hash: A Hashing framework

dice-hash provides a framework to generate stable hashes. It provides state-of-the-art hash functions, supports STL containers out of the box and helps you to defines stable hashes for your own structs and classes. 

**🔋 batteries included:** dice-hash defines _policies_ to support different hash algorithms. It comes with predefined policies for three state-of-the-art hash functions:
- [XXH3](https://github.com/Cyan4973/xxHash)
- [wyhash](https://github.com/wangyi-fudan/wyhash)
- "martinus", the internal hash function from [robin-hood-hashing](https://github.com/martinus/robin-hood-hashing)

These three, additional, general purpose hash functions are also (optionally) provided
- [Blake2b](https://www.blake2.net)
- [Blake2Xb](https://www.blake2.net/blake2x.pdf)
- [LtHash](https://engineering.fb.com/2019/03/01/security/homomorphic-hashing)

**📦 STL out of the box:** dice-hash supports many common STL types already: 
arithmetic types like `bool`, `int`, `double`, ... etc.; collections like `std::unordered_map/set`, `std::map/set`, `std::vector`, `std::tuple`, `std::pair`, `std::optional`, `std::variant`, `std::array` and; all combinations of them. 

**🔩 extensible:** dice-hash supports you with helper functions to define hashes for your own classes. Checkout [usage](#usage).

## Requirements
- A C++20 compatible compiler. Code was only tested on x86_64.
- If you want to use [Blake2b](https://www.blake2.net), [Blake2Xb](https://www.blake2.net/blake2x.pdf) or [LtHash](https://engineering.fb.com/2019/03/01/security/homomorphic-hashing): [libsodium](https://doc.libsodium.org/) (either using conan or a local system installation) (for more details scroll down to "Usage for general data hashing")

## Include it into your projects 

### CMake

### conan
To use it with [conan](https://conan.io/) you need to add the repository:
```shell
conan remote add dice-group https://conan.dice-research.org/artifactory/api/conan/tentris
```

To use it add `dice-hash/0.4.9` to the `[requires]` section of your conan file.

You can now add it to your target with:
```cmake
target_link_libraries(your_target
        dice-hash::dice-hash
        )
```

## build and run tests

```shell
#get it 
git clone https://github.com/dice-group/dice-hash.git
cd dice-hash
#build it
wget https://github.com/conan-io/cmake-conan/raw/develop2/conan_provider.cmake -O conan_provider.cmake
mkdir build
cd build
cmake -DBUILD_TESTING -DCMAKE_BUILD_TYPE=Release ..  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake
make -j tests_dice_hash
./test/tests_dice_hash
```
Note: This example uses conan as dependency provider, other providers are possible.
See https://cmake.org/cmake/help/latest/guide/using-dependencies/index.html#dependency-providers

## Usage for C++ container hashing
You need to include a single header:
```c++
#include <dice/hash.hpp>
```

The hash is already defined for a lot of common types. In that case you can use the `DiceHash` just like `std::hash`.
This means these hashes return `size_t`, if you need larger hashes skip to the section below.
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

## Usage for general data hashing
**The hash functions mentioned in this section are enabled/disabled using the feature flag `WITH_SODIUM=ON/OFF`.**
**Enabling this flag (default behaviour) results in [libsodium](https://doc.libsodium.org/) being required as a dependency.**
**If using conan, [libsodium](https://doc.libsodium.org/) will be fetched using conan, otherwise dice-hash will look for a local system installation.**

The hashes mentioned here are not meant to be used in C++ containers as they do _not_ return `size_t`.
They are instead meant as general hashing functions for arbitrary data.

### [Blake2b](https://www.blake2.net/) - ["fast secure hashing"](https://www.blake2.net/) (with output sizes from 16 bytes up to 64 bytes)
["BLAKE2 is a cryptographic hash function faster than MD5, SHA-1, SHA-2, and SHA-3, yet is at least as secure as the latest standard SHA-3."](https://www.blake2.net/)

To use it you need to include
```c++
#include <dice/hash/blake2/Blake2b.hpp>
```
For a usage examples see: [examples/blake2b.cpp](examples/blake2b.cpp).

### [Blake2Xb](https://www.blake2.net/blake2x.pdf) - arbitrary length hashing based on [Blake2b](https://www.blake2.net/)
Blake2Xb is a hash function that produces hashes of arbitrary length.

To use it you need to include
```c++
#include <dice/hash/blake2/Blake2xb.hpp>
```
For a usage examples see: [examples/blake2xb.cpp](examples/blake2xb.cpp).

### [Blake3](https://github.com/BLAKE3-team/BLAKE3-specs/blob/master/blake3.pdf) - one function, fast everywhere
Blake3 is an evolution of Blake2.

To use it you need to include
```c++
#include <dice/hash/blake2/Blake3.hpp>
```
For a usage examples see: [examples/blake3.cpp](examples/blake3.cpp).

### [LtHash](https://engineering.fb.com/2019/03/01/security/homomorphic-hashing/) - homomorphic/multiset hashing
LtHash is a multiset/homomorphic hash function, meaning, instead of working on streams of data, it digests
individual "objects". This means you can add and remove "objects" to/from an `LtHash` (object by object)
as if it were a multiset and then read the hash that would result from hashing that multiset.

Small non-code example that shows the basic principle:
> LtHash({apple}) + LtHash({banana}) - LtHash({peach}) + LtHash({banana}) = LtHash({apple<sup>1</sup>, banana<sup>2</sup>, peach<sup>-1</sup>})

To use it you need to include
```c++
#include <dice/hash/lthash/LtHash.hpp>
```
For a usage example see [examples/ltHash.cpp](examples/ltHash.cpp).
