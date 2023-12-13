#ifndef DICE_HASH_MATHENGINE_SSE2_HPP
#define DICE_HASH_MATHENGINE_SSE2_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>

#include <emmintrin.h>

namespace dice::hash::lthash {

	template<typename Bits>
	struct MathEngine_SSE2 {
		// Note: SSE2 is x86_64 only therefore this platform must be little endian
		static_assert(std::endian::native == std::endian::little);

		// using unaligned load+store to not bloat memory with data of such high alignment
		static constexpr size_t min_buffer_align = 1;

		template<size_t DstExtent, size_t SrcExtent>
		static void add(std::span<std::byte, DstExtent> dst, std::span<std::byte const, SrcExtent> src) noexcept {
			assert(dst.size() == src.size());
			assert(dst.size() % sizeof(__m128i) == 0);

			auto *dst128 = reinterpret_cast<__m128i *>(dst.data());
			auto const *src128 = reinterpret_cast<__m128i const *>(src.data());

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				for (size_t ix = 0; ix < dst.size() / sizeof(__m128i); ++ix) {
					auto const ai = _mm_loadu_si128(dst128 + ix);
					auto const bi = _mm_loadu_si128(src128 + ix);

					if constexpr (Bits::bits_per_element == 16) {
						_mm_storeu_si128(dst128 + ix, _mm_add_epi16(ai, bi));
					} else { // Bits::bits_per_element == 32
						_mm_storeu_si128(dst128 + ix, _mm_add_epi32(ai, bi));
					}
				}
			} else {
				__m128i const mask = _mm_set1_epi64x(Bits::data_mask);

				for (size_t ix = 0; ix < dst.size() / sizeof(__m128i); ++ix) {
					auto const ai = _mm_loadu_si128(dst128 + ix);
					auto const bi = _mm_loadu_si128(src128 + ix);

					_mm_storeu_si128(dst128 + ix, _mm_and_si128(_mm_add_epi64(ai, bi),
																mask));
				}
			}
		}

		template<size_t DstExtent, size_t SrcExtent>
		static void sub(std::span<std::byte, DstExtent> dst, std::span<std::byte const, SrcExtent> src) noexcept {
			assert(dst.size() == src.size());
			assert(dst.size() % sizeof(__m128i) == 0);

			auto *dst128 = reinterpret_cast<__m128i *>(dst.data());
			auto const *src128 = reinterpret_cast<__m128i const *>(src.data());

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				for (size_t ix = 0; ix < dst.size() / sizeof(__m128i); ++ix) {
					auto const ai = _mm_loadu_si128(dst128 + ix);
					auto const bi = _mm_loadu_si128(src128 + ix);

					if constexpr (Bits::bits_per_element == 16) {
						_mm_storeu_si128(dst128 + ix, _mm_sub_epi16(ai, bi));
					} else { // Bits::bits_per_element == 32
						_mm_storeu_si128(dst128 + ix, _mm_sub_epi32(ai, bi));
					}
				}
			} else {
				__m128i const mask = _mm_set1_epi64x(Bits::data_mask);
				__m128i const padding_mask = _mm_set1_epi64x(~Bits::data_mask);

				for (size_t ix = 0; ix < dst.size() / sizeof(__m128i); ++ix) {
					auto const ai = _mm_loadu_si128(dst128 + ix);
					auto const bi = _mm_loadu_si128(src128 + ix);
					auto const inv_bi = _mm_and_si128(_mm_sub_epi64(padding_mask, bi), mask);

					_mm_storeu_si128(dst128 + ix, _mm_and_si128(_mm_add_epi64(ai, inv_bi),
																mask));
				}
			}
		}

		template<size_t Extent>
		static bool check_padding_bits(std::span<std::byte const, Extent> data) noexcept requires (Bits::needs_padding) {
			assert(data.size() % sizeof(__m128i) == 0);

			if constexpr (Bits::needs_padding) {
				__m128i const padding_mask = _mm_set1_epi64x(~Bits::data_mask);
				__m128i const zero128 = _mm_setzero_si128();

				auto const *data128 = reinterpret_cast<__m128i const *>(data.data());
				for (size_t ix = 0; ix < data.size() / sizeof(__m128i); ++ix) {
					auto const val = _mm_loadu_si128(data128 + ix);
					auto const padding = _mm_and_si128(val, padding_mask);

					// apparently there is no instruction that does that
					if (sodium_memcmp(&padding, &zero128, sizeof(__m128i)) != 0) {
						return false;
					}
				}
			}

			return true;
		}

		template<size_t Extent>
		static void clear_padding_bits(std::span<std::byte, Extent> data) noexcept requires (Bits::needs_padding) {
			assert(data.size() % sizeof(__m128i) == 0);

			if constexpr (Bits::needs_padding) {
				__m128i const mask = _mm_set1_epi64x(Bits::data_mask);

				auto *data128 = reinterpret_cast<__m128i *>(data.data());
				for (size_t ix = 0; ix < data.size() / sizeof(__m128i); ++ix) {
					_mm_storeu_si128(data128 + ix,
									 _mm_and_si128(_mm_loadu_si128(data128 + ix), mask));
				}
			}
		}
	};

} // namespace dice::hash::lthash

#endif//DICE_HASH_MATHENGINE_SSE2_HPP
