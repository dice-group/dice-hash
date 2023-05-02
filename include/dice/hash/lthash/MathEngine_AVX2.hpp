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

#ifdef DICE_HASH_CACHE_LINE_SIZE
		static_assert(DICE_HASH_CACHE_LINE_SIZE % alignof(__m256i) == 0,
					  "Buffer alignment must be compatible with data alignment");
		static constexpr size_t min_buffer_align = DICE_HASH_CACHE_LINE_SIZE;
#else
		static constexpr size_t min_buffer_align = alignof(__m256i);
#endif

		static void add(std::span<std::byte const> a, std::span<std::byte const> b, std::span<std::byte> out) noexcept {
			assert(a.size() == b.size());
			assert(b.size() == out.size());
			assert(a.size() % sizeof(__m256i) == 0);
			assert(reinterpret_cast<uintptr_t>(a.data()) % alignof(__m256i) == 0);
			assert(reinterpret_cast<uintptr_t>(b.data()) % alignof(__m256i) == 0);
			assert(reinterpret_cast<uintptr_t>(out.data()) % alignof(__m256i) == 0);

			auto const *a256 = reinterpret_cast<__m256i const *>(a.data());
			auto const *b256 = reinterpret_cast<__m256i const *>(b.data());
			auto *out256 = reinterpret_cast<__m256i *>(out.data());

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				for (size_t ix = 0; ix < a.size() / sizeof(__m256i); ++ix) {
					auto const ai = _mm256_load_si256(a256 + ix);
					auto const bi = _mm256_load_si256(b256 + ix);

					if constexpr (Bits::bits_per_element == 16) {
						_mm256_store_si256(out256 + ix, _mm256_add_epi16(ai, bi));
					} else { // Bits::bits_per_element == 32
						_mm256_store_si256(out256 + ix, _mm256_add_epi32(ai, bi));
					}
				}
			} else {
				__m256i const mask = _mm256_set1_epi64x(Bits::data_mask);

				for (size_t ix = 0; ix < a.size() / sizeof(__m256i); ++ix) {
					auto const ai = _mm256_load_si256(a256 + ix);
					auto const bi = _mm256_load_si256(b256 + ix);

					_mm256_store_si256(out256 + ix,
									   _mm256_and_si256(_mm256_add_epi64(ai, bi),
														mask));
				}
			}
		}

		static void sub(std::span<std::byte const> a, std::span<std::byte const> b, std::span<std::byte> out) noexcept {
			assert(a.size() == b.size());
			assert(b.size() == out.size());
			assert(a.size() % sizeof(__m256i) == 0);
			assert(reinterpret_cast<uintptr_t>(a.data()) % alignof(__m256i) == 0);
			assert(reinterpret_cast<uintptr_t>(b.data()) % alignof(__m256i) == 0);
			assert(reinterpret_cast<uintptr_t>(out.data()) % alignof(__m256i) == 0);

			auto const *a256 = reinterpret_cast<__m256i const *>(a.data());
			auto const *b256 = reinterpret_cast<__m256i const *>(b.data());
			auto *out256 = reinterpret_cast<__m256i *>(out.data());

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				for (size_t ix = 0; ix < a.size() / sizeof(__m256i); ++ix) {
					auto const ai = _mm256_load_si256(a256 + ix);
					auto const bi = _mm256_load_si256(b256 + ix);

					if constexpr (Bits::bits_per_element == 16) {
						_mm256_store_si256(out256 + ix, _mm256_sub_epi16(ai, bi));
					} else { // Bits::bits_per_element == 32
						_mm256_store_si256(out256 + ix, _mm256_sub_epi32(ai, bi));
					}
				}
			} else {
				__m256i const mask = _mm256_set1_epi64x(Bits::data_mask);
				__m256i const padding_mask = _mm256_set1_epi64x(~Bits::data_mask);

				for (size_t ix = 0; ix < a.size() / sizeof(__m256i); ++ix) {
					auto const ai = _mm256_load_si256(a256 + ix);
					auto const bi = _mm256_load_si256(b256 + ix);
					auto const inv_bi = _mm256_and_si256(_mm256_sub_epi64(padding_mask, bi), mask);

					_mm256_store_si256(out256 + ix, _mm256_and_si256(_mm256_add_epi64(ai, inv_bi), mask));
				}
			}
		}

		static bool check_padding_bits(std::span<std::byte const> data) noexcept requires (Bits::needs_padding) {
			assert(data.size() % sizeof(__m256i) == 0);
			assert(reinterpret_cast<uintptr_t>(data.data()) % alignof(__m256i) == 0);

			if constexpr (Bits::needs_padding) {
				__m256i const padding_mask = _mm256_set1_epi64x(~Bits::data_mask);
				__m256i const zero256 = _mm256_setzero_si256();

				auto const *data256 = reinterpret_cast<__m256i const *>(data.data());
				for (size_t ix = 0; ix < data.size() / sizeof(__m256i); ++ix) {
					auto const val = _mm256_load_si256(data256 + ix);
					auto const padding = _mm256_and_si256(val, padding_mask);

					// apparently there is no instruction that does that
					if (sodium_memcmp(&padding, &zero256, sizeof(__m256i)) != 0) {
						return false;
					}
				}
			}

			return true;
		}

		static void clear_padding_bits(std::span<std::byte> data) noexcept requires (Bits::needs_padding) {
			assert(data.size() % sizeof(__m256i) == 0);
			assert(reinterpret_cast<uintptr_t>(data.data()) % alignof(__m256i) == 0);

			if constexpr (Bits::needs_padding) {
				__m256i const mask = _mm256_set1_epi64x(Bits::data_mask);

				auto *data256 = reinterpret_cast<__m256i *>(data.data());
				for (size_t ix = 0; ix < data.size() / sizeof(__m256i); ++ix) {
					_mm256_store_si256(data256 + ix,
									   _mm256_and_si256(_mm256_load_si256(data256 + ix), mask));
				}
			}
		}
	};

} // namespace dice::hash::lthash

#endif//DICE_HASH_MATHENGINE_AVX2_HPP
