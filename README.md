# dice-hash: High-Performance Hash for Hash-Based Container

dice-hash is based on the hash from [robin-hood-hashing hash map/set](https://github.com/martinus/robin-hood-hashing) by [Martin Ankerl](https://martin.ankerl.com/). It performs well as hash function for most hash-based containers like `std::unordered_map/set` or [`tsl::sparse_map/set`](https://github.com/Tessil/sparse-map). 
dice-hash is an extended version with support for all kinds of fixed, ordered and unordered data structures from STL. 

## Requirements

A C++20 compatible compiler. Code was only tested on x86_64.

