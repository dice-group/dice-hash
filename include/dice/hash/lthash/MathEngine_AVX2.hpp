#ifndef DICE_HASH_MATHENGINE_AVX2_HPP
#define DICE_HASH_MATHENGINE_AVX2_HPP

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>

#include <immintrin.h>
#include <sodium.h>

namespace dice::hash::lthash {

	template<typename Bits>
	struct MathEngine_AVX2 {
		// Note: AVX2 is x86_64 only therefore this platform must be little endian
		static_assert(std::endian::native == std::endian::little);

		// using unaligned load+store to not bloat memory with data of such high alignment
		static constexpr size_t min_buffer_align = 1;

		template<size_t DstExtent, size_t SrcExtent>
		static void add(std::span<std::byte, DstExtent> dst, std::span<std::byte const, SrcExtent> src) noexcept {
			assert(dst.size() == src.size());
			assert(dst.size() % sizeof(__m256i) == 0);

			auto *dst256 = reinterpret_cast<__m256i *>(dst.data());
			auto const *src256 = reinterpret_cast<__m256i const *>(src.data());

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				for (size_t ix = 0; ix < dst.size() / sizeof(__m256i); ++ix) {
					auto const ai = _mm256_loadu_si256(dst256 + ix);
					auto const bi = _mm256_loadu_si256(src256 + ix);

					if constexpr (Bits::bits_per_element == 16) {
						_mm256_storeu_si256(dst256 + ix, _mm256_add_epi16(ai, bi));
					} else { // Bits::bits_per_element == 32
						_mm256_storeu_si256(dst256 + ix, _mm256_add_epi32(ai, bi));
					}
				}
			} else {
				__m256i const mask = _mm256_set1_epi64x(Bits::data_mask);

				for (size_t ix = 0; ix < dst.size() / sizeof(__m256i); ++ix) {
					auto const ai = _mm256_loadu_si256(dst256 + ix);
					auto const bi = _mm256_loadu_si256(src256 + ix);

					_mm256_storeu_si256(dst256 + ix, _mm256_and_si256(_mm256_add_epi64(ai, bi),
																	 mask));
				}
			}
		}

		template<size_t DstExtent, size_t SrcExtent>
		static void sub(std::span<std::byte, DstExtent> dst, std::span<std::byte const, SrcExtent> src) noexcept {
			assert(dst.size() == src.size());
			assert(dst.size() % sizeof(__m256i) == 0);

			auto *dst256 = reinterpret_cast<__m256i *>(dst.data());
			auto const *src256 = reinterpret_cast<__m256i const *>(src.data());

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				for (size_t ix = 0; ix < dst.size() / sizeof(__m256i); ++ix) {
					auto const ai = _mm256_loadu_si256(dst256 + ix);
					auto const bi = _mm256_loadu_si256(src256 + ix);

					if constexpr (Bits::bits_per_element == 16) {
						_mm256_storeu_si256(dst256 + ix, _mm256_sub_epi16(ai, bi));
					} else { // Bits::bits_per_element == 32
						_mm256_storeu_si256(dst256 + ix, _mm256_sub_epi32(ai, bi));
					}
				}
			} else {
				__m256i const mask = _mm256_set1_epi64x(Bits::data_mask);
				__m256i const padding_mask = _mm256_set1_epi64x(~Bits::data_mask);

				for (size_t ix = 0; ix < dst.size() / sizeof(__m256i); ++ix) {
					auto const ai = _mm256_loadu_si256(dst256 + ix);
					auto const bi = _mm256_loadu_si256(src256 + ix);
					auto const inv_bi = _mm256_and_si256(_mm256_sub_epi64(padding_mask, bi),
														 mask);

					_mm256_storeu_si256(dst256 + ix, _mm256_and_si256(_mm256_add_epi64(ai, inv_bi),
																	  mask));
				}
			}
		}

		template<size_t Extent>
		static bool check_padding_bits(std::span<std::byte const, Extent> data) noexcept requires (Bits::needs_padding) {
			assert(data.size() % sizeof(__m256i) == 0);

			if constexpr (Bits::needs_padding) {
				__m256i const padding_mask = _mm256_set1_epi64x(~Bits::data_mask);
				__m256i const zero256 = _mm256_setzero_si256();

				auto const *data256 = reinterpret_cast<__m256i const *>(data.data());
				for (size_t ix = 0; ix < data.size() / sizeof(__m256i); ++ix) {
					auto const val = _mm256_loadu_si256(data256 + ix);
					auto const padding = _mm256_and_si256(val, padding_mask);

					// apparently there is no instruction that does that
					if (sodium_memcmp(&padding, &zero256, sizeof(__m256i)) != 0) {
						return false;
					}
				}
			}

			return true;
		}

		template<size_t Extent>
		static void clear_padding_bits(std::span<std::byte, Extent> data) noexcept requires (Bits::needs_padding) {
			assert(data.size() % sizeof(__m256i) == 0);

			if constexpr (Bits::needs_padding) {
				__m256i const mask = _mm256_set1_epi64x(Bits::data_mask);

				auto *data256 = reinterpret_cast<__m256i *>(data.data());
				for (size_t ix = 0; ix < data.size() / sizeof(__m256i); ++ix) {
					auto const val = _mm256_loadu_si256(data256 + ix);
					_mm256_storeu_si256(data256 + ix, _mm256_and_si256(val, mask));
				}
			}
		}
	};

} // namespace dice::hash::lthash

#endif//DICE_HASH_MATHENGINE_AVX2_HPP
