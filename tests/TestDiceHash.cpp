#include <catch2/catch_all.hpp>

#include <dice/hash.hpp>

#include <cstring>
#include <limits>

#define AllPoliciesToTestForDiceHash dice::hash::Policies::Martinus, dice::hash::Policies::xxh3, \
									 dice::hash::Policies::wyhash, dice::hash::Policies::rapidhash
#define AllTypesToTestForDiceHash int, long, std::size_t, std::byte, __int128, unsigned __int128, std::string, std::string_view, int *, long *,             \
								  std::string *, std::unique_ptr<int>, std::shared_ptr<int>, std::vector<int>,                 \
								  std::set<int>, std::unordered_set<int>, (std::array<int, 10>), (std::tuple<int, int, long>), \
								  (std::pair<int, int>), (std::variant<std::monostate>), (std::variant<int, float, std::string>),           \
								  (std::optional<std::string>), (std::optional<int>), (std::optional<std::monostate>), std::nullopt_t


namespace dice::tests::hash {
	struct UserDefinedStruct {
		int a;
		UserDefinedStruct(int a) : a(a) {}
		friend bool operator<(UserDefinedStruct const &l, UserDefinedStruct const &r) {
			return l.a < r.a;
		}
	};

	/** Second struct with the same content as UserDefinedStruct.
	 * Both hash to the same value, so a variant of them can only be told apart by its index.
	 */
	struct OtherUserDefinedStruct {
		int a;
		OtherUserDefinedStruct(int a) : a(a) {}
	};

	struct ValuelessByException {
		ValuelessByException() = default;
		ValuelessByException(const ValuelessByException &) { throw std::domain_error("copy ctor"); }
		ValuelessByException &operator=(const ValuelessByException &) { throw std::domain_error("copy assignment"); }
	};

	template<typename Policy, typename T>
	std::size_t getHash(T const &t) {
		dice::hash::DiceHash<T, Policy> hasher;
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
		std::vector const vec{args...};
		std::array<Head_t<Args...>, sizeof...(Args)> arr{args...};
		std::span span1{vec};
		std::span span2{arr};

		size_t vech = getHash<Policy>(vec);
		size_t arrh = getHash<Policy>(arr);
		size_t spanh1 = getHash<Policy>(span1);
		size_t spanh2 = getHash<Policy>(span2);
		return equal({vech, arrh, spanh1, spanh2});
	}

	template<typename Policy, typename T, typename V>
	bool test_pair_tuple(T const &first, V const &second) {
		size_t p = getHash<Policy>(std::pair<std::decay_t<T>, std::decay_t<V>>(first, second));
		size_t t = getHash<Policy>(std::tuple<std::decay_t<T>, std::decay_t<V>>(first, second));
		return p == t;
	}

	TEMPLATE_PRODUCT_TEST_CASE("DiceHash with default policy compiles for every type", "[DiceHash]", dice::hash::DiceHash, (AllTypesToTestForDiceHash)) {
		TestType hasher;
		(void) hasher;
	}

	TEMPLATE_TEST_CASE("DiceHash works with different Policies", "[DiceHash]", AllPoliciesToTestForDiceHash) {
		using CurrentPolicy = TestType;
		/*
        SECTION("If the hash is not defined for a specific type, it will not compile") {
            struct NotImplementedHashType {};
            NotImplementedHashType test;
            getHash<CurrentPolicy>(test);
        }//*/

		SECTION("Vectors and arrays of char generate the same hash") {
			REQUIRE(test_vec_arr<CurrentPolicy>('0', '1', '2', '3', '4', '5', '6', '7', '8'));
		}

		SECTION("Strings, vectors and arrays of char generate the same hash") {
			REQUIRE(test_str_vec_arr<CurrentPolicy>('0', '1', '2', '3', '4', '5', '6', '7', '8'));
		}

		SECTION("Vectors and arrays of int generate the same hash (basic type)") {
			REQUIRE(test_vec_arr<CurrentPolicy>(1, 2, 3, 4, 5, 6, 7, 8));
		}

		SECTION("Vectors and arrays of double generate the same hash (basic type)") {
			REQUIRE(test_vec_arr<CurrentPolicy>(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0));
		}

		SECTION("Vectors and arrays of std::byte generate the same hash (basic type)") {
			REQUIRE(test_vec_arr<CurrentPolicy>(std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}, std::byte{5}, std::byte{6}, std::byte{7}, std::byte{8}, std::byte{9}));
		}

		SECTION("Vectors and arrays of tuples generate the same hash (non-basic type)") {
			REQUIRE(test_vec_arr<CurrentPolicy>(std::make_tuple(1, 2), std::make_tuple(3, 4), std::make_tuple(5, 6)));
		}

		SECTION("Pairs and tuples of size_t generate the same hash (basic type)") {
			size_t first = 12;
			size_t second = 42;
			REQUIRE(test_pair_tuple<CurrentPolicy>(first, second));
		}

		SECTION("Pairs and tuples of double generate the same hash (same types)") {
			REQUIRE(test_pair_tuple<CurrentPolicy>(3.14159, 4.2));
		}

		SECTION("Pairs and tuples of booleans generate the same hash (same types)") {
			REQUIRE(test_pair_tuple<CurrentPolicy>(true, false));
		}

		SECTION("Pairs and tuples of double and size_t generate the same hash (mixed types)") {
			size_t second = 42;
			REQUIRE(test_pair_tuple<CurrentPolicy>(3.141, second));
		}

		SECTION("Pairs and tuples of char and string generate the same hash (mixed types)") {
			REQUIRE(test_pair_tuple<CurrentPolicy>('a', std::string("abc")));
		}

		SECTION("unoccupied optional and monostate don't generate the same hash") {
			std::optional<std::string> example;
			std::monostate target{};
			REQUIRE(getHash<CurrentPolicy>(target) != getHash<CurrentPolicy>(example));
		}

		SECTION("unoccupied optional and nullopt_t don't generate the same hash") {
			std::optional<std::string> example;
			std::nullopt_t target{std::nullopt};
			REQUIRE(getHash<CurrentPolicy>(target) != getHash<CurrentPolicy>(example));
		}

		SECTION("Some option and target don't generate the same hash") {
			std::optional<std::string> example{"dog"};
			std::string target{"dog"};
			REQUIRE(getHash<CurrentPolicy>(target) != getHash<CurrentPolicy>(example));
		}

		SECTION("Some option(monostate) and monostate don't generate the same hash") {
			std::optional example{std::monostate{}};
			std::monostate target{};
			REQUIRE(getHash<CurrentPolicy>(target) != getHash<CurrentPolicy>(example));
		}

		SECTION("unoccupied optional(monostate) and occupied optional(monostate) don't generate the same hash") {
			std::optional<std::monostate> example;
			std::optional target{std::monostate{}};

			REQUIRE(getHash<CurrentPolicy>(target) != getHash<CurrentPolicy>(example));
		}

		SECTION("(index based(1), Some option) and target generate the same hash") {
			std::optional<std::string> example{"dog"};
			std::string target{"dog"};

			REQUIRE(getHash<CurrentPolicy>(std::make_tuple(std::size_t{1}, target)) == getHash<CurrentPolicy>(example));
		}

		SECTION("(index based(0), unoccupied optional) and target generate the same hash") {
			std::optional<std::string> example;
			std::nullopt_t target{std::nullopt};

			REQUIRE(getHash<CurrentPolicy>(std::make_tuple(std::size_t{0}, target)) == getHash<CurrentPolicy>(example));
		}

		SECTION("An unoccupied optional and a variant holding monostate don't generate the same hash") {
			std::optional<int> example;
			std::variant<std::monostate, int> target;

			REQUIRE(getHash<CurrentPolicy>(target) != getHash<CurrentPolicy>(example));
		}

		SECTION("nullopt_t and monostate don't generate the same hash") {
			std::nullopt_t example{std::nullopt};
			std::monostate target{};
			REQUIRE(getHash<CurrentPolicy>(target) != getHash<CurrentPolicy>(example));
		}

		SECTION("set of optionals compiles") {
			std::set<std::optional<std::string>> exampleSet;
			exampleSet.insert("cat");
			exampleSet.insert(std::nullopt);
			exampleSet.insert("horse");
			getHash<CurrentPolicy>(exampleSet);
		}

		SECTION("arr of optionals compiles") {
			std::array<std::optional<std::string>, 5> exampleArray{"dog", std::nullopt, "cat", std::nullopt, "horse"};
			getHash<CurrentPolicy>(exampleArray);
		}

		SECTION("vec of optionals compiles") {
			std::vector<std::optional<std::string>> exampleVec{"dog", std::nullopt, "cat", std::nullopt, "horse"};
			getHash<CurrentPolicy>(exampleVec);
		}

		SECTION("set of strings compiles") {
			std::set<std::string> exampleSet;
			exampleSet.insert("cat");
			exampleSet.insert("dog");
			exampleSet.insert("horse");
			getHash<CurrentPolicy>(exampleSet);
		}

		SECTION("map of string -> int compiles") {
			std::map<std::string, int> exampleMap;
			exampleMap["cat"] = 1;
			exampleMap["horse"] = 5;
			exampleMap["dog"] = 100;
			getHash<CurrentPolicy>(exampleMap);
		}

		SECTION("unordered map of string ->int compiles") {
			std::unordered_map<std::string, int> exampleMap;
			exampleMap["cat"] = 1;
			exampleMap["horse"] = 5;
			exampleMap["dog"] = 100;
			getHash<CurrentPolicy>(exampleMap);
		}

		SECTION("unordered maps of string ->int are equal, if the entries are equal") {
			std::vector<std::pair<std::string, int>> entries{{"cat", 1},
															 {"horse", 5},
															 {"dog", 100}};
			std::unordered_map<std::string, int> exampleMap1(entries.begin(), entries.end());
			std::unordered_map<std::string, int> exampleMap2(entries.rbegin(), entries.rend());
			REQUIRE(getHash<CurrentPolicy>(exampleMap1) == getHash<CurrentPolicy>(exampleMap2));
		}

		SECTION("unordered set of integers compiles") {
			std::vector<int> entries{1, 2, 42, 512};
			std::unordered_set<int> exampleSet(entries.begin(), entries.end());
			getHash<CurrentPolicy>(exampleSet);
		}

		SECTION("unordered set of strings compiles") {
			std::vector<std::string> entries{"cat", "dog", "horse"};
			std::unordered_set<std::string> exampleSet(entries.begin(), entries.end());
			getHash<CurrentPolicy>(exampleSet);
		}

		SECTION("unordered sets of strings are equal if entries are equal") {
			std::vector<std::string> entries{"cat", "dog", "horse"};
			std::unordered_set<std::string> exampleSet1(entries.begin(), entries.end());
			std::unordered_set<std::string> exampleSet2(entries.rbegin(), entries.rend());
			REQUIRE(getHash<CurrentPolicy>(exampleSet1) == getHash<CurrentPolicy>(exampleSet2));
		}

		SECTION("Raw pointer hash themself, not the value pointed to") {
			int i = 42;
			auto raw = &i;
			auto firstHash = getHash<CurrentPolicy>(raw);
			i = 43;
			auto secondHash = getHash<CurrentPolicy>(raw);
			REQUIRE(firstHash == secondHash);
		}

		SECTION("Unique pointer hash the managed pointer, not the value pointed to") {
			auto smartPtr = std::make_unique<int>(42);
			REQUIRE(getHash<CurrentPolicy>(smartPtr) == getHash<CurrentPolicy>(smartPtr.get()));
		}

		SECTION("Shared pointer hash the managed pointer, not the value pointed to") {
			auto smartPtr = std::make_shared<int>(42);
			REQUIRE(getHash<CurrentPolicy>(smartPtr) == getHash<CurrentPolicy>(smartPtr.get()));
		}

		SECTION("Complicated types can be hashed (fix for the definition/declaration order bug)") {
			int i = 42;
			int *first = &i;
			int *second = &i;
			getHash<CurrentPolicy>(std::make_tuple(first, second));
		}

		SECTION("Fundamental types can be hashed") {
			int i = 42;
			REQUIRE(getHash<CurrentPolicy>(i) == getHash<CurrentPolicy>(42));
		}

		SECTION("Variant objects can be hashed") {
			int first = 42;
			char second = 'c';
			std::string third = "42";
			std::variant<int, char, std::string> test;
			test = first;
			REQUIRE(getHash<CurrentPolicy>(test) == getHash<CurrentPolicy>(std::make_tuple(std::size_t{0}, first)));
			test = second;
			REQUIRE(getHash<CurrentPolicy>(test) == getHash<CurrentPolicy>(std::make_tuple(std::size_t{1}, second)));
			test = third;
			REQUIRE(getHash<CurrentPolicy>(test) == getHash<CurrentPolicy>(std::make_tuple(std::size_t{2}, third)));
		}

		SECTION("Variant and the value it holds don't generate the same hash") {
			std::variant<int, char, std::string> test{42};
			REQUIRE(getHash<CurrentPolicy>(test) != getHash<CurrentPolicy>(42));
		}

		SECTION("Alternatives of the same type with equal content don't generate the same hash") {
			std::variant<long, long> first{std::in_place_index<0>, 1};
			std::variant<long, long> second{std::in_place_index<1>, 1};
			REQUIRE(getHash<CurrentPolicy>(first) != getHash<CurrentPolicy>(second));
		}

		SECTION("Alternatives of different types with equal content don't generate the same hash") {
			std::variant<UserDefinedStruct, OtherUserDefinedStruct> first{UserDefinedStruct{1}};
			std::variant<UserDefinedStruct, OtherUserDefinedStruct> second{OtherUserDefinedStruct{1}};
			REQUIRE(getHash<CurrentPolicy>(UserDefinedStruct{1}) == getHash<CurrentPolicy>(OtherUserDefinedStruct{1}));
			REQUIRE(getHash<CurrentPolicy>(first) != getHash<CurrentPolicy>(second));
		}

		SECTION("An unoccupied optional and a monostate in a variant don't generate the same hash") {
			std::variant<std::optional<int>, std::monostate> first{std::in_place_index<0>, std::nullopt};
			std::variant<std::optional<int>, std::monostate> second{std::in_place_index<1>};
			REQUIRE(getHash<CurrentPolicy>(first) != getHash<CurrentPolicy>(second));
		}

        SECTION("is_faulty returns true if ErrorValue is tested") {
			REQUIRE(dice::hash::DiceHash<int, CurrentPolicy>::is_faulty(CurrentPolicy::ErrorValue));
		}

        SECTION("is_faulty returns false if value tested isn't ErrorValue") {
            REQUIRE(dice::hash::DiceHash<int, CurrentPolicy>::is_faulty(CurrentPolicy::ErrorValue+1) == false);
        }

		SECTION("Variant monostate is not an error") {
			std::variant<std::monostate, int, char> test;
			auto hashed = getHash<CurrentPolicy>(test);
            REQUIRE_FALSE(dice::hash::DiceHash<decltype(test), CurrentPolicy>::is_faulty(hashed));
		}

		SECTION("Values which hold nothing are not errors") {
			REQUIRE_FALSE(dice::hash::DiceHash<std::monostate, CurrentPolicy>::is_faulty(getHash<CurrentPolicy>(std::monostate{})));
			REQUIRE_FALSE(dice::hash::DiceHash<std::nullopt_t, CurrentPolicy>::is_faulty(getHash<CurrentPolicy>(std::nullopt)));
			REQUIRE_FALSE(dice::hash::DiceHash<std::optional<int>, CurrentPolicy>::is_faulty(getHash<CurrentPolicy>(std::optional<int>{})));
		}

		SECTION("Empty containers are not errors") {
			REQUIRE_FALSE(dice::hash::DiceHash<std::tuple<>, CurrentPolicy>::is_faulty(getHash<CurrentPolicy>(std::tuple<>{})));
			REQUIRE_FALSE(dice::hash::DiceHash<std::string, CurrentPolicy>::is_faulty(getHash<CurrentPolicy>(std::string{})));
			REQUIRE_FALSE(dice::hash::DiceHash<std::vector<int>, CurrentPolicy>::is_faulty(getHash<CurrentPolicy>(std::vector<int>{})));
			REQUIRE_FALSE(dice::hash::DiceHash<std::set<int>, CurrentPolicy>::is_faulty(getHash<CurrentPolicy>(std::set<int>{})));
			REQUIRE_FALSE(dice::hash::DiceHash<std::map<int, int>, CurrentPolicy>::is_faulty(getHash<CurrentPolicy>(std::map<int, int>{})));
		}

		SECTION("Hash of ill-formed variant is the error value") {
			std::variant<int, ValuelessByException> test;
			try {
				test = ValuelessByException();
			} catch (std::domain_error const &) {}
			// now test is valueless_by_exception
            auto hashed = getHash<CurrentPolicy>(test);
            REQUIRE(dice::hash::DiceHash<decltype(test), CurrentPolicy>::is_faulty(hashed));
		}

		SECTION("user-defined types can be used in collections") {
			std::set<UserDefinedStruct> mySet;
			mySet.insert(UserDefinedStruct(3));
			mySet.insert(UserDefinedStruct(4));
			mySet.insert(UserDefinedStruct(7));
			getHash<CurrentPolicy>(mySet);
		}

		SECTION("dice_hash_invertible_combine can be called with any number of size_t") {
			std::size_t a = 3;
			std::size_t b = 4;
			std::size_t c = 7;
			std::size_t d = 42;
			dice::hash::DiceHash<CurrentPolicy>::hash_invertible_combine({a, b, c, d});
		}

		SECTION("dice_hash_invertible_combine is self inverse") {
			std::size_t a = 3;
			std::size_t b = 4;
			REQUIRE(a == dice::hash::DiceHash<CurrentPolicy>::hash_invertible_combine({a, b, a, a, b}));
		}

		SECTION("dice_hash_combine can be called with any number of size_t") {
			std::size_t a = 3;
			std::size_t b = 4;
			std::size_t c = 7;
			std::size_t d = 42;
			dice::hash::DiceHash<CurrentPolicy>::hash_combine({a, b, c, d});
		}

		SECTION("A part that hashes to 0 does not set the hash of a pair or tuple to 0") {
			using Set = std::unordered_set<std::uint64_t>;
			// the hash of an empty unordered container is the xor of no hashes
			REQUIRE(getHash<CurrentPolicy>(Set{}) == 0);

			std::size_t const pair1 = getHash<CurrentPolicy>(std::pair<Set, std::uint64_t>{Set{}, 1});
			std::size_t const pair2 = getHash<CurrentPolicy>(std::pair<Set, std::uint64_t>{Set{}, 2});
			CHECK(pair1 != 0);
			CHECK(pair1 != pair2);

			std::size_t const tuple1 = getHash<CurrentPolicy>(std::tuple<std::uint64_t, Set>{1, Set{}});
			std::size_t const tuple2 = getHash<CurrentPolicy>(std::tuple<std::uint64_t, Set>{2, Set{}});
			CHECK(tuple1 != 0);
			CHECK(tuple1 != tuple2);
		}

		SECTION("A part that hashes to 0 does not set the hash of a vector to 0") {
			using Sets = std::vector<std::unordered_set<std::uint64_t>>;
			std::size_t const vector1 = getHash<CurrentPolicy>(Sets{{1}, {}});
			std::size_t const vector2 = getHash<CurrentPolicy>(Sets{{2}, {}});
			CHECK(vector1 != 0);
			CHECK(vector1 != vector2);
		}
	}

	TEST_CASE("rapidhash runs in its protected mode", "[DiceHash]") {
		using Policy = dice::hash::Policies::rapidhash;
		// In the protected mode `rapid_mix` xors the 128-bit product into its operands, so mixing
		// in 0 keeps the other operand. In the fast mode the product replaces the operands, and
		// the product with 0 is 0.
		CHECK(dice::hash::rapidhash::rapid_mix(Policy::kSeed, 0) == Policy::kSeed);
		CHECK(dice::hash::rapidhash::rapid_mix(0, 42) == 42);
	}

	/** The number of bytes that hold the value of a `long double`.
	 * It comes from the macros of gcc and clang, not from the code under test: 10 for x87 extended
	 * precision, else every byte.
	 */
#if defined(__LDBL_MANT_DIG__) && __LDBL_MANT_DIG__ == 64
	inline constexpr std::size_t long_double_value_size = 10;
#else
	inline constexpr std::size_t long_double_value_size = sizeof(long double);
#endif

	/** The hash of a floating point value, computed from the primitives of the policy: the hash of
	 * the value bytes (`hash_int` of them for Martinus if they fill a `size_t`). For `float`, `double`
	 * and a `long double` without padding, this is how the policies hashed floating point values
	 * before they hashed only the value bytes.
	 */
	template<typename Policy, typename T>
	std::size_t value_bytes_hash(T const &x) {
		constexpr std::size_t value_size = std::is_same_v<T, long double> ? long_double_value_size : sizeof(T);
		if constexpr (std::is_same_v<Policy, dice::hash::Policies::Martinus> && value_size == sizeof(std::size_t)) {
			std::size_t word;
			std::memcpy(&word, &x, sizeof(word));
			return dice::hash::martinus::hash_int(word);
		} else {
			return Policy::hash_bytes(&x, value_size);
		}
	}

	/** Checks for some values of `T` that they hash to `value_bytes_hash`.
	 */
	template<typename Policy, typename T>
	void check_value_bytes_hash() {
		using limits = std::numeric_limits<T>;
		static_assert(limits::is_specialized);
		T const values[] = {T(0), T(1), T(-1.5), T(3.14159), limits::max(), limits::lowest(), limits::min(),
							limits::denorm_min(), -limits::denorm_min(), limits::infinity(), -limits::infinity(),
							limits::quiet_NaN()};
		for (T const &x : values) {
			CAPTURE(static_cast<double>(x));
			REQUIRE(getHash<Policy>(x) == value_bytes_hash<Policy>(x));
		}
	}

	/** Writes `value` to `target` and sets every byte after the value bytes to `padding`.
	 * The bytes are written with `std::memcpy`, because a copy of a `long double` by value need not
	 * keep the padding.
	 */
	void write_with_padding(long double *target, long double value, unsigned char padding) {
		unsigned char bytes[sizeof(long double)];
		std::memset(bytes, padding, sizeof(bytes));
		std::memcpy(bytes, &value, long_double_value_size);
		std::memcpy(target, bytes, sizeof(bytes));
	}

	TEST_CASE("The format of floating point types is detected", "[DiceHash]") {
		using dice::hash::internal::FloatFormat;
		using dice::hash::internal::float_format;
		using dice::hash::internal::float_format_of;
		using dice::hash::internal::float_value_size_of;

		// arguments: radix, digits, max_exponent, sizeof, little endian
		// IEEE binary32 and binary64
		STATIC_REQUIRE(float_format_of(2, 24, 128, 4, true) == FloatFormat::all_bytes);
		STATIC_REQUIRE(float_format_of(2, 53, 1024, 8, true) == FloatFormat::all_bytes);
		// IEEE binary128: long double on aarch64 Linux, RISC-V and s390x
		STATIC_REQUIRE(float_format_of(2, 113, 16384, 16, true) == FloatFormat::all_bytes);
		STATIC_REQUIRE(float_format_of(2, 113, 16384, 16, false) == FloatFormat::all_bytes);
		// x87 extended precision: long double on x86_64 (16 bytes) and x86 (12 bytes)
		STATIC_REQUIRE(float_format_of(2, 64, 16384, 16, true) == FloatFormat::x87_extended);
		STATIC_REQUIRE(float_format_of(2, 64, 16384, 12, true) == FloatFormat::x87_extended);
		// m68k extended precision: the limits of x87, but big endian with padding in the middle
		STATIC_REQUIRE(float_format_of(2, 64, 16384, 12, false) == FloatFormat::all_bytes);
		// double-double: long double on PowerPC
		STATIC_REQUIRE(float_format_of(2, 106, 1024, 16, true) == FloatFormat::double_double);
		STATIC_REQUIRE(float_format_of(2, 106, 1024, 16, false) == FloatFormat::double_double);

		STATIC_REQUIRE(float_value_size_of(FloatFormat::x87_extended, 16) == 10);
		STATIC_REQUIRE(float_value_size_of(FloatFormat::x87_extended, 12) == 10);
		STATIC_REQUIRE(float_value_size_of(FloatFormat::double_double, 16) == 16);
		STATIC_REQUIRE(float_value_size_of(FloatFormat::all_bytes, 16) == 16);
		STATIC_REQUIRE(float_value_size_of(FloatFormat::all_bytes, 8) == 8);

		STATIC_REQUIRE(float_format<float> == FloatFormat::all_bytes);
		STATIC_REQUIRE(float_format<double> == FloatFormat::all_bytes);
#if defined(__LDBL_MANT_DIG__) && defined(__LDBL_MAX_EXP__)
		// the macros of gcc and clang name the format of `long double` independent of `std::numeric_limits`
		constexpr FloatFormat expected = __LDBL_MANT_DIG__ == 64 && __LDBL_MAX_EXP__ == 16384 ? FloatFormat::x87_extended
										 : __LDBL_MANT_DIG__ == 106							  ? FloatFormat::double_double
																							  : FloatFormat::all_bytes;
		STATIC_REQUIRE(float_format<long double> == expected);
#endif
	}

	TEST_CASE("The low part of a double-double value is +0.0 in canonical form", "[DiceHash]") {
		using dice::hash::internal::canonical_float_bytes;
		using dice::hash::internal::FloatFormat;
		using Bytes = std::array<unsigned char, 16>;

		// -1.0 as (-1.0, -0.0), the result of negating 1.0, and as (-1.0, +0.0)
		constexpr Bytes le_negated{0, 0, 0, 0, 0, 0, 0xf0, 0xbf, 0, 0, 0, 0, 0, 0, 0, 0x80};
		constexpr Bytes le_converted{0, 0, 0, 0, 0, 0, 0xf0, 0xbf, 0, 0, 0, 0, 0, 0, 0, 0};
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::double_double>(le_negated, true) == le_converted);
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::double_double>(le_converted, true) == le_converted);

		constexpr Bytes be_negated{0xbf, 0xf0, 0, 0, 0, 0, 0, 0, 0x80, 0, 0, 0, 0, 0, 0, 0};
		constexpr Bytes be_converted{0xbf, 0xf0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::double_double>(be_negated, false) == be_converted);
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::double_double>(be_converted, false) == be_converted);

		// a nonzero low part does not change: 1.0 + 2^-60, and its negation
		constexpr Bytes le_low{0, 0, 0, 0, 0, 0, 0xf0, 0x3f, 0, 0, 0, 0, 0, 0, 0x30, 0x3c};
		constexpr Bytes le_negative_low{0, 0, 0, 0, 0, 0, 0xf0, 0xbf, 0, 0, 0, 0, 0, 0, 0x30, 0xbc};
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::double_double>(le_low, true) == le_low);
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::double_double>(le_negative_low, true) == le_negative_low);
	}

	TEMPLATE_TEST_CASE("Floating point values hash their value bytes", "[DiceHash]", AllPoliciesToTestForDiceHash) {
		using CurrentPolicy = TestType;

		SECTION("float and double hash as the policy hashes their bytes") {
			check_value_bytes_hash<CurrentPolicy, float>();
			check_value_bytes_hash<CurrentPolicy, double>();
		}

		SECTION("long double hashes only its value bytes") {
			check_value_bytes_hash<CurrentPolicy, long double>();
		}

#ifdef __SIZEOF_FLOAT128__
		SECTION("__float128 hashes its 16 bytes where it is a floating point type") {
			// libstdc++ counts `__float128` as a floating point type in the GNU modes (`-std=gnu++20`).
			// `tests_dice_hash_gnu` is built in such a mode.
#if defined(DICE_HASH_TEST_GNU_MODE) && defined(__GLIBCXX__) && defined(_GLIBCXX_USE_FLOAT128)
			STATIC_REQUIRE(std::is_floating_point_v<__float128>);
#endif
			if constexpr (std::is_floating_point_v<__float128>) {
				STATIC_REQUIRE(dice::hash::internal::float_value_size<__float128> == 16);
				check_value_bytes_hash<CurrentPolicy, __float128>();
			} else {
				SUCCEED("__float128 is not a floating point type in this mode");
			}
		}
#endif

		SECTION("Ranges of long double with the same values and different padding hash the same") {
			if constexpr (long_double_value_size < sizeof(long double)) {
				for (long double const value : {1.5L, -2.25L, 0.0L}) {
					CAPTURE(static_cast<double>(value));
					// Only ranges are checked: `hash_fundamental` takes a single value by value, and
					// the copy need not keep the padding of the original object.
					std::array<long double, 2> arr_x;
					std::array<long double, 2> arr_y;
					std::vector<long double> vec_x(2);
					std::vector<long double> vec_y(2);
					for (std::size_t i = 0; i < 2; ++i) {
						write_with_padding(&arr_x[i], value, 0x00);
						write_with_padding(&arr_y[i], value, 0xff);
						write_with_padding(&vec_x[i], value, 0x5a);
						write_with_padding(&vec_y[i], value, 0xa5);
					}
					REQUIRE(getHash<CurrentPolicy>(arr_x) == getHash<CurrentPolicy>(arr_y));
					REQUIRE(getHash<CurrentPolicy>(vec_x) == getHash<CurrentPolicy>(vec_y));
					REQUIRE(getHash<CurrentPolicy>(std::span<long double const>{arr_x}) == getHash<CurrentPolicy>(std::span<long double const>{vec_y}));
				}
			} else {
				WARN("long double has no padding bytes on this platform");
			}
		}

		SECTION("Vectors and arrays of long double generate the same hash") {
			REQUIRE(test_vec_arr<CurrentPolicy>(1.0L, 2.0L, 3.0L));
		}

		SECTION("Ranges of float and double hash as one block of bytes") {
			std::vector<float> const floats{1.0f, -2.5f, 3.25f};
			std::vector<double> const doubles{1.0, -2.5, 3.25};
			REQUIRE(getHash<CurrentPolicy>(floats) == CurrentPolicy::hash_bytes(floats.data(), sizeof(float) * floats.size()));
			REQUIRE(getHash<CurrentPolicy>(doubles) == CurrentPolicy::hash_bytes(doubles.data(), sizeof(double) * doubles.size()));
		}
	}

	TEST_CASE("The canonical form of -0.0 is +0.0", "[DiceHash]") {
		using dice::hash::internal::canonical_float_bytes;
		using dice::hash::internal::FloatFormat;
		using Bytes4 = std::array<unsigned char, 4>;
		using Bytes8 = std::array<unsigned char, 8>;
		using Bytes10 = std::array<unsigned char, 10>;
		using Bytes16 = std::array<unsigned char, 16>;

		// IEEE binary32, binary64 and binary128, little and big endian
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::all_bytes>(Bytes4{0, 0, 0, 0x80}, true) == Bytes4{});
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::all_bytes>(Bytes4{0x80, 0, 0, 0}, false) == Bytes4{});
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::all_bytes>(Bytes8{0, 0, 0, 0, 0, 0, 0, 0x80}, true) == Bytes8{});
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::all_bytes>(Bytes8{0x80, 0, 0, 0, 0, 0, 0, 0}, false) == Bytes8{});
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::all_bytes>(Bytes16{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80}, true) == Bytes16{});
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::all_bytes>(Bytes16{0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, false) == Bytes16{});
		// x87 extended precision, the 10 value bytes
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::x87_extended>(Bytes10{0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80}, true) == Bytes10{});
		// double-double: (-0.0, -0.0) and (-0.0, +0.0)
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::double_double>(Bytes16{0, 0, 0, 0, 0, 0, 0, 0x80, 0, 0, 0, 0, 0, 0, 0, 0x80}, true) == Bytes16{});
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::double_double>(Bytes16{0, 0, 0, 0, 0, 0, 0, 0x80, 0, 0, 0, 0, 0, 0, 0, 0}, true) == Bytes16{});
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::double_double>(Bytes16{0x80, 0, 0, 0, 0, 0, 0, 0, 0x80, 0, 0, 0, 0, 0, 0, 0}, false) == Bytes16{});

		// values that are not zero do not change: the negative subnormal closest to zero, -NaN, -1.0
		constexpr Bytes8 le_negative_subnormal{1, 0, 0, 0, 0, 0, 0, 0x80};
		constexpr Bytes8 be_negative_subnormal{0x80, 0, 0, 0, 0, 0, 0, 1};
		constexpr Bytes8 le_negative_nan{0, 0, 0, 0, 0, 0, 0xf8, 0xff};
		constexpr Bytes10 x87_negative_subnormal{1, 0, 0, 0, 0, 0, 0, 0, 0, 0x80};
		constexpr Bytes10 x87_minus_one{0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0xbf};
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::all_bytes>(le_negative_subnormal, true) == le_negative_subnormal);
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::all_bytes>(be_negative_subnormal, false) == be_negative_subnormal);
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::all_bytes>(le_negative_nan, true) == le_negative_nan);
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::x87_extended>(x87_negative_subnormal, true) == x87_negative_subnormal);
		STATIC_REQUIRE(canonical_float_bytes<FloatFormat::x87_extended>(x87_minus_one, true) == x87_minus_one);
	}

	TEMPLATE_TEST_CASE("+0.0 and -0.0 hash the same", "[DiceHash]", AllPoliciesToTestForDiceHash) {
		using CurrentPolicy = TestType;

		SECTION("float, double and long double") {
			REQUIRE(getHash<CurrentPolicy>(-0.0f) == getHash<CurrentPolicy>(0.0f));
			REQUIRE(getHash<CurrentPolicy>(-0.0) == getHash<CurrentPolicy>(0.0));
			REQUIRE(getHash<CurrentPolicy>(-0.0L) == getHash<CurrentPolicy>(0.0L));
		}

		SECTION("-0.0 hashes to the hash value of +0.0") {
			REQUIRE(getHash<CurrentPolicy>(-0.0f) == value_bytes_hash<CurrentPolicy>(0.0f));
			REQUIRE(getHash<CurrentPolicy>(-0.0) == value_bytes_hash<CurrentPolicy>(0.0));
			REQUIRE(getHash<CurrentPolicy>(-0.0L) == value_bytes_hash<CurrentPolicy>(0.0L));
		}

		SECTION("Types that hash their parts one by one") {
			REQUIRE(getHash<CurrentPolicy>(std::make_tuple(1, -0.0)) == getHash<CurrentPolicy>(std::make_tuple(1, 0.0)));
			REQUIRE(getHash<CurrentPolicy>(std::optional<float>{-0.0f}) == getHash<CurrentPolicy>(std::optional<float>{0.0f}));
			REQUIRE(getHash<CurrentPolicy>(std::set<double>{-0.0, 1.0}) == getHash<CurrentPolicy>(std::set<double>{0.0, 1.0}));
		}

		SECTION("Ranges of long double that are hashed value by value") {
			if constexpr (!dice::hash::internal::hash_range_as_bytes<long double>) {
				REQUIRE(getHash<CurrentPolicy>(std::vector<long double>{-0.0L, 1.0L}) == getHash<CurrentPolicy>(std::vector<long double>{0.0L, 1.0L}));
			} else {
				WARN("ranges of long double are hashed as bytes on this platform");
			}
		}

#ifdef __SIZEOF_FLOAT128__
		SECTION("__float128 where it is a floating point type") {
			if constexpr (std::is_floating_point_v<__float128>) {
				__float128 const zero = 0;
				REQUIRE(getHash<CurrentPolicy>(-zero) == getHash<CurrentPolicy>(zero));
			} else {
				SUCCEED("__float128 is not a floating point type in this mode");
			}
		}
#endif
	}
}// namespace dice::tests::hash

/*
* Define hash for test structures.
*/
namespace dice::hash {
	using dice::tests::hash::OtherUserDefinedStruct;
	using dice::tests::hash::UserDefinedStruct;
	using dice::tests::hash::ValuelessByException;

	template<typename Policy>
	struct dice_hash_overload<Policy, UserDefinedStruct> {
		static std::size_t dice_hash(UserDefinedStruct const &s) noexcept {
			return dice_hash_templates<Policy>::dice_hash(s.a);
		}
	};
	template<typename Policy>
	struct dice_hash_overload<Policy, OtherUserDefinedStruct> {
		static std::size_t dice_hash(OtherUserDefinedStruct const &s) noexcept {
			return dice_hash_templates<Policy>::dice_hash(s.a);
		}
	};
	template<typename Policy>
	struct dice_hash_overload<Policy, ValuelessByException> {
		static std::size_t dice_hash(ValuelessByException const &) noexcept {
			return 0;
		}
	};
}// namespace dice::hash