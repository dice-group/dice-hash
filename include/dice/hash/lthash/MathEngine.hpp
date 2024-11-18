#ifndef DICE_HASH_MATHENGINE_HPP
#define DICE_HASH_MATHENGINE_HPP

#include "dice/hash/lthash/MathEngine_Hwy.hpp"
#include "dice/hash/lthash/MathEngine_Simple.hpp"

namespace dice::hash::lthash {
	template<typename B>
	concept UnpaddedBits = requires {
		{ B::bits_per_element } -> std::convertible_to<size_t>;
	};

	template<typename B>
	concept PaddedBits = requires {
		requires UnpaddedBits<B>;
		{ B::needs_padding } -> std::convertible_to<std::true_type>;
		{ B::data_mask } -> std::convertible_to<uint64_t>;
	};

	template<template<typename> typename ME, typename B>
	concept UnpaddedMathEngine = requires (std::span<std::byte> dst, std::span<std::byte const> src) {
		requires UnpaddedBits<B>;
		{ ME<B>::min_buffer_align } -> std::convertible_to<size_t>;
		ME<B>::add(dst, src);
		ME<B>::sub(dst, src);
	};

	template<template<typename> typename ME, typename B>
	concept PaddedMathEngine = requires (std::span<std::byte const> data, std::span<std::byte> out) {
		requires PaddedBits<B>;
		{ ME<B>::check_padding_bits(data) } -> std::convertible_to<bool>;
		ME<B>::clear_padding_bits(out);
	};

	template<template<typename> typename ME, typename B>
	concept MathEngine = UnpaddedMathEngine<ME, B> && (!PaddedBits<B> || PaddedMathEngine<ME, B>);

	template<typename Bits>
	using DefaultMathEngine = MathEngine_Hwy<Bits>;
} // namespace dice::hash::lthash

#endif//DICE_HASH_MATHENGINE_HPP
