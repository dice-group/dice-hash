#ifndef DICE_HASH_MATHENGINE_SIMPLE_HPP
#define DICE_HASH_MATHENGINE_SIMPLE_HPP

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

namespace dice::hash::internal::lthash {

	// todo deduplicate with other definitions
	template<typename T>
	inline T little_endian(T const &value) noexcept {
		if constexpr (std::endian::native == std::endian::little) {
			return value;
		} else {
			auto const *input_beg = byte_iter(value);
			auto const *input_end = input_beg + sizeof(T);
			T output;

			std::reverse_copy(input_beg, input_end, byte_iter_mut(output));
			return output;
		}
	}

	template<typename Bits>
	struct MathEngine_Simple {
#ifdef DICE_HASH_CACHE_LINE_SIZE
		static_assert(DICE_HASH_CACHE_LINE_SIZE % alignof(uint64_t) == 0,
					  "Buffer alignment must be compatible with data alignment");
		static constexpr size_t min_buffer_align = DICE_HASH_CACHE_LINE_SIZE;
#else
		static constexpr size_t min_buffer_align = alignof(uint64_t);
#endif

		static void add(std::span<std::byte const> a, std::span<std::byte const> b, std::span<std::byte> out) noexcept {
			assert(a.size() == b.size());
			assert(b.size() == out.size());
			assert(a.size() % sizeof(uint64_t) == 0);

			std::span<uint64_t const> a64{reinterpret_cast<uint64_t const *>(a.data()), a.size() / sizeof(uint64_t)};
			std::span<uint64_t const> b64{reinterpret_cast<uint64_t const *>(b.data()), b.size() / sizeof(uint64_t)};
			std::span<uint64_t> out64{reinterpret_cast<uint64_t *>(out.data()), out.size() / sizeof(uint64_t)};

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				static constexpr uint64_t mask_group_1 = Bits::bits_per_element == 16
																 ? 0xffff0000ffff0000ull
																 : 0xffffffff00000000ull;
				static constexpr uint64_t mask_group_2 = ~mask_group_1;

				for (size_t ix = 0; ix < out64.size(); ++ix) {
					auto const ai = little_endian(a64[ix]);
					auto const bi = little_endian(b64[ix]);

					auto const ai1 = ai & mask_group_1;
					auto const ai2 = ai & mask_group_2;
					auto const bi1 = bi & mask_group_1;
					auto const bi2 = bi & mask_group_2;

					uint64_t oi1 = (ai1 + bi1) & mask_group_1;
					uint64_t oi2 = (ai2 + bi2) & mask_group_2;
					out64[ix] = little_endian(oi1 | oi2);
				}
			} else {
				for (size_t ix = 0; ix < out64.size(); ++ix) {
					auto const ai = little_endian(a64[ix]);
					auto const bi = little_endian(b64[ix]);
					out64[ix] = little_endian((ai + bi) & Bits::data_mask);
				}
			}
		}

		static void sub(std::span<std::byte const> a, std::span<std::byte const> b, std::span<std::byte> out) noexcept {
			assert(a.size() == b.size());
			assert(b.size() == out.size());
			assert(a.size() % sizeof(uint64_t) == 0);

			std::span<uint64_t const> a64{reinterpret_cast<uint64_t const *>(a.data()), a.size() / sizeof(uint64_t)};
			std::span<uint64_t const> b64{reinterpret_cast<uint64_t const *>(b.data()), b.size() / sizeof(uint64_t)};
			std::span<uint64_t> out64{reinterpret_cast<uint64_t *>(out.data()), out.size() / sizeof(uint64_t)};

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				static constexpr uint64_t mask_group_1 = Bits::bits_per_element == 16
																 ? 0xffff0000ffff0000ull
																 : 0xffffffff00000000ull;
				static constexpr uint64_t mask_group_2 = ~mask_group_1;

				for (size_t ix = 0; ix < out64.size(); ++ix) {
					auto const ai = little_endian(a64[ix]);
					auto const bi = little_endian(b64[ix]);

					auto const ai1 = ai & mask_group_1;
					auto const ai2 = ai & mask_group_2;
					auto const bi1 = bi & mask_group_1;
					auto const bi2 = bi & mask_group_2;

					uint64_t oi1 = (ai1 + (mask_group_2 - bi1)) & mask_group_1;
					uint64_t oi2 = (ai2 + (mask_group_1 - bi2)) & mask_group_2;
					out64[ix] = little_endian(oi1 | oi2);
				}
			} else {
				for (size_t ix = 0; ix < out64.size(); ++ix) {
					auto const ai = little_endian(a64[ix]);
					auto const bi = little_endian(b64[ix]);
					out64[ix] = little_endian((ai + ((~Bits::data_mask - bi) & Bits::data_mask)) & Bits::data_mask);
				}
			}
		}

		static bool check_padding_bits(std::span<std::byte const> data) noexcept requires (Bits::needs_padding) {
			assert(data.size() % sizeof(uint64_t) == 0);

			std::span<uint64_t const> data64{reinterpret_cast<uint64_t const *>(data.data()), data.size() / sizeof(uint64_t)};
			for (auto const val : data64) {
				if ((little_endian(val) & ~Bits::data_mask) != 0) {
					return false;
				}
			}
			return true;
		}

		static void clear_padding_bits(std::span<std::byte> data) noexcept requires (Bits::needs_padding) {
			assert(data.size() % sizeof(uint64_t) == 0);

			if constexpr (Bits::needs_padding) {
				std::span<uint64_t> data64{reinterpret_cast<uint64_t *>(data.data()), data.size() / sizeof(uint64_t)};
				for (auto &val : data64) {
					val = little_endian(little_endian(val) & Bits::data_mask);
				}
			}
		}
	};

} // namespace dice::hash::internal::lthash

#endif//DICE_HASH_MATHENGINE_SIMPLE_HPP
