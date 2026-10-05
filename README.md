# dice-hash: A Hashing framework

dice-hash provides a framework to generate stable hashes. It provides state-of-the-art hash functions, supports STL containers out of the box and helps you to defines stable hashes for your own structs and classes. 

**🔋 batteries included:** dice-hash defines _policies_ to support different hash algorithms. It comes with predefined policies for four state-of-the-art hash functions:
- [XXH3](https://github.com/Cyan4973/xxHash)
- [rapidhash](https://github.com/Nicoshev/rapidhash), in its protected mode
- [wyhash](https://github.com/wangyi-fudan/wyhash), in its condom 2 mode (`WYHASH_CONDOM 2`)
- "martinus", the internal hash function from [robin-hood-hashing](https://github.com/martinus/robin-hood-hashing)

dice-hash has its own copies of `wyhash.h` and `rapidhash.h`. They are in their own namespaces, and their macros are pushed and popped. So their configuration does not leak into other copies of these headers that a program uses.

These three, additional, general purpose hash functions are also (optionally) provided
- [Blake2b](https://www.blake2.net)
- [Blake2Xb](https://www.blake2.net/blake2x.pdf)
- [LtHash](https://engineering.fb.com/2019/03/01/security/homomorphic-hashing)

**📦 STL out of the box:** dice-hash supports many common STL types already: 
arithmetic types like `bool`, `int`, `double`, ... etc.; collections like `std::unordered_map/set`, `std::map/set`, `std::vector`, `std::tuple`, `std::pair`, `std::optional`, `std::variant`, `std::array` and; all combinations of them. 
`long double` is not supported: with gcc and clang on x86 and x86_64 it has padding bytes with undefined content, so equal values could get different hashes.

**🔩 extensible:** dice-hash supports you with helper functions to define hashes for your own classes. Checkout [usage](#usage).

## Requirements
- A C++20 compatible compiler. Tested on x86_64 and arm64, on Linux and macOS.
- If you want to use [Blake2b](https://www.blake2.net), [Blake2Xb](https://www.blake2.net/blake2x.pdf) or [LtHash](https://engineering.fb.com/2019/03/01/security/homomorphic-hashing): [libsodium](https://doc.libsodium.org/) (either using conan or a local system installation) (for more details scroll down to "Usage for general data hashing")

## Include it into your projects 

### CMake

### conan
To use it with [conan](https://conan.io/) you need to add the repository:
```shell
conan remote add dice-group https://conan.dice-research.org/artifactory/api/conan/tentris
```
The packages are also served from a second public endpoint:
```shell
conan remote add tentris https://conan.tentris.io/artifactory/api/conan/tentris
```

To use it add `dice-hash/0.5.2` to the `[requires]` section of your conan file.

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
cmake -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Release ..  -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake
make -j tests_dice_hash
./tests/tests_dice_hash
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

`DiceHash<T>` uses the policy `dice::hash::Policies::wyhash`. To use another policy, name it as the
second template argument, or use one of the aliases `DiceHashMartinus`, `DiceHashxxh3`,
`DiceHashwyhash` and `DiceHashrapidhash`:
```c++
dice::hash::DiceHash<int, dice::hash::Policies::Martinus> hash;
dice::hash::DiceHashMartinus<int> same_hash;
```
[policyUsage](examples/policyUsage.cpp) is a runnable example for this.
If you persist hash values, name the policy explicitly. Then a change of the default policy does
not change your values.

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
Declare the specialization before `DiceHash<YourType>`, or a `DiceHash` of a type that contains
`YourType`, is first instantiated as a class. For example, a member of the type
`std::unordered_set<YourType, dice::hash::DiceHash<YourType>>` instantiates it. `DiceHash` reads the
member type `is_avalanching` of the specialization (see
[Avalanching hashes of your own types](#avalanching-hashes-of-your-own-types)), so a specialization
that comes later is ill-formed: gcc reports a partial specialization after instantiation, and clang
uses the primary template, so the call of `DiceHash<YourType>` does not compile.

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

### Avalanching hashes
A hash is avalanching if every bit of the input changes each bit of the result with a probability
of about one half. A hash table can then use the lowest bits of the result directly, for example
with a mask, and needs no extra mixing step.

`DiceHash<T, Policy>` declares the member type `is_avalanching` (as `void`) exactly for the
combinations of type and policy whose result is avalanching. This is the convention of
[ankerl::unordered_dense](https://github.com/martinus/unordered_dense). A hash table can check it with
`requires { typename Hash::is_avalanching; }`. `DiceHash<T>` without a policy uses `wyhash`, so it
declares `is_avalanching` for the types of the `wyhash` column, for example integers and strings.

On a 64-bit platform:

| type | `Martinus` | `xxh3` | `wyhash` | `rapidhash` |
|---|---|---|---|---|
| integers up to 64 bits, `bool`, character types, `std::byte`, `float`, `double`, `long double` where it is not larger than `double`, pointers, `std::unique_ptr`, `std::shared_ptr` | no | yes | yes | yes |
| `__int128`, `unsigned __int128`, and `long double` where it is larger than `double` | yes | yes | yes | yes |
| strings, string views, and `std::vector`, `std::array` and `std::span` of fundamental types that are not floating point types | no | yes | yes | yes |
| `std::vector`, `std::array` and `std::span` of floating point types | yes | yes | yes | yes |
| `std::pair`, `std::tuple`, `std::optional`, `std::variant`, ordered containers, and `std::vector`, `std::array` and `std::span` of other types | yes | yes | if all parts are | if all parts are |
| unordered containers | no | no | no | no |
| `std::monostate`, `std::nullopt_t`, `std::nullptr_t` | yes | yes | yes | yes |
| types whose `dice_hash_overload` declares `is_avalanching` for the policy (see below) | yes | yes | yes | yes |
| types with another `dice_hash_overload`, and types that contain one | no | no | no | no |

Ordered containers are `std::map`, `std::set` and the types of `is_ordered_container`. Unordered
containers are `std::unordered_map`, `std::unordered_set` and the types of `is_unordered_container`.
A policy of your own has no avalanching results, except for the three types with only one value
and the types whose `dice_hash_overload` declares `is_avalanching`.

The reasons:
- `Martinus` hashes integers up to 8 bytes, `bool`, `double` and pointers with a multiplication and
  a rotation. Some input bits never change some output bits. Its byte hash is MurmurHash64A. When
  4 to 7 bytes follow the last full block of 8 bytes, some output bits flip with a probability of
  0.44 instead of 0.5. This affects `float` and every string whose length leaves such a rest. A
  `long double` that is larger than `double` is hashed with MurmurHash64A over its 10 or 16 value
  bytes, so it is avalanching. Its `hash_combine` and `HashState` mix every input hash again, so
  pairs, tuples and the other combined types are avalanching even if their parts are not (unless a
  part has a `dice_hash_overload` without a true `is_avalanching`). For example
  `std::pair<std::string, int>` is avalanching, but `std::string` and `int` are not.
- A `std::vector`, `std::array` or `std::span` of a fundamental type is hashed as one block of
  bytes, except for the floating point types. A range of floating point values is hashed value by
  value with `HashState`, as a range of pairs. So with `Martinus` a `std::vector<double>` is
  avalanching and a `std::vector<int>` is not.
- `xxh3` hashes every value and every combination with XXH3.
- `wyhash` and `rapidhash` hash every value with a function that avalanches. `wyhash` hashes
  integers up to 64 bits with `wyhash64` and `__int128` and `unsigned __int128` by their 16 bytes,
  in every `-std` mode. Their `hash_combine` and `HashState` keep the avalanche of their inputs, but
  they do not create it. So a combined type is avalanching only if all its parts are.
- `-0.0` hashes like `+0.0` with every policy. So for the inputs `+0.0` and `-0.0` the sign bit
  changes no output bit. These are two of all values of the type, so the type is still avalanching.
- The hash of an unordered container is the xor of the hashes of its elements. This keeps
  relations between results: for all values `a`, `b` and `c`,
  `h({a, b}) ^ h({a, c}) == h({b, c})`. With `xxh3`, `wyhash` and `rapidhash` a single flipped
  input bit still changes about half of the result bits, but the result is not marked for any
  policy.
- The types with only one value have no input bit that could fail the test.

[TestDiceHashAvalanche.cpp](tests/TestDiceHashAvalanche.cpp) checks every marked combination of a
list of types. It flips each input bit of many inputs and checks that each output bit flips with a
probability between 0.475 and 0.525. Types with 8 bits have only 256 values, so their bound is
wider.

#### Avalanching hashes of your own types
`DiceHash` cannot see what the `dice_hash_overload` of a type does. If its result is avalanching,
declare the member type `is_avalanching` in the specialization. If `dice_hash` returns
`dice_hash_templates<Policy>::dice_hash(v)`, use `dice::hash::avalanching_like` with the type of `v`:
```c++
struct Point { int x; int y; };
namespace dice::hash {
    template <typename Policy>
    struct dice_hash_overload<Policy, Point> {
        using is_avalanching = avalanching_like<std::pair<int, int>, Policy>;
        static std::size_t dice_hash(Point const& p) noexcept {
            return dice_hash_templates<Policy>::dice_hash(std::pair{p.x, p.y});
        }
    };
}
```
`avalanching_like<std::pair<int, int>, Policy>` is `std::true_type` exactly for the policies for
which `DiceHash<std::pair<int, int>, Policy>` declares `is_avalanching`. These are the four policies
of dice-hash, but not a policy of your own. So `DiceHash<Point, Policy>` declares `is_avalanching`
for the four policies of dice-hash. With an overload that hashes a single `int`,
`avalanching_like<int, Policy>` would be false for `Martinus`. Pairs, tuples, optionals, variants,
ordered containers, vectors, arrays and spans of `Point` follow the table above, with `Point` as an
avalanching part. With a policy of your own they are not avalanching.

`using is_avalanching = void;` promises the avalanche for every policy, also for every policy of
your own. Use it only if the result does not depend on the policy in that way, for example if
`dice_hash` applies a hash function of its own that avalanches. If `is_avalanching` names a type
with a `value` that is false, like `std::false_type`, it makes no promise. A specialization that
derives from a class with `is_avalanching`, for example another `dice_hash_overload`, gets the
promise of that class. dice-hash does not check a promise.

A hash table that honors `is_avalanching` uses the result of `DiceHash` without a mixing step of
its own. So when a specialization starts to declare `is_avalanching`, such a table places the keys
in other buckets. A table that was persisted with the old placement is not valid any more.

Declare the specialization before `DiceHash<Point, Policy>`, or a `DiceHash` of a type that contains
`Point`, is first instantiated as a class. This holds for every specialization of
`dice_hash_overload`, with or without `is_avalanching`, as described above.

### The error value
A `std::variant` which is `valueless_by_exception` holds no alternative, so no hash can be
calculated for it. In that case `DiceHash` returns the `ErrorValue` of the policy, which
`is_faulty` reports:
```c++
using Hash = dice::hash::DiceHash<std::variant<int, std::string>>;
Hash::is_faulty(Hash{}(your_variant));
```
Every other value gets a regular hash. Types which hold nothing, like `std::monostate`,
`std::nullopt` and empty containers, are regular values and are never reported as faulty.

`ErrorValue` is a sentinel, not a value outside the range of the hash functions. A regular
value can land on it, so `is_faulty` is a strong hint and not a proof. For unordered
containers this is easy to trigger on purpose, because their hash is the xor of the hashes of
their elements.

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
#include <dice/hash/blake/Blake2b.hpp>
```
For a usage examples see: [examples/blake2b.cpp](examples/blake2b.cpp).

### [Blake2Xb](https://www.blake2.net/blake2x.pdf) - arbitrary length hashing based on [Blake2b](https://www.blake2.net/)
Blake2Xb is a hash function that produces hashes of arbitrary length.

To use it you need to include
```c++
#include <dice/hash/blake/Blake2Xb.hpp>
```
For a usage examples see: [examples/blake2xb.cpp](examples/blake2xb.cpp).

### [Blake3](https://github.com/BLAKE3-team/BLAKE3-specs/blob/master/blake3.pdf) - one function, fast everywhere
Blake3 is an evolution of Blake2.

To use it you need to include
```c++
#include <dice/hash/blake/Blake3.hpp>
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
