/** Tests for the hash of 128-bit integers.
 * This file is built twice: once with `-std=c++20` and once with `-std=gnu++20`
 * (see `tests/CMakeLists.txt`). `DICE_HASH_TEST_STRICT_MODE` is 1 in the first build and 0 in the second.
 * In the GNU modes libstdc++ counts `__int128` as an integral type, in the strict modes it does not.
 * The hash of a 128-bit integer must not depend on this.
 */

#include <catch2/catch_all.hpp>

#include <dice/hash.hpp>

#include <type_traits>

#define AllPoliciesToTestForInt128 dice::hash::Policies::Martinus, dice::hash::Policies::xxh3, \
								   dice::hash::Policies::wyhash, dice::hash::Policies::rapidhash

namespace dice::tests::hash::int128 {

	TEST_CASE("The test is built in the -std mode it names", "[DiceHash][int128]") {
#ifdef __STRICT_ANSI__
		constexpr bool strict_mode = true;
#else
		constexpr bool strict_mode = false;
#endif
		STATIC_REQUIRE(strict_mode == (DICE_HASH_TEST_STRICT_MODE != 0));
#if defined(__SIZEOF_INT128__) && defined(__GLIBCXX__)
		// libstdc++ counts `__int128` as integral only in the GNU modes. So the GNU build takes the
		// integral path of the policies.
		STATIC_REQUIRE(std::is_integral_v<__int128> == !strict_mode);
#endif
	}

#ifdef __SIZEOF_INT128__
	template<typename Policy, typename T>
	std::size_t get_hash(T const &t) {
		dice::hash::DiceHash<T, Policy> hasher;
		return hasher(t);
	}

	inline constexpr unsigned __int128 upper_one = static_cast<unsigned __int128>(1) << 64;

	TEMPLATE_TEST_CASE("128-bit integers that differ only in the upper 64 bits hash differently", "[DiceHash][int128]", AllPoliciesToTestForInt128) {
		using Policy = TestType;

		SECTION("unsigned __int128") {
			unsigned __int128 const low = 42;
			unsigned __int128 const high = low + upper_one;
			REQUIRE(get_hash<Policy>(low) != get_hash<Policy>(high));
		}

		SECTION("__int128") {
			// -1 has all 128 bits set. 2^64 - 1 has the same lower 64 bits and zeros in the upper 64 bits.
			__int128 const minus_one = -1;
			__int128 const lower_ones = static_cast<__int128>(upper_one - 1);
			REQUIRE(get_hash<Policy>(minus_one) != get_hash<Policy>(lower_ones));
		}
	}

	TEMPLATE_TEST_CASE("A 128-bit integer is hashed by its 16 bytes in every -std mode", "[DiceHash][int128]", AllPoliciesToTestForInt128) {
		using Policy = TestType;
		// `hash_bytes` takes no type, so its result is the same in every `-std` mode.

		SECTION("unsigned __int128") {
			unsigned __int128 const value = (static_cast<unsigned __int128>(0x0123456789abcdefULL) << 64) | 0xfedcba9876543210ULL;
			REQUIRE(get_hash<Policy>(value) == Policy::hash_bytes(&value, sizeof(value)));
		}

		SECTION("__int128") {
			__int128 const value = -static_cast<__int128>((static_cast<unsigned __int128>(0x0123456789abcdefULL) << 64) | 0xfedcba9876543210ULL);
			REQUIRE(get_hash<Policy>(value) == Policy::hash_bytes(&value, sizeof(value)));
		}
	}
#else
	TEST_CASE("128-bit integers", "[DiceHash][int128]") {
		SKIP("the compiler has no __int128");
	}
#endif// __SIZEOF_INT128__

}// namespace dice::tests::hash::int128
