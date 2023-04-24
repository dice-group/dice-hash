#ifndef DICE_HASH_MATHENGINE_AVX2_HPP
#define DICE_HASH_MATHENGINE_AVX2_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace dice::hash::internal::lthash::detail {

	struct MathEngine_AVX2 {
		static void add(uint64_t mask, size_t bits_per_elem, std::span<std::byte const> a, std::span<std::byte const> b, std::span<std::byte> out) noexcept;
		static void sub(uint64_t mask, size_t bits_per_elem, std::span<std::byte const> a, std::span<std::byte const> b, std::span<std::byte> out) noexcept;
		static bool check_padding_bits(uint64_t mask, std::span<std::byte const> data) noexcept;
		static void clear_padding_bits(uint64_t mask, std::span<std::byte> buf) noexcept;
	};

} // namespace dice::hash::internal::lthash::detail

#endif//DICE_HASH_MATHENGINE_AVX2_HPP
