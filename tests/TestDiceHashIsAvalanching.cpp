#include <catch2/catch_all.hpp>

#include <dice/hash.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

/** Compile-time checks of the member type `is_avalanching` of `DiceHash`.
 * The statistical checks are in TestDiceHashAvalanche.cpp.
 */
namespace dice::tests::hash::is_avalanching {
	using dice::hash::DiceHash;
	using dice::hash::Policies::Martinus;
	using dice::hash::Policies::rapidhash;
	using dice::hash::Policies::wyhash;
	using dice::hash::Policies::xxh3;

	template<typename T, typename Policy>
	inline constexpr bool marked = requires { typename DiceHash<T, Policy>::is_avalanching; };

	/** `DiceHash<T>` without a policy declares `is_avalanching`. */
	template<typename T>
	inline constexpr bool marked_default = requires { typename DiceHash<T>::is_avalanching; };

	/** With `Martinus`, `hash_bytes` does not avalanche and `HashState` does. So a vector, array or
	 * span of `T` is marked exactly if `dice_hash_templates` hashes it value by value, that is if
	 * `hash_range_as_bytes<T>` is false.
	 */
	template<typename T>
	inline constexpr bool ranges_marked_as_hashed =
			marked<std::vector<T>, Martinus> == !dice::hash::internal::hash_range_as_bytes<T>
			&& marked<std::array<T, 2>, Martinus> == !dice::hash::internal::hash_range_as_bytes<T>
			&& marked<std::span<T const>, Martinus> == !dice::hash::internal::hash_range_as_bytes<T>;

	/** A type with its own `dice_hash_overload`. */
	struct Custom {
		int a;
	};

	/** A type whose `dice_hash_overload` declares `is_avalanching` as `void`. This promises it for every
	 * policy, also for a policy of your own. The checks below test the marker, not the promise.
	 */
	struct CustomAvalanching {
		int a;
	};

	/** A type whose `dice_hash_overload` declares `is_avalanching` as `std::bool_constant`, true for
	 * every policy except `Martinus`.
	 */
	struct CustomPerPolicy {
		int a;
	};

	/** A type whose `dice_hash_overload` hashes a `std::pair<int, int>` and declares `is_avalanching` as
	 * `avalanching_like` of it.
	 */
	struct CustomLikePair {
		int a;
		int b;
	};

	/** A type whose `dice_hash_overload` hashes an `int` and declares `is_avalanching` as
	 * `avalanching_like` of it.
	 */
	struct CustomLikeInt {
		int a;
	};

	/** A policy of your own: the functions of `wyhash`, but no specialization of
	 * `avalanching_functions`.
	 */
	struct OwnPolicy : dice::hash::Policies::wyhash {};

	/** A container of the trait `is_ordered_container`. */
	struct OrderedInts {
		std::vector<int> values;
		[[nodiscard]] auto begin() const noexcept { return values.begin(); }
		[[nodiscard]] auto end() const noexcept { return values.end(); }
		[[nodiscard]] auto size() const noexcept { return values.size(); }
	};

	/** A container of the trait `is_unordered_container`. */
	struct UnorderedInts {
		std::vector<int> values;
		[[nodiscard]] auto begin() const noexcept { return values.begin(); }
		[[nodiscard]] auto end() const noexcept { return values.end(); }
		[[nodiscard]] auto size() const noexcept { return values.size(); }
	};
}// namespace dice::tests::hash::is_avalanching

namespace dice::hash {
	template<typename Policy>
	struct dice_hash_overload<Policy, dice::tests::hash::is_avalanching::Custom> {
		static std::size_t dice_hash(dice::tests::hash::is_avalanching::Custom const &c) noexcept {
			return dice_hash_templates<Policy>::dice_hash(c.a);
		}
	};

	template<typename Policy>
	struct dice_hash_overload<Policy, dice::tests::hash::is_avalanching::CustomAvalanching> {
		using is_avalanching = void;
		static std::size_t dice_hash(dice::tests::hash::is_avalanching::CustomAvalanching const &c) noexcept {
			return dice_hash_templates<Policy>::dice_hash(std::tuple{c.a});
		}
	};

	template<typename Policy>
	struct dice_hash_overload<Policy, dice::tests::hash::is_avalanching::CustomPerPolicy> {
		using is_avalanching = std::bool_constant<!std::is_same_v<Policy, Policies::Martinus>>;
		static std::size_t dice_hash(dice::tests::hash::is_avalanching::CustomPerPolicy const &c) noexcept {
			return dice_hash_templates<Policy>::dice_hash(c.a);
		}
	};

	template<typename Policy>
	struct dice_hash_overload<Policy, dice::tests::hash::is_avalanching::CustomLikePair> {
		using is_avalanching = avalanching_like<std::pair<int, int>, Policy>;
		static std::size_t dice_hash(dice::tests::hash::is_avalanching::CustomLikePair const &c) noexcept {
			return dice_hash_templates<Policy>::dice_hash(std::pair{c.a, c.b});
		}
	};

	template<typename Policy>
	struct dice_hash_overload<Policy, dice::tests::hash::is_avalanching::CustomLikeInt> {
		using is_avalanching = avalanching_like<int, Policy>;
		static std::size_t dice_hash(dice::tests::hash::is_avalanching::CustomLikeInt const &c) noexcept {
			return dice_hash_templates<Policy>::dice_hash(c.a);
		}
	};

	template<>
	struct is_ordered_container<dice::tests::hash::is_avalanching::OrderedInts> : std::true_type {};

	template<>
	struct is_unordered_container<dice::tests::hash::is_avalanching::UnorderedInts> : std::true_type {};
}// namespace dice::hash

namespace dice::tests::hash::is_avalanching {

	TEST_CASE("The test is built in the -std mode it names", "[DiceHash][is_avalanching]") {
#ifdef DICE_HASH_TEST_GNU_MODE
		constexpr bool gnu_mode = true;
#else
		constexpr bool gnu_mode = false;
#endif
#ifdef __STRICT_ANSI__
		STATIC_REQUIRE_FALSE(gnu_mode);
#else
		STATIC_REQUIRE(gnu_mode);
#endif
#ifdef __GLIBCXX__
		// libstdc++ counts `__int128` as integral only in the GNU modes. `wyhash` hashes it by its bytes
		// in both modes.
		STATIC_REQUIRE(std::is_integral_v<__int128> == gnu_mode);
#endif
	}

	TEMPLATE_TEST_CASE("is_avalanching is the same for every policy", "[DiceHash][is_avalanching]",
					   Martinus, xxh3, wyhash, rapidhash) {
		using Policy = TestType;

		SECTION("the member type is void") {
			STATIC_REQUIRE(std::is_void_v<typename DiceHash<std::monostate, Policy>::is_avalanching>);
		}

		SECTION("types with only one value are marked") {
			STATIC_REQUIRE(marked<std::monostate, Policy>);
			STATIC_REQUIRE(marked<std::nullopt_t, Policy>);
			STATIC_REQUIRE(marked<std::nullptr_t, Policy>);
			STATIC_REQUIRE(marked<std::tuple<>, Policy>);
		}

		SECTION("unordered containers are not marked") {
			STATIC_REQUIRE_FALSE(marked<std::unordered_set<int>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::unordered_set<std::string>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::unordered_map<int, int>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::unordered_map<std::string, std::pair<int, int>>, Policy>);
			STATIC_REQUIRE_FALSE(marked<UnorderedInts, Policy>);
		}

		SECTION("types with a dice_hash_overload without is_avalanching and the types that contain them are not marked") {
			STATIC_REQUIRE_FALSE(marked<Custom, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::pair<int, Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::tuple<Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::optional<Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::variant<int, Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::vector<Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::set<Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::map<int, Custom>, Policy>);
		}

		SECTION("a type whose dice_hash_overload declares is_avalanching as void is marked") {
			STATIC_REQUIRE(marked<CustomAvalanching, Policy>);
			STATIC_REQUIRE(marked<CustomAvalanching const, Policy>);
		}

		SECTION("the combined types and ordered containers of a marked overload type are marked") {
			STATIC_REQUIRE(marked<std::pair<CustomAvalanching, int>, Policy>);
			STATIC_REQUIRE(marked<std::pair<CustomAvalanching, CustomAvalanching>, Policy>);
			STATIC_REQUIRE(marked<std::tuple<CustomAvalanching, std::string>, Policy>);
			STATIC_REQUIRE(marked<std::optional<CustomAvalanching>, Policy>);
			STATIC_REQUIRE(marked<std::variant<int, CustomAvalanching>, Policy>);
			STATIC_REQUIRE(marked<std::vector<CustomAvalanching>, Policy>);
			STATIC_REQUIRE(marked<std::array<CustomAvalanching, 2>, Policy>);
			STATIC_REQUIRE(marked<std::span<CustomAvalanching const>, Policy>);
			STATIC_REQUIRE(marked<std::set<CustomAvalanching>, Policy>);
			STATIC_REQUIRE(marked<std::map<int, CustomAvalanching>, Policy>);
		}

		SECTION("a combination of a marked overload type with an overload type without is_avalanching is not marked") {
			STATIC_REQUIRE_FALSE(marked<std::pair<CustomAvalanching, Custom>, Policy>);
		}

		SECTION("an overload type with avalanching_like of a type that is marked for every policy is marked") {
			STATIC_REQUIRE(dice::hash::avalanching_like<std::pair<int, int>, Policy>::value);
			STATIC_REQUIRE(marked<CustomLikePair, Policy>);
		}

		SECTION("references in combined types are removed") {
			STATIC_REQUIRE(marked<std::tuple<int const &>, Policy>);
			STATIC_REQUIRE(marked<std::pair<std::string const &, int &>, Policy>);
		}

		SECTION("types without a hash are not marked") {
			STATIC_REQUIRE_FALSE(marked<std::vector<bool>, Policy>);
		}
	}

	TEST_CASE("is_avalanching with a policy of your own", "[DiceHash][is_avalanching]") {
		using Policy = OwnPolicy;

		SECTION("std::monostate, std::nullopt_t and std::nullptr_t are marked") {
			STATIC_REQUIRE(marked<std::monostate, Policy>);
			STATIC_REQUIRE(marked<std::nullopt_t, Policy>);
			STATIC_REQUIRE(marked<std::nullptr_t, Policy>);
		}

		SECTION("a type whose dice_hash_overload declares is_avalanching is marked") {
			STATIC_REQUIRE(marked<CustomAvalanching, Policy>);
			STATIC_REQUIRE(marked<CustomPerPolicy, Policy>);
		}

		SECTION("the types that contain it are not marked, because the combine of the policy is not known") {
			STATIC_REQUIRE_FALSE(marked<std::pair<CustomAvalanching, CustomAvalanching>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::vector<CustomAvalanching>, Policy>);
		}

		SECTION("an overload type with avalanching_like of std::pair<int, int> is not marked") {
			STATIC_REQUIRE_FALSE(marked<CustomLikePair, Policy>);
		}

		SECTION("int and std::string are not marked") {
			STATIC_REQUIRE_FALSE(marked<int, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::string, Policy>);
		}
	}

	TEST_CASE("is_avalanching with Martinus", "[DiceHash][is_avalanching]") {
		using Policy = Martinus;

		SECTION("hash_int is not avalanching") {
			STATIC_REQUIRE_FALSE(marked<bool, Policy>);
			STATIC_REQUIRE_FALSE(marked<char, Policy>);
			STATIC_REQUIRE_FALSE(marked<char32_t, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::byte, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::int8_t, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::uint16_t, Policy>);
			STATIC_REQUIRE_FALSE(marked<int, Policy>);
			STATIC_REQUIRE_FALSE(marked<long, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::size_t, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::uint64_t, Policy>);
			STATIC_REQUIRE_FALSE(marked<double, Policy>);
			STATIC_REQUIRE_FALSE(marked<int *, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::string const *, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::unique_ptr<int>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::shared_ptr<int>, Policy>);
		}

		SECTION("hash_bytes is not avalanching") {
			STATIC_REQUIRE_FALSE(marked<float, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::string, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::string_view, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::u16string, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::vector<int>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::array<char, 4>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::span<std::uint64_t const>, Policy>);
		}

		SECTION("hash_bytes over a multiple of 8 bytes is avalanching") {
			STATIC_REQUIRE(marked<__int128, Policy>);
			STATIC_REQUIRE(marked<unsigned __int128, Policy>);
		}

		SECTION("long double is avalanching where it is larger than double") {
			// x87 extended precision has 10 value bytes, IEEE binary128 and double-double have 16.
			// hash_bytes over them leaves a rest of 2 or 0 bytes. A long double of 8 bytes is hashed
			// like a double, with hash_int.
			STATIC_REQUIRE(marked<long double, Policy> == (sizeof(long double) > sizeof(double)));
		}

		SECTION("ranges of floating point values are hashed with HashState and are avalanching") {
			STATIC_REQUIRE(marked<std::vector<float>, Policy>);
			STATIC_REQUIRE(marked<std::vector<double>, Policy>);
			STATIC_REQUIRE(marked<std::vector<long double>, Policy>);
			STATIC_REQUIRE(marked<std::array<double, 2>, Policy>);
			STATIC_REQUIRE(marked<std::span<float const>, Policy>);
		}

		SECTION("hash_combine and HashState are avalanching") {
			STATIC_REQUIRE(marked<std::pair<int, int>, Policy>);
			STATIC_REQUIRE(marked<std::pair<std::string, int>, Policy>);
			STATIC_REQUIRE(marked<std::tuple<int, long, char>, Policy>);
			STATIC_REQUIRE(marked<std::tuple<std::string>, Policy>);
			STATIC_REQUIRE(marked<std::optional<int>, Policy>);
			STATIC_REQUIRE(marked<std::optional<std::string>, Policy>);
			STATIC_REQUIRE(marked<std::variant<int, std::string>, Policy>);
			STATIC_REQUIRE(marked<std::variant<std::monostate, int>, Policy>);
			STATIC_REQUIRE(marked<std::set<int>, Policy>);
			STATIC_REQUIRE(marked<std::map<int, std::string>, Policy>);
			STATIC_REQUIRE(marked<OrderedInts, Policy>);
			STATIC_REQUIRE(marked<std::vector<std::string>, Policy>);
			STATIC_REQUIRE(marked<std::vector<std::pair<int, int>>, Policy>);
			STATIC_REQUIRE(marked<std::vector<int *>, Policy>);
			STATIC_REQUIRE(marked<std::array<std::string, 2>, Policy>);
			STATIC_REQUIRE(marked<std::span<std::string const>, Policy>);
			STATIC_REQUIRE(marked<std::pair<std::unordered_set<int>, int>, Policy>);
			STATIC_REQUIRE(marked<std::vector<std::unordered_set<int>>, Policy>);
			STATIC_REQUIRE(marked<std::pair<CustomAvalanching, std::unordered_set<int>>, Policy>);
		}

		SECTION("an overload type whose is_avalanching has a false value is not marked") {
			STATIC_REQUIRE_FALSE(marked<CustomPerPolicy, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::pair<CustomPerPolicy, int>, Policy>);
			STATIC_REQUIRE_FALSE(marked<CustomLikeInt, Policy>);
		}
	}

	TEST_CASE("is_avalanching with xxh3", "[DiceHash][is_avalanching]") {
		using Policy = xxh3;

		SECTION("fundamental types and pointers are avalanching") {
			STATIC_REQUIRE(marked<bool, Policy>);
			STATIC_REQUIRE(marked<char, Policy>);
			STATIC_REQUIRE(marked<char32_t, Policy>);
			STATIC_REQUIRE(marked<std::byte, Policy>);
			STATIC_REQUIRE(marked<std::int8_t, Policy>);
			STATIC_REQUIRE(marked<std::uint16_t, Policy>);
			STATIC_REQUIRE(marked<int, Policy>);
			STATIC_REQUIRE(marked<long, Policy>);
			STATIC_REQUIRE(marked<std::size_t, Policy>);
			STATIC_REQUIRE(marked<std::uint64_t, Policy>);
			STATIC_REQUIRE(marked<__int128, Policy>);
			STATIC_REQUIRE(marked<unsigned __int128, Policy>);
			STATIC_REQUIRE(marked<float, Policy>);
			STATIC_REQUIRE(marked<double, Policy>);
			STATIC_REQUIRE(marked<long double, Policy>);
			STATIC_REQUIRE(marked<int *, Policy>);
			STATIC_REQUIRE(marked<std::string const *, Policy>);
			STATIC_REQUIRE(marked<std::unique_ptr<int>, Policy>);
			STATIC_REQUIRE(marked<std::shared_ptr<int>, Policy>);
		}

		SECTION("hash_bytes is avalanching") {
			STATIC_REQUIRE(marked<std::string, Policy>);
			STATIC_REQUIRE(marked<std::string_view, Policy>);
			STATIC_REQUIRE(marked<std::u16string, Policy>);
			STATIC_REQUIRE(marked<std::vector<int>, Policy>);
			STATIC_REQUIRE(marked<std::array<char, 4>, Policy>);
			STATIC_REQUIRE(marked<std::span<std::uint64_t const>, Policy>);
		}

		SECTION("hash_combine and HashState are avalanching") {
			STATIC_REQUIRE(marked<std::pair<int, int>, Policy>);
			STATIC_REQUIRE(marked<std::pair<std::string, int>, Policy>);
			STATIC_REQUIRE(marked<std::tuple<int, long, char>, Policy>);
			STATIC_REQUIRE(marked<std::optional<int>, Policy>);
			STATIC_REQUIRE(marked<std::variant<int, std::string>, Policy>);
			STATIC_REQUIRE(marked<std::set<int>, Policy>);
			STATIC_REQUIRE(marked<std::map<int, std::string>, Policy>);
			STATIC_REQUIRE(marked<OrderedInts, Policy>);
			STATIC_REQUIRE(marked<std::vector<std::string>, Policy>);
			STATIC_REQUIRE(marked<std::array<std::string, 2>, Policy>);
			STATIC_REQUIRE(marked<std::vector<double>, Policy>);
			STATIC_REQUIRE(marked<std::span<float const>, Policy>);
			STATIC_REQUIRE(marked<std::pair<std::unordered_set<int>, int>, Policy>);
			STATIC_REQUIRE(marked<std::vector<std::unordered_set<int>>, Policy>);
			STATIC_REQUIRE(marked<std::pair<CustomAvalanching, std::unordered_set<int>>, Policy>);
		}

		SECTION("an overload type whose is_avalanching has a true value is marked") {
			STATIC_REQUIRE(marked<CustomPerPolicy, Policy>);
			STATIC_REQUIRE(marked<std::pair<CustomPerPolicy, int>, Policy>);
			STATIC_REQUIRE(marked<CustomLikeInt, Policy>);
		}
	}

	TEMPLATE_TEST_CASE("is_avalanching with wyhash and rapidhash", "[DiceHash][is_avalanching]", wyhash, rapidhash) {
		using Policy = TestType;

		SECTION("fundamental types and pointers are avalanching") {
			STATIC_REQUIRE(marked<bool, Policy>);
			STATIC_REQUIRE(marked<char, Policy>);
			STATIC_REQUIRE(marked<char32_t, Policy>);
			STATIC_REQUIRE(marked<std::byte, Policy>);
			STATIC_REQUIRE(marked<std::int8_t, Policy>);
			STATIC_REQUIRE(marked<std::uint16_t, Policy>);
			STATIC_REQUIRE(marked<int, Policy>);
			STATIC_REQUIRE(marked<long, Policy>);
			STATIC_REQUIRE(marked<std::size_t, Policy>);
			STATIC_REQUIRE(marked<std::uint64_t, Policy>);
			STATIC_REQUIRE(marked<__int128, Policy>);
			STATIC_REQUIRE(marked<unsigned __int128, Policy>);
			STATIC_REQUIRE(marked<float, Policy>);
			STATIC_REQUIRE(marked<double, Policy>);
			STATIC_REQUIRE(marked<long double, Policy>);
			STATIC_REQUIRE(marked<int *, Policy>);
			STATIC_REQUIRE(marked<std::string const *, Policy>);
			STATIC_REQUIRE(marked<std::unique_ptr<int>, Policy>);
			STATIC_REQUIRE(marked<std::shared_ptr<int>, Policy>);
		}

		SECTION("hash_bytes is avalanching") {
			STATIC_REQUIRE(marked<std::string, Policy>);
			STATIC_REQUIRE(marked<std::string_view, Policy>);
			STATIC_REQUIRE(marked<std::u16string, Policy>);
			STATIC_REQUIRE(marked<std::vector<int>, Policy>);
			STATIC_REQUIRE(marked<std::array<char, 4>, Policy>);
			STATIC_REQUIRE(marked<std::span<std::uint64_t const>, Policy>);
		}

		SECTION("hash_combine and HashState are avalanching if all parts are") {
			STATIC_REQUIRE(marked<std::pair<int, int>, Policy>);
			STATIC_REQUIRE(marked<std::pair<std::string, int>, Policy>);
			STATIC_REQUIRE(marked<std::tuple<int, long, char>, Policy>);
			STATIC_REQUIRE(marked<std::optional<int>, Policy>);
			STATIC_REQUIRE(marked<std::variant<int, std::string>, Policy>);
			STATIC_REQUIRE(marked<std::variant<std::monostate, int>, Policy>);
			STATIC_REQUIRE(marked<std::set<int>, Policy>);
			STATIC_REQUIRE(marked<std::map<int, std::string>, Policy>);
			STATIC_REQUIRE(marked<OrderedInts, Policy>);
			STATIC_REQUIRE(marked<std::vector<std::string>, Policy>);
			STATIC_REQUIRE(marked<std::vector<int *>, Policy>);
			STATIC_REQUIRE(marked<std::array<std::string, 2>, Policy>);
			STATIC_REQUIRE(marked<std::pair<__int128, int>, Policy>);
		}

		SECTION("ranges of floating point values are hashed with HashState over avalanching parts") {
			STATIC_REQUIRE(marked<std::vector<float>, Policy>);
			STATIC_REQUIRE(marked<std::vector<double>, Policy>);
			STATIC_REQUIRE(marked<std::vector<long double>, Policy>);
			STATIC_REQUIRE(marked<std::array<double, 2>, Policy>);
			STATIC_REQUIRE(marked<std::span<float const>, Policy>);
		}

		SECTION("hash_combine and HashState are not avalanching if a part is not") {
			STATIC_REQUIRE_FALSE(marked<std::pair<std::unordered_set<int>, int>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::vector<std::unordered_set<int>>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::optional<std::unordered_set<int>>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::set<std::unordered_set<int>>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::pair<CustomAvalanching, std::unordered_set<int>>, Policy>);
		}

		SECTION("an overload type whose is_avalanching has a true value is marked") {
			STATIC_REQUIRE(marked<CustomPerPolicy, Policy>);
			STATIC_REQUIRE(marked<std::pair<CustomPerPolicy, int>, Policy>);
			STATIC_REQUIRE(marked<CustomLikeInt, Policy>);
		}
	}

	TEMPLATE_TEST_CASE("Integers with 128 bits are marked in every -std mode", "[DiceHash][is_avalanching]",
					   Martinus, xxh3, wyhash, rapidhash) {
		using Policy = TestType;
		// `wyhash` hashes integers with more than 64 bits by their bytes, also where
		// `std::is_integral_v<__int128>` is true.
		STATIC_REQUIRE(marked<__int128, Policy>);
		STATIC_REQUIRE(marked<unsigned __int128, Policy>);
		STATIC_REQUIRE(marked<std::pair<__int128, int>, Policy>);
	}

	TEST_CASE("The marker of a range follows hash_range_as_bytes", "[DiceHash][is_avalanching]") {
		STATIC_REQUIRE(ranges_marked_as_hashed<char>);
		STATIC_REQUIRE(ranges_marked_as_hashed<std::byte>);
		STATIC_REQUIRE(ranges_marked_as_hashed<int>);
		STATIC_REQUIRE(ranges_marked_as_hashed<std::uint64_t>);
		STATIC_REQUIRE(ranges_marked_as_hashed<__int128>);
		STATIC_REQUIRE(ranges_marked_as_hashed<int *>);
		STATIC_REQUIRE(ranges_marked_as_hashed<float>);
		STATIC_REQUIRE(ranges_marked_as_hashed<double>);
		STATIC_REQUIRE(ranges_marked_as_hashed<long double>);
		STATIC_REQUIRE(ranges_marked_as_hashed<std::string>);
#if defined(DICE_HASH_TEST_GNU_MODE) && defined(__GLIBCXX__) && defined(_GLIBCXX_USE_FLOAT128)
		// libstdc++ counts `__float128` as a floating point type in the GNU modes.
		STATIC_REQUIRE(ranges_marked_as_hashed<__float128>);
		STATIC_REQUIRE(marked<std::vector<__float128>, Martinus>);
#endif
		// floating point values are hashed value by value, the other fundamental types as bytes
		STATIC_REQUIRE(marked<std::vector<double>, Martinus>);
		STATIC_REQUIRE_FALSE(marked<std::vector<int>, Martinus>);

		// The checks above compare the marker with the trait that `dice_hash_templates` reads. The checks
		// below run the hash: a range is marked exactly if its hash differs from `Martinus::hash_bytes`
		// over its block of bytes.
		auto const marker_follows_path = []<typename T>(T const &x, T const &y) {
			std::vector<T> vec(2);
			std::array<T, 2> arr{};
			// Fill the bytes first, so that the padding bytes of `long double` have a value.
			for (std::size_t i = 0; i < sizeof(T) * 2; ++i) {
				reinterpret_cast<unsigned char *>(vec.data())[i] = 0x5a;
				reinterpret_cast<unsigned char *>(arr.data())[i] = 0x5a;
			}
			vec[0] = x;
			vec[1] = y;
			arr[0] = x;
			arr[1] = y;
			std::span<T const> const span{vec};
			auto const block = [](T const *data) { return Martinus::hash_bytes(data, sizeof(T) * 2); };
			return marked<std::vector<T>, Martinus> == (DiceHash<std::vector<T>, Martinus>{}(vec) != block(vec.data()))
				   && marked<std::array<T, 2>, Martinus> == (DiceHash<std::array<T, 2>, Martinus>{}(arr) != block(arr.data()))
				   && marked<std::span<T const>, Martinus> == (DiceHash<std::span<T const>, Martinus>{}(span) != block(vec.data()));
		};
		int a = 1;
		int b = 2;
		CHECK(marker_follows_path('a', 'b'));
		CHECK(marker_follows_path(std::byte{3}, std::byte{200}));
		CHECK(marker_follows_path(3, -7));
		CHECK(marker_follows_path(std::uint64_t{3}, std::uint64_t{1} << 40));
		CHECK(marker_follows_path(__int128{3}, __int128{1} << 100));
		CHECK(marker_follows_path(&a, &b));
		CHECK(marker_follows_path(1.5f, -0.25f));
		CHECK(marker_follows_path(1.5, -0.25));
		CHECK(marker_follows_path(1.5L, -0.25L));
#if defined(DICE_HASH_TEST_GNU_MODE) && defined(__GLIBCXX__) && defined(_GLIBCXX_USE_FLOAT128)
		CHECK(marker_follows_path(static_cast<__float128>(1.5), static_cast<__float128>(-0.25)));
#endif
	}

	TEST_CASE("DiceHash without a policy is marked as with wyhash", "[DiceHash][is_avalanching]") {
		STATIC_REQUIRE(marked_default<int>);
		STATIC_REQUIRE(marked_default<std::uint64_t>);
		STATIC_REQUIRE(marked_default<__int128>);
		STATIC_REQUIRE(marked_default<double>);
		STATIC_REQUIRE(marked_default<std::string>);
		STATIC_REQUIRE(marked_default<std::string_view>);
		STATIC_REQUIRE(marked_default<std::vector<int>>);
		STATIC_REQUIRE(marked_default<std::pair<int, std::string>>);
		STATIC_REQUIRE_FALSE(marked_default<std::unordered_set<int>>);
		STATIC_REQUIRE_FALSE(marked_default<std::pair<std::unordered_set<int>, int>>);
		STATIC_REQUIRE_FALSE(marked_default<Custom>);
	}
}// namespace dice::tests::hash::is_avalanching
