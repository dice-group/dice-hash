#ifndef DICE_HASH_MATHENGINE_HPP
#define DICE_HASH_MATHENGINE_HPP

#ifdef __AVX2__
#include "dice/hash/lthash/MathEngine_AVX2.hpp"
#endif

#ifdef __SSE2__
#include "dice/hash/lthash/MathEngine_SSE2.hpp"
#endif

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

#if defined(__AVX2__)
	template<typename Bits>
	using DefaultMathEngine = MathEngine_AVX2<Bits>;
#elif defined(__SSE2__)
	template<typename Bits>
	using DefaultMathEngine = MathEngine_SSE2<Bits>;
#else
	template<typename Bits>
	using DefaultMathEngine = MathEngine_Simple<Bits>;
#endif

} // namespace dice::hash::lthash

#endif//DICE_HASH_MATHENGINE_HPP
