#ifndef DICE_HASH_MATHENGINE_SSE2_HPP
#define DICE_HASH_MATHENGINE_SSE2_HPP

#include <cstddef>
#include <cstdint>
#include <span>

#include <emmintrin.h>

namespace dice::hash::internal::lthash {

	template<typename Bits>
	struct MathEngine_SSE2 {
		// Note: SSE2 is x86_64 only therefore this platform must be little endian
		static_assert(std::endian::native == std::endian::little);

#ifdef DICE_HASH_CACHE_LINE_SIZE
		static_assert(DICE_HASH_CACHE_LINE_SIZE % alignof(__m128i) == 0,
					  "Buffer alignment must be compatible with data alignment");
		static constexpr size_t min_buffer_align = DICE_HASH_CACHE_LINE_SIZE;
#else
		static constexpr size_t min_buffer_align = alignof(__m128i);
#endif

		static void add(std::span<std::byte const> a, std::span<std::byte const> b, std::span<std::byte> out) noexcept {
			assert(a.size() == b.size());
			assert(b.size() == out.size());
			assert(a.size() % sizeof(__m128i) == 0);
			assert(reinterpret_cast<uintptr_t>(a.data()) % alignof(__m128i) == 0);
			assert(reinterpret_cast<uintptr_t>(b.data()) % alignof(__m128i) == 0);
			assert(reinterpret_cast<uintptr_t>(out.data()) % alignof(__m128i) == 0);

			auto const *a128 = reinterpret_cast<__m128i const *>(a.data());
			auto const *b128 = reinterpret_cast<__m128i const *>(b.data());
			auto *out128 = reinterpret_cast<__m128i *>(out.data());

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				for (size_t ix = 0; ix < a.size() / sizeof(__m128i); ++ix) {
					auto const ai = _mm_load_si128(a128 + ix);
					auto const bi = _mm_load_si128(b128 + ix);

					if constexpr (Bits::bits_per_element == 16) {
						_mm_store_si128(out128 + ix, _mm_add_epi16(ai, bi));
					} else { // Bits::bits_per_element == 32
						_mm_store_si128(out128 + ix, _mm_add_epi32(ai, bi));
					}
				}
			} else {
				__m128i const mask = _mm_set1_epi64x(Bits::data_mask);

				for (size_t ix = 0; ix < a.size() / sizeof(__m128i); ++ix) {
					auto const ai = _mm_load_si128(a128 + ix);
					auto const bi = _mm_load_si128(b128 + ix);

					_mm_store_si128(out128 + ix,
									_mm_and_si128(_mm_add_epi64(ai, bi),
												  mask));
				}
			}
		}

		static void sub(std::span<std::byte const> a, std::span<std::byte const> b, std::span<std::byte> out) noexcept {
			assert(a.size() == b.size());
			assert(b.size() == out.size());
			assert(a.size() % sizeof(__m128i) == 0);
			assert(reinterpret_cast<uintptr_t>(a.data()) % alignof(__m128i) == 0);
			assert(reinterpret_cast<uintptr_t>(b.data()) % alignof(__m128i) == 0);
			assert(reinterpret_cast<uintptr_t>(out.data()) % alignof(__m128i) == 0);

			auto const *a128 = reinterpret_cast<__m128i const *>(a.data());
			auto const *b128 = reinterpret_cast<__m128i const *>(b.data());
			auto *out128 = reinterpret_cast<__m128i *>(out.data());

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				for (size_t ix = 0; ix < a.size() / sizeof(__m128i); ++ix) {
					auto const ai = _mm_load_si128(a128 + ix);
					auto const bi = _mm_load_si128(b128 + ix);

					if constexpr (Bits::bits_per_element == 16) {
						_mm_store_si128(out128 + ix, _mm_sub_epi16(ai, bi));
					} else { // Bits::bits_per_element == 32
						_mm_store_si128(out128 + ix, _mm_sub_epi32(ai, bi));
					}
				}
			} else {
				__m128i const mask = _mm_set1_epi64x(Bits::data_mask);
				__m128i const padding_mask = _mm_set1_epi64x(~Bits::data_mask);

				for (size_t ix = 0; ix < a.size() / sizeof(__m128i); ++ix) {
					auto const ai = _mm_load_si128(a128 + ix);
					auto const bi = _mm_load_si128(b128 + ix);
					auto const inv_bi = _mm_and_si128(_mm_sub_epi64(padding_mask, bi), mask);

					_mm_store_si128(out128 + ix, _mm_and_si128(_mm_add_epi64(ai, inv_bi), mask));
				}
			}
		}

		static bool check_padding_bits(std::span<std::byte const> data) noexcept requires (Bits::needs_padding) {
			assert(data.size() % sizeof(__m128i) == 0);
			assert(reinterpret_cast<uintptr_t>(data.data()) % alignof(__m128i) == 0);

			if constexpr (Bits::needs_padding) {
				__m128i const padding_mask = _mm_set1_epi64x(~Bits::data_mask);
				__m128i const zero128 = _mm_setzero_si128();

				auto const *data128 = reinterpret_cast<__m128i const *>(data.data());
				for (size_t ix = 0; ix < data.size() / sizeof(__m128i); ++ix) {
					auto const val = _mm_load_si128(data128 + ix);
					auto const padding = _mm_and_si128(val, padding_mask);

					// apparently there is no instruction that does that
					if (sodium_memcmp(&padding, &zero128, sizeof(__m128i)) != 0) {
						return false;
					}
				}
			}

			return true;
		}

		static void clear_padding_bits(std::span<std::byte> data) noexcept requires (Bits::needs_padding) {
			assert(data.size() % sizeof(__m128i) == 0);
			assert(reinterpret_cast<uintptr_t>(data.data()) % alignof(__m128i) == 0);

			if constexpr (Bits::needs_padding) {
				__m128i const mask = _mm_set1_epi64x(Bits::data_mask);

				auto *data128 = reinterpret_cast<__m128i *>(data.data());
				for (size_t ix = 0; ix < data.size() / sizeof(__m128i); ++ix) {
					_mm_store_si128(data128 + ix,
									_mm_and_si128(_mm_load_si128(data128 + ix), mask));
				}
			}
		}
	};

} // namespace dice::hash::internal::lthash

#endif//DICE_HASH_MATHENGINE_SSE2_HPP
