#ifndef DICE_HASH_MATHENGINE_SSE2_HPP
#define DICE_HASH_MATHENGINE_SSE2_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace dice::hash::internal::lthash::detail {

	template<typename Bits>
	struct MathEngine_SSE2 {
		static void add(std::span<std::byte const> a, std::span<std::byte const> b, std::span<std::byte> out) noexcept {
		}

		static void sub(std::span<std::byte const> a, std::span<std::byte const> b, std::span<std::byte> out) noexcept {
		}

		static bool check_padding_bits(std::span<std::byte const> data) noexcept requires (Bits::needs_padding) {
			return true;
		}

		static void clear_padding_bits(std::span<std::byte> buf) noexcept requires (Bits::needs_padding) {
		}
	};

} // namespace dice::hash::internal::lthash::detail

#endif//DICE_HASH_MATHENGINE_SSE2_HPP
