#ifndef DICE_HASH_FLOATINGPOINT_HPP
#define DICE_HASH_FLOATINGPOINT_HPP

/** @file
 * @brief The bytes of a floating point value that the policies hash.
 *
 * Most floating point formats use every byte of their type. Two formats of `long double` differ:
 * - x87 extended precision (`long double` on x86 and x86_64 with gcc and clang): the value has
 *   10 bytes, the type has 12 or 16. The other bytes are padding. Their content is not defined,
 *   so two objects with the same value can differ in them.
 * - double-double (`long double` on PowerPC with the IBM format): the value is the sum of two
 *   `double`s, the high part at the lower address. Negation flips the sign of both parts, so the
 *   low part of a value can be `+0.0` or `-0.0`.
 * In every format, `+0.0` and `-0.0` compare equal and differ in the sign bit.
 * `float_value_bytes` gives the bytes of a value without the padding and turns `-0.0` into `+0.0`,
 * for double-double in both parts. Then equal values have equal bytes, except for encodings that
 * arithmetic does not produce (for example x87 pseudo-denormals). NaN is not equal to anything, so
 * its bytes do not change.
 */

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <type_traits>

namespace dice::hash::internal {

	/** Formats of floating point values, as far as hashing needs to know them.
	 */
	enum struct FloatFormat {
		/** Every byte of the type is part of the value. */
		all_bytes,
		/** x87 extended precision: the lowest 10 bytes hold the value, the other bytes are padding. */
		x87_extended,
		/** Two `double`s, the high part at the lower address. */
		double_double,
	};

	/** Finds the format of a floating point type from its properties.
	 * The arguments are the values of `std::numeric_limits` and `sizeof`, so the function can be
	 * checked for every format on every platform. The x87 format is only detected on little endian
	 * platforms, because only there the value is in the lowest 10 bytes. Formats which this function
	 * does not know are `all_bytes`. One of them is the extended precision of m68k: it has the
	 * `std::numeric_limits` of x87 and 12 bytes, but it is big endian and has 2 padding bytes between
	 * the exponent and the mantissa. So on m68k a `long double` hashes its padding bytes.
	 * @param radix `std::numeric_limits<T>::radix`
	 * @param digits `std::numeric_limits<T>::digits`
	 * @param max_exponent `std::numeric_limits<T>::max_exponent`
	 * @param size `sizeof(T)`
	 * @param little_endian true if the platform is little endian
	 * @return The format.
	 */
	constexpr FloatFormat float_format_of(int radix, int digits, int max_exponent, std::size_t size, bool little_endian) noexcept {
		if (radix == 2 && digits == 64 && max_exponent == 16384 && size > 10 && little_endian) {
			return FloatFormat::x87_extended;
		}
		if (radix == 2 && digits == 106 && max_exponent == 1024 && size == 16) {
			return FloatFormat::double_double;
		}
		return FloatFormat::all_bytes;
	}

	/** The number of bytes that hold the value, for a format and the size of its type.
	 * @param format The format.
	 * @param size `sizeof(T)`
	 * @return 10 for x87 extended precision, `size` for the other formats.
	 */
	constexpr std::size_t float_value_size_of(FloatFormat format, std::size_t size) noexcept {
		return format == FloatFormat::x87_extended ? 10 : size;
	}

	/** The format of the floating point type `T`.
	 */
	template<typename T>
	inline constexpr FloatFormat float_format = float_format_of(std::numeric_limits<T>::radix,
																std::numeric_limits<T>::digits,
																std::numeric_limits<T>::max_exponent,
																sizeof(T),
																std::endian::native == std::endian::little);

	/** The number of bytes that hold the value of the floating point type `T`.
	 */
	template<typename T>
	inline constexpr std::size_t float_value_size = float_value_size_of(float_format<T>, sizeof(T));

	/** Turns `-0.0` into `+0.0` in the bytes of one binary floating point value.
	 * A value is zero if all bits except the sign bit are zero. The sign bit is the highest bit of
	 * the last byte (little endian) or of the first byte (big endian). This holds for the binary
	 * formats of IEEE 754 and for x87 extended precision. Other values do not change.
	 * Values of 2, 4 or 8 bytes are checked as one integer, so this costs one compare for `float`
	 * and `double`.
	 * @tparam N The number of bytes of the value.
	 * @param bytes The bytes of the value.
	 * @param little_endian true if the value is stored little endian.
	 */
	template<std::size_t N>
	constexpr void fold_signed_zero(std::span<unsigned char, N> bytes, bool little_endian) noexcept {
		std::size_t const sign_byte = little_endian ? N - 1 : 0;
		bool is_zero = true;
		if constexpr (N == 2 || N == 4 || N == 8) {
			using Word = std::conditional_t<N == 2, std::uint16_t, std::conditional_t<N == 4, std::uint32_t, std::uint64_t>>;
			std::array<unsigned char, N> copy;
			std::copy(bytes.begin(), bytes.end(), copy.begin());
			auto const word = std::bit_cast<Word>(copy);
			// the position of the sign byte in `word` depends on the byte order of the platform
			std::size_t const sign_shift = 8 * (std::endian::native == std::endian::little ? sign_byte : N - 1 - sign_byte) + 7;
			is_zero = (word & static_cast<Word>(~(Word{1} << sign_shift))) == 0;
		} else {
			is_zero = (bytes[sign_byte] & 0x7fU) == 0;
			for (std::size_t i = 0; i < N; ++i) {
				if (i != sign_byte && bytes[i] != 0) {
					is_zero = false;
				}
			}
		}
		if (is_zero) {
			bytes[sign_byte] = 0;
		}
	}

	/** Brings the value bytes of a floating point value into a form in which equal values have
	 * equal bytes.
	 * `-0.0` becomes `+0.0`. For double-double, this is done for both parts. Other values, NaN
	 * included, do not change.
	 * @tparam format The format of the value.
	 * @tparam N The number of value bytes.
	 * @param bytes The value bytes.
	 * @param little_endian true if the value is stored little endian.
	 * @return The bytes in canonical form.
	 */
	template<FloatFormat format, std::size_t N>
	constexpr std::array<unsigned char, N> canonical_float_bytes(std::array<unsigned char, N> bytes, bool little_endian) noexcept {
		if constexpr (format == FloatFormat::double_double) {
			static_assert(N == 16, "double-double has two parts of 8 bytes");
			fold_signed_zero(std::span<unsigned char, 8>{bytes.data(), 8}, little_endian);
			fold_signed_zero(std::span<unsigned char, 8>{bytes.data() + 8, 8}, little_endian);
		} else {
			fold_signed_zero(std::span<unsigned char, N>{bytes}, little_endian);
		}
		return bytes;
	}

	/** The value bytes of a floating point value in canonical form (see `canonical_float_bytes`).
	 * The padding bytes of x87 extended precision are not read.
	 * @tparam T A floating point type.
	 * @param x The value.
	 * @return The `float_value_size<T>` bytes of the value.
	 */
	template<typename T>
	std::array<unsigned char, float_value_size<T>> float_value_bytes(T const &x) noexcept {
		static_assert(std::is_floating_point_v<T>);
		std::array<unsigned char, float_value_size<T>> bytes;
		std::memcpy(bytes.data(), &x, bytes.size());
		return canonical_float_bytes<float_format<T>>(bytes, std::endian::native == std::endian::little);
	}
}// namespace dice::hash::internal

#endif//DICE_HASH_FLOATINGPOINT_HPP
