#include <catch2/catch_all.hpp>

#include <dice/hash.hpp>

#include <cstddef>
#include <cstdint>
#include <type_traits>
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

	/** A type with its own `dice_hash_overload`. */
	struct Custom {
		int a;
	};

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

	template<>
	struct is_ordered_container<dice::tests::hash::is_avalanching::OrderedInts> : std::true_type {};

	template<>
	struct is_unordered_container<dice::tests::hash::is_avalanching::UnorderedInts> : std::true_type {};
}// namespace dice::hash

namespace dice::tests::hash::is_avalanching {

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

		SECTION("types with a dice_hash_overload and the types that contain them are not marked") {
			STATIC_REQUIRE_FALSE(marked<Custom, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::pair<int, Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::tuple<Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::optional<Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::variant<int, Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::vector<Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::set<Custom>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::map<int, Custom>, Policy>);
		}

		SECTION("types without a hash are not marked") {
			STATIC_REQUIRE_FALSE(marked<std::vector<bool>, Policy>);
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
			STATIC_REQUIRE(marked<std::pair<std::unordered_set<int>, int>, Policy>);
			STATIC_REQUIRE(marked<std::vector<std::unordered_set<int>>, Policy>);
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
			STATIC_REQUIRE(marked<float, Policy>);
			STATIC_REQUIRE(marked<double, Policy>);
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
		}

		SECTION("hash_combine and HashState are not avalanching if a part is not") {
			STATIC_REQUIRE_FALSE(marked<std::pair<std::unordered_set<int>, int>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::vector<std::unordered_set<int>>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::optional<std::unordered_set<int>>, Policy>);
			STATIC_REQUIRE_FALSE(marked<std::set<std::unordered_set<int>>, Policy>);
		}
	}

	TEST_CASE("is_avalanching of integers with 128 bits", "[DiceHash][is_avalanching]") {
		// wyhash64 takes 64 bits. If __int128 is an integral type, wyhash loses its upper 64 bits.
		STATIC_REQUIRE(marked<__int128, wyhash> == !std::is_integral_v<__int128>);
		STATIC_REQUIRE(marked<unsigned __int128, wyhash> == !std::is_integral_v<unsigned __int128>);
		STATIC_REQUIRE(marked<std::pair<__int128, int>, wyhash> == !std::is_integral_v<__int128>);
		STATIC_REQUIRE(marked<__int128, rapidhash>);
		STATIC_REQUIRE(marked<unsigned __int128, rapidhash>);
	}
}// namespace dice::tests::hash::is_avalanching
