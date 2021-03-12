#define CATCH_CONFIG_MAIN// This tells Catch to provide a main() - only do this in one cpp file

#include "Dice/hash/DiceHash.hpp"
#include <catch2/catch.hpp>

#define AllPoliciesToTestForDiceHash Dice::hash::Policies::Martinus, Dice::hash::Policies::xxhash, \
									 Dice::hash::Policies::wyhash
#define AllTypesToTestForDiceHash int, long, std::size_t, std::string, std::string_view, int *, long *,                        \
								  std::string *, std::unique_ptr<int>, std::shared_ptr<int>, std::vector<int>,                 \
								  std::set<int>, std::unordered_set<int>, (std::array<int, 10>), (std::tuple<int, int, long>), \
								  (std::pair<int, int>), (std::variant<std::monostate>), (std::variant<int, float, std::string>)


namespace Dice::tests::hash {
	struct UserDefinedStruct {
		int a;
		UserDefinedStruct(int a) : a(a) {}
		friend bool operator<(UserDefinedStruct const &l, UserDefinedStruct const &r) {
			return l.a < r.a;
		}
	};

	struct ValuelessByException {
		ValuelessByException() = default;
		ValuelessByException(const ValuelessByException &) { throw std::domain_error("copy ctor"); }
	};

	template<typename Policy, typename T>
	std::size_t getHash(T const &t) {
		Dice::hash::DiceHash<T, Policy> hasher;
		return hasher(t);
	}

	bool equal(std::initializer_list<size_t> l) {
		return std::min(l) == std::max(l);
	}

	// Helper to get the first type
	template<typename First, typename...>
	struct Head {
		using type = First;
	};
	template<typename... Args>
	using Head_t = typename Head<Args...>::type;

	template<typename Policy, typename... Args>
	bool test_str_vec_arr(Args &&...args) {
		size_t str = getHash<Policy>(std::string({args...}));
		size_t vec = getHash<Policy>(std::vector({args...}));
		size_t arr = getHash<Policy>(std::array<Head_t<Args...>, sizeof...(Args)>({args...}));
		return equal({str, vec, arr});
	}

	template<typename Policy, typename... Args>
	bool test_vec_arr(Args &&...args) {
		size_t vec = getHash<Policy>(std::vector({args...}));
		size_t arr = getHash<Policy>(std::array<Head_t<Args...>, sizeof...(Args)>({args...}));
		return vec == arr;
	}

	template<typename Policy, typename T, typename V>
	bool test_pair_tuple(T const &first, V const &second) {
		size_t p = getHash<Policy>(std::pair<std::decay_t<T>, std::decay_t<V>>(first, second));
		size_t t = getHash<Policy>(std::tuple<std::decay_t<T>, std::decay_t<V>>(first, second));
		return p == t;
	}

	TEMPLATE_PRODUCT_TEST_CASE("DiceHash with default policy compiles for every type", "[DiceHash]", Dice::hash::DiceHash, (AllTypesToTestForDiceHash)) {
		TestType hasher;
	}

	TEMPLATE_TEST_CASE("DiceHash works with different Policies", "[DiceHash]", AllPoliciesToTestForDiceHash) {
		/*
        SECTION("If the hash is not defined for a specific type, it will not compile") {
            struct NotImplementedHashType {};
            NotImplementedHashType test;
            getHash<TestType>(test);
        }//*/

		SECTION("Vectors and arrays of char generate the same hash") {
			REQUIRE(test_vec_arr<TestType>('0', '1', '2', '3', '4', '5', '6', '7', '8'));
		}

		SECTION("Strings, vectors and arrays of char generate the same hash") {
			REQUIRE(test_str_vec_arr<TestType>('0', '1', '2', '3', '4', '5', '6', '7', '8'));
		}

		SECTION("Vectors and arrays of int generate the same hash (basic type)") {
			REQUIRE(test_vec_arr<TestType>(1, 2, 3, 4, 5, 6, 7, 8));
		}

		SECTION("Vectors and arrays of double generate the same hash (basic type)") {
			REQUIRE(test_vec_arr<TestType>(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0));
		}

		SECTION("Vectors and arrays of tuples generate the same hash (non-basic type)") {
			REQUIRE(test_vec_arr<TestType>(std::make_tuple(1, 2), std::make_tuple(3, 4), std::make_tuple(5, 6)));
		}

		SECTION("Pairs and tuples of size_t generate the same hash (basic type)") {
			size_t first = 12;
			size_t second = 42;
			REQUIRE(test_pair_tuple<TestType>(first, second));
		}

		SECTION("Pairs and tuples of double generate the same hash (same types)") {
			REQUIRE(test_pair_tuple<TestType>(3.14159, 4.2));
		}

		SECTION("Pairs and tuples of booleans generate the same hash (same types)") {
			REQUIRE(test_pair_tuple<TestType>(true, false));
		}

		SECTION("Pairs and tuples of double and size_t generate the same hash (mixed types)") {
			size_t second = 42;
			REQUIRE(test_pair_tuple<TestType>(3.141, second));
		}

		SECTION("Pairs and tuples of char and string generate the same hash (mixed types)") {
			REQUIRE(test_pair_tuple<TestType>('a', std::string("abc")));
		}

		SECTION("set of strings compiles") {
			std::set<std::string> exampleSet;
			exampleSet.insert("cat");
			exampleSet.insert("dog");
			exampleSet.insert("horse");
			getHash<TestType>(exampleSet);
		}

		SECTION("map of string -> int compiles") {
			std::map<std::string, int> exampleMap;
			exampleMap["cat"] = 1;
			exampleMap["horse"] = 5;
			exampleMap["dog"] = 100;
			getHash<TestType>(exampleMap);
		}

		SECTION("unordered map of string ->int compiles") {
			std::unordered_map<std::string, int> exampleMap;
			exampleMap["cat"] = 1;
			exampleMap["horse"] = 5;
			exampleMap["dog"] = 100;
			getHash<TestType>(exampleMap);
		}

		SECTION("unordered maps of string ->int are equal, if the entries are equal") {
			std::vector<std::pair<std::string, int>> entries{{"cat", 1},
															 {"horse", 5},
															 {"dog", 100}};
			std::unordered_map<std::string, int> exampleMap1(entries.begin(), entries.end());
			std::unordered_map<std::string, int> exampleMap2(entries.rbegin(), entries.rend());
			REQUIRE(getHash<TestType>(exampleMap1) == getHash<TestType>(exampleMap2));
		}

		SECTION("unordered set of integers compiles") {
			std::vector<int> entries{1, 2, 42, 512};
			std::unordered_set<int> exampleSet(entries.begin(), entries.end());
			getHash<TestType>(exampleSet);
		}

		SECTION("unordered set of strings compiles") {
			std::vector<std::string> entries{"cat", "dog", "horse"};
			std::unordered_set<std::string> exampleSet(entries.begin(), entries.end());
			getHash<TestType>(exampleSet);
		}

		SECTION("unordered sets of strings are equal if entries are equal") {
			std::vector<std::string> entries{"cat", "dog", "horse"};
			std::unordered_set<std::string> exampleSet1(entries.begin(), entries.end());
			std::unordered_set<std::string> exampleSet2(entries.rbegin(), entries.rend());
			REQUIRE(getHash<TestType>(exampleSet1) == getHash<TestType>(exampleSet2));
		}

		SECTION("Raw pointer hash themself, not the value pointed to") {
			int i = 42;
			auto raw = &i;
			auto firstHash = getHash<TestType>(raw);
			i = 43;
			auto secondHash = getHash<TestType>(raw);
			REQUIRE(firstHash == secondHash);
		}

		SECTION("Unique pointer hash the managed pointer, not the value pointed to") {
			auto smartPtr = std::make_unique<int>(42);
			REQUIRE(getHash<TestType>(smartPtr) == getHash<TestType>(smartPtr.get()));
		}

		SECTION("Shared pointer hash the managed pointer, not the value pointed to") {
			auto smartPtr = std::make_shared<int>(42);
			REQUIRE(getHash<TestType>(smartPtr) == getHash<TestType>(smartPtr.get()));
		}

		SECTION("Complicated types can be hashed (fix for the definition/declaration order bug)") {
			int i = 42;
			int *first = &i;
			int *second = &i;
			getHash<TestType>(std::make_tuple(first, second));
		}

		SECTION("Fundamental types can be hashed") {
			int i = 42;
			REQUIRE(getHash<TestType>(i) == getHash<TestType>(42));
		}

		SECTION("Variant objects can be hashed") {
			int first = 42;
			char second = 'c';
			std::string third = "42";
			std::variant<int, char, std::string> test;
			test = first;
			REQUIRE(getHash<TestType>(test) == getHash<TestType>(first));
			test = second;
			REQUIRE(getHash<TestType>(test) == getHash<TestType>(second));
			test = third;
			REQUIRE(getHash<TestType>(test) == getHash<TestType>(third));
		}
		SECTION("Variant monostate returns ErrorValue") {
			std::variant<std::monostate, int, char> test;
			REQUIRE(getHash<TestType>(test) == TestType::ErrorValue);
		}

		SECTION("Hash of ill-formed variant is the seed") {
			std::variant<int, ValuelessByException> test;
			try {
				test = ValuelessByException();
			} catch (std::domain_error const &) {}
			// now test is valueless_by_exception
			REQUIRE(getHash<TestType>(test) == TestType::ErrorValue);
		}

		SECTION("user-defined types can be used in collections") {
			std::set<UserDefinedStruct> mySet;
			mySet.insert(UserDefinedStruct(3));
			mySet.insert(UserDefinedStruct(4));
			mySet.insert(UserDefinedStruct(7));
			getHash<TestType>(mySet);
		}
		SECTION("dice_hash_invertible_combine can be called with any number of size_t") {
			std::size_t a = 3;
			std::size_t b = 4;
			std::size_t c = 7;
			std::size_t d = 42;
			Dice::hash::DiceHash<TestType>::hash_invertible_combine({a, b, c, d});
		}

		SECTION("dice_hash_invertible_combine is self inverse") {
			std::size_t a = 3;
			std::size_t b = 4;
			REQUIRE(a == Dice::hash::DiceHash<TestType>::hash_invertible_combine({a, b, a, a, b}));
		}

		SECTION("dice_hash_combine can be called with any number of size_t") {
			std::size_t a = 3;
			std::size_t b = 4;
			std::size_t c = 7;
			std::size_t d = 42;
			Dice::hash::DiceHash<TestType>::hash_combine({a, b, c, d});
		}
	}
}// namespace Dice::tests::hash

/*
* Define hash for test structures.
*/
namespace Dice::hash {
	using Dice::tests::hash::UserDefinedStruct;
	using Dice::tests::hash::ValuelessByException;

	template<typename Policy>
	struct dice_hash_overload<Policy, UserDefinedStruct> {
		static std::size_t dice_hash(UserDefinedStruct const &s) noexcept {
			return dice_hash_templates<Policy>::dice_hash(s.a);
		}
	};
	template<typename Policy>
	struct dice_hash_overload<Policy, ValuelessByException> {
		static std::size_t dice_hash(ValuelessByException const &) noexcept {
			return 0;
		}
	};
}// namespace Dice::hash