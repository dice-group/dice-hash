/** Fixed hash values of `Policies::rapidhash` and of the copy of `rapidhash.h` in `dice::hash::rapidhash`.
 * The expected values are those of the original `rapidhash.h` (tag `rapidhash_v3`) in its protected mode.
 * This file is built twice (see `tests/CMakeLists.txt`). `tests_rapidhash_values` defines no macro of the
 * original `rapidhash.h`. `tests_rapidhash_values_macros` defines `RAPIDHASH_FAST` and `RAPIDHASH_UNROLLED`,
 * which switch the original to its fast mode and to its unrolled loop. The copy reads neither, so both
 * builds check the same values. `DICE_HASH_TEST_RAPIDHASH_MACROS` is 0 in the first build and 1 in the second.
 */

#include <catch2/catch_all.hpp>

#include <dice/hash.hpp>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

// The copy of `rapidhash.h` undefines its own macros at its end.
#if defined(DICE_HASH_RAPIDHASH_ALWAYS_INLINE) || defined(DICE_HASH_RAPIDHASH_INLINE) || defined(DICE_HASH_RAPIDHASH_INLINE_CONSTEXPR) \
		|| defined(DICE_HASH_RAPIDHASH_LIKELY) || defined(DICE_HASH_RAPIDHASH_LITTLE_ENDIAN) || defined(DICE_HASH_RAPIDHASH_BIG_ENDIAN)
#error "the copy of rapidhash.h leaves one of its macros defined"
#endif

// The copy of `wyhash.h` undefines its own macros `DICE_HASH_WYHASH_LIKELY` and `DICE_HASH_WYHASH_UNLIKELY` at its end.
#if defined(DICE_HASH_WYHASH_LIKELY) || defined(DICE_HASH_WYHASH_UNLIKELY)
#error "the copy of wyhash.h leaves one of its macros defined"
#endif

// dice-hash defines no macro of the original `rapidhash.h`: none of its `RAPIDHASH_` macros, and not `_likely_` or
// `_unlikely_`. So it does not change the mode of the original, and the macros of the original are not defined twice.
#if defined(_likely_) || defined(_unlikely_)
#error "dice-hash defines _likely_ or _unlikely_, which the original rapidhash.h defines too"
#endif
#if !DICE_HASH_TEST_RAPIDHASH_MACROS
#if defined(RAPIDHASH_PROTECTED) || defined(RAPIDHASH_FAST) || defined(RAPIDHASH_COMPACT) || defined(RAPIDHASH_UNROLLED) \
		|| defined(RAPIDHASH_ALWAYS_INLINE) || defined(RAPIDHASH_INLINE) || defined(RAPIDHASH_INLINE_CONSTEXPR)          \
		|| defined(RAPIDHASH_NOEXCEPT) || defined(RAPIDHASH_CONSTEXPR) || defined(RAPIDHASH_LITTLE_ENDIAN)               \
		|| defined(RAPIDHASH_BIG_ENDIAN)
#error "dice-hash defines a macro of the original rapidhash.h"
#endif
#endif

namespace dice::tests::hash::rapidhash_values {
	using Policy = dice::hash::Policies::rapidhash;

	template<typename T>
	std::size_t get_hash(T const &t) {
		dice::hash::DiceHash<T, Policy> hasher;
		return hasher(t);
	}

	/** 1024 bytes from a linear congruential generator, the same on every platform. */
	std::array<unsigned char, 1024> make_buffer() {
		std::array<unsigned char, 1024> buf{};
		std::uint64_t x = 0x9e3779b97f4a7c15ull;
		for (auto &b : buf) {
			x = x * 6364136223846793005ull + 1442695040888963407ull;
			b = static_cast<unsigned char>(x >> 56);
		}
		return buf;
	}

	TEST_CASE("The test is built with the macros it names", "[rapidhash]") {
#if defined(RAPIDHASH_FAST) && defined(RAPIDHASH_UNROLLED)
		constexpr bool macros = true;
#else
		constexpr bool macros = false;
#endif
		STATIC_REQUIRE(macros == (DICE_HASH_TEST_RAPIDHASH_MACROS != 0));
	}

	TEST_CASE("hash_bytes of rapidhash keeps its values for every length", "[rapidhash]") {
		struct Case {
			std::size_t len;
			std::uint64_t hash;
		};
		// rapidhash reads 0 to 3, 4 to 7 and 8 to 16 bytes in three different ways. From 17 to 112
		// bytes it adds one step per 16 bytes. Above 112 bytes it runs a loop over 112 bytes first.
		constexpr Case cases[] = {
				{0, 12189947477442126982ull},
				{1, 8671219834948532705ull},
				{2, 15042618368266346884ull},
				{3, 13168725237526851400ull},
				{4, 16473331633147136454ull},
				{5, 4841736492744603317ull},
				{7, 13011959425012913128ull},
				{8, 10043029816270670442ull},
				{9, 553244966842135617ull},
				{15, 11948105421285538212ull},
				{16, 18186725903067585786ull},
				{17, 17899665087515879917ull},
				{31, 11268001147454662087ull},
				{32, 2560239266553624437ull},
				{33, 2492280430897391528ull},
				{48, 13656105031526515663ull},
				{49, 4856084159604209454ull},
				{64, 6850251275761776573ull},
				{65, 1383362918973674137ull},
				{80, 10188348513869253642ull},
				{81, 17746783130882982820ull},
				{96, 17848981982208620657ull},
				{97, 13629427016138448781ull},
				{112, 2458051009485733066ull},
				{113, 8088326182504788330ull},
				{128, 2572501674909016124ull},
				{224, 11715433182083373085ull},
				{225, 3738260945403479282ull},
				{226, 15235727566241125865ull},
				{1000, 1094241413705742833ull},
		};
		auto const buf = make_buffer();
		for (auto const &c : cases) {
			CAPTURE(c.len);
			CHECK(Policy::hash_bytes(buf.data(), c.len) == c.hash);
		}
	}

	TEST_CASE("hash_combine and HashState of rapidhash keep their values", "[rapidhash]") {
		CHECK(Policy::hash_combine({}) == 13679853920966426665ull);
		CHECK(Policy::hash_combine({0}) == 13679853920966426665ull);
		CHECK(Policy::hash_combine({1}) == 1ull);
		CHECK(Policy::hash_combine({42}) == 10975770812582905510ull);
		CHECK(Policy::hash_combine({2, 3}) == 10760401422104297745ull);
		CHECK(Policy::hash_combine({0x0123456789abcdefull, 0xfedcba9876543210ull, 42}) == 2600459239961407341ull);

		Policy::HashState state(3);
		state.add(0x0123456789abcdefull);
		state.add(0xfedcba9876543210ull);
		state.add(42);
		CHECK(state.digest() == 2600459239961407341ull);
	}

	TEST_CASE("DiceHash with rapidhash keeps its values", "[rapidhash]") {
		CHECK(get_hash(std::string("hello world")) == 1908365708439027566ull);
		CHECK(get_hash(std::string(64, 'a')) == 5954492890421232174ull);
		CHECK(get_hash(std::string()) == 12189947477442126982ull);
		CHECK(get_hash(std::vector<std::string>{"a", "b"}) == 1801724953763790663ull);
		// These values hash the bytes of integers, so they hold on little endian platforms only.
		if constexpr (std::endian::native == std::endian::little) {
			CHECK(get_hash(42) == 13241730303766878626ull);
			CHECK(get_hash(std::uint64_t{42}) == 2569937442799093748ull);
			CHECK(get_hash(std::vector<std::uint64_t>{1, 2, 3}) == 16965835381867387043ull);
			CHECK(get_hash(std::pair<std::uint64_t, std::uint64_t>{1, 2}) == 18152573622526020674ull);
			CHECK(get_hash(std::unordered_set<std::uint64_t>{1, 2}) == 9110286580912089635ull);
			CHECK(get_hash(std::pair<std::unordered_set<std::uint64_t>, std::uint64_t>{{}, 1}) == 5950522265253415685ull);
		}
	}

	TEST_CASE("The copy of rapidhash.h gives the values of the original in its protected mode", "[rapidhash]") {
		namespace rh = dice::hash::rapidhash;
		// The original gives 14647777377830833570 in its fast mode.
		CHECK(rh::rapidhash("abc", 3) == 6940674137509107786ull);
		CHECK(rh::rapid_mix(0x0123456789abcdefull, 0xfedcba9876543210ull) == 15918216359132636237ull);
		auto const buf = make_buffer();
		CHECK(rh::rapidhash_withSeed(buf.data(), 1000, 7) == 14032925866951752007ull);
		CHECK(rh::rapidhashMicro(buf.data(), 10) == 11064886776932023373ull);
		CHECK(rh::rapidhashMicro(buf.data(), 200) == 17709718943099738981ull);
		CHECK(rh::rapidhashNano(buf.data(), 10) == 11064886776932023373ull);
		CHECK(rh::rapidhashNano(buf.data(), 100) == 17908432128986484472ull);
	}
}// namespace dice::tests::hash::rapidhash_values
