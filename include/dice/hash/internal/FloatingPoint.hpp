#ifndef DICE_HASH_FLOATINGPOINT_HPP
#define DICE_HASH_FLOATINGPOINT_HPP

/** @file
 * @brief The bytes of a floating point value that the policies hash.
 *
 * Most floating point formats use every byte of their type. x87 extended precision (`long double`
 * on x86 and x86_64 with gcc and clang) differs: the value has 10 bytes, the type has 12 or 16. The
 * other bytes are padding. Their content is not defined, so two objects with the same value can
 * differ in them. `float_value_bytes` gives the bytes of a value without the padding.
 */

#include <array>
#include <bit>
#include <cstddef>
#include <cstring>
#include <limits>
#include <type_traits>

namespace dice::hash::internal {

	/** Formats of floating point values, as far as hashing needs to know them.
	 */
	enum struct FloatFormat {
		/** Every byte of the type is part of the value. */
		all_bytes,
		/** x87 extended precision: the lowest 10 bytes hold the value, the other bytes are padding. */
		x87_extended,
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

	/** The value bytes of a floating point value.
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
		return bytes;
	}
}// namespace dice::hash::internal

#endif//DICE_HASH_FLOATINGPOINT_HPP
