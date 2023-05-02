#ifndef DICE_HASH_TESTLTHASH_METALL_COMMON_HPP
#define DICE_HASH_TESTLTHASH_METALL_COMMON_HPP

#include <cassert>
#include <iostream>
#include <span>

#include <dice/hash/lthash/LtHash.hpp>
#include <metall/metall.hpp>

using namespace dice::hash::lthash;

using allocator_type = metall::manager::allocator_type<std::byte>;
inline constexpr char const *lthash_name = "lthash0";

template<typename Allocator>
using LtHash_t = LtHash<20, 1008, MathEngine_Simple, Allocator>;

inline std::span<std::byte const> obj = as_bytes(std::span<char const>{"spherical cow"});

void print_span(std::span<std::byte const> bytes) noexcept {
	for (auto const b : bytes) {
		std::cout << std::hex << static_cast<unsigned>(b);
	}
	std::cout << std::endl;
}

#endif//DICE_HASH_TESTLTHASH_METALL_COMMON_HPP
