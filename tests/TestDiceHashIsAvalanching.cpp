#include <catch2/catch_all.hpp>

#include <dice/hash.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
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

	template<typename Policy>
	inline constexpr bool policy_marked = requires { typename Policy::is_avalanching; };

	/** A type with its own `dice_hash_overload`. */
	struct Custom {
		int a;
	};

	/** A policy of your own without `is_avalanching`. */
	struct OwnPolicy : Martinus {};

	/** A policy of your own that declares `is_avalanching`. */
	struct OwnAvalanchingPolicy : Martinus {
		using is_avalanching = void;
	};

	/** A policy of your own that derives from `wyhash` and so inherits `is_avalanching`. */
	struct OwnWyhashPolicy : wyhash {};

	/** `DiceHash<T, Policy>` is marked exactly if `Policy` declares `is_avalanching`, for each type. */
	template<typename Policy>
	inline constexpr bool marked_as_policy = [] {
		constexpr bool expected = policy_marked<Policy>;
		return marked<bool, Policy> == expected
			   && marked<int, Policy> == expected
			   && marked<std::uint64_t, Policy> == expected
			   && marked<__int128, Policy> == expected
			   && marked<double, Policy> == expected
			   && marked<int *, Policy> == expected
			   && marked<std::unique_ptr<int>, Policy> == expected
			   && marked<std::string, Policy> == expected
			   && marked<std::string_view, Policy> == expected
			   && marked<std::vector<int>, Policy> == expected
			   && marked<std::vector<double>, Policy> == expected
			   && marked<std::array<std::string, 2>, Policy> == expected
			   && marked<std::span<int const>, Policy> == expected
			   && marked<std::pair<std::string, int>, Policy> == expected
			   && marked<std::tuple<int, long, char>, Policy> == expected
			   && marked<std::tuple<int const &>, Policy> == expected
			   && marked<std::optional<int>, Policy> == expected
			   && marked<std::variant<int, std::string>, Policy> == expected
			   && marked<std::set<int>, Policy> == expected
			   && marked<std::map<int, std::string>, Policy> == expected
			   && marked<std::unordered_set<int>, Policy> == expected
			   && marked<std::unordered_map<int, int>, Policy> == expected
			   && marked<std::monostate, Policy> == expected
			   && marked<std::nullopt_t, Policy> == expected
			   && marked<Custom, Policy> == expected
			   && marked<std::pair<Custom, std::unordered_set<int>>, Policy> == expected;
	}();
}// namespace dice::tests::hash::is_avalanching

namespace dice::hash {
	template<typename Policy>
	struct dice_hash_overload<Policy, dice::tests::hash::is_avalanching::Custom> {
		static std::size_t dice_hash(dice::tests::hash::is_avalanching::Custom const &c) noexcept {
			return dice_hash_templates<Policy>::dice_hash(c.a);
		}
	};
}// namespace dice::hash

namespace dice::tests::hash::is_avalanching {
	TEST_CASE("xxh3, wyhash and rapidhash declare is_avalanching, Martinus does not", "[DiceHash][is_avalanching]") {
		STATIC_REQUIRE(std::is_void_v<xxh3::is_avalanching>);
		STATIC_REQUIRE(std::is_void_v<wyhash::is_avalanching>);
		STATIC_REQUIRE(std::is_void_v<rapidhash::is_avalanching>);
		STATIC_REQUIRE_FALSE(policy_marked<Martinus>);
	}

	TEMPLATE_TEST_CASE("DiceHash is marked exactly if the policy is", "[DiceHash][is_avalanching]",
					   xxh3, wyhash, rapidhash, Martinus, OwnPolicy, OwnAvalanchingPolicy, OwnWyhashPolicy) {
		STATIC_REQUIRE(marked_as_policy<TestType>);
	}

	TEST_CASE("The marker of DiceHash", "[DiceHash][is_avalanching]") {
		STATIC_REQUIRE(std::is_void_v<DiceHash<int, wyhash>::is_avalanching>);
		STATIC_REQUIRE(marked<std::unordered_set<int>, xxh3>);
		STATIC_REQUIRE_FALSE(marked<int, Martinus>);
		STATIC_REQUIRE(marked<int, OwnAvalanchingPolicy>);
		STATIC_REQUIRE_FALSE(marked<int, OwnPolicy>);
		// `DiceHash<T>` without a policy uses `wyhash`.
		STATIC_REQUIRE(policy_marked<DiceHash<int>>);
		// the marker adds no member data
		STATIC_REQUIRE(sizeof(DiceHash<int, wyhash>) == 1);
		STATIC_REQUIRE(sizeof(DiceHash<int, Martinus>) == 1);
	}

	/** The inputs are hashes, as `DiceHash` passes them to the combine functions. */
	TEST_CASE("DiceHash keeps the combine functions of the policy", "[DiceHash][is_avalanching]") {
		std::size_t const wy_a = wyhash::hash_fundamental(2);
		std::size_t const wy_b = wyhash::hash_fundamental(3);
		CHECK(DiceHash<int, wyhash>::hash_combine({wy_a, wy_b}) == wyhash::hash_combine({wy_a, wy_b}));
		std::size_t const martinus_a = Martinus::hash_fundamental(2);
		std::size_t const martinus_b = Martinus::hash_fundamental(3);
		CHECK(DiceHash<int, Martinus>::hash_combine({martinus_a, martinus_b}) == Martinus::hash_combine({martinus_a, martinus_b}));
		std::size_t const xxh3_a = xxh3::hash_fundamental(2);
		std::size_t const xxh3_b = xxh3::hash_fundamental(3);
		CHECK(DiceHash<int, xxh3>::hash_invertible_combine({xxh3_a, xxh3_b}) == xxh3::hash_invertible_combine({xxh3_a, xxh3_b}));
	}
}// namespace dice::tests::hash::is_avalanching

// `DiceHash<Late>` is instantiated as a class by the member of `LateHolder`, before the
// `dice_hash_overload` of `Late` is declared. The overload only has to come before the call.
namespace dice::tests::hash::is_avalanching {
	struct Late {
		int a;
		bool operator==(Late const &) const = default;
	};
	struct LateHolder {
		std::unordered_set<Late, DiceHash<Late>> set;
	};
}// namespace dice::tests::hash::is_avalanching

namespace dice::hash {
	template<typename Policy>
	struct dice_hash_overload<Policy, dice::tests::hash::is_avalanching::Late> {
		static std::size_t dice_hash(dice::tests::hash::is_avalanching::Late const &x) noexcept {
			return dice_hash_templates<Policy>::dice_hash(x.a);
		}
	};
}// namespace dice::hash

namespace dice::tests::hash::is_avalanching {
	TEST_CASE("The dice_hash_overload can follow the first instantiation of DiceHash as a class", "[DiceHash][is_avalanching]") {
		STATIC_REQUIRE(policy_marked<DiceHash<Late>>);
		LateHolder holder;
		holder.set.insert(Late{1});
		holder.set.insert(Late{2});
		CHECK(holder.set.size() == 2);
		CHECK(DiceHash<Late>{}(Late{7}) == DiceHash<int>{}(7));
	}
}// namespace dice::tests::hash::is_avalanching
