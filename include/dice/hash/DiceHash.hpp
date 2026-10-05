#ifndef DICE_HASH_DICEHASH_HPP
#define DICE_HASH_DICEHASH_HPP

/** @file
 * @brief Home of the DiceHash implementation.
 *
 * To speed up tests of the Hypertrie and Tentris we needed to be able to serialize a Hypertrie and save it.
 * However the last hash function was not "stable", i.e. it chose two different random seeds, so the results differed.
 * Because of that (and to not worry about versioning problems) this hash function was created.
 */

#include "dice/hash/internal/Container_trait.hpp"
#include "dice/hash/internal/DiceHashPolicies.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
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

/** Home of the DiceHash.
 *
 */
namespace dice::hash {

	/** Helper struct for defining the hash for custom structs.
	 * Because of partial specialization problems with functions, this struct must be specialized to define the hash for a custom type.
	 *
	 * Every specialization must be declared before `DiceHash<T, Policy>`, or a `DiceHash` of a type that contains
	 * `T`, is first instantiated as a class, also a specialization without `is_avalanching`. The class reads the
	 * member type `is_avalanching` of the specialization.
	 *
	 * A specialization can declare the member type `is_avalanching`. It promises that the result of its `dice_hash`
	 * is avalanching with `Policy`, and `DiceHash<T, Policy>` then declares `is_avalanching` too. dice-hash does not
	 * check the promise. A member type with a `value` that is false, like `std::false_type`, makes no promise.
	 * `using is_avalanching = avalanching_like<U, Policy>;` promises it exactly for the policies for which
	 * `DiceHash<U, Policy>` declares `is_avalanching`. Use it if `dice_hash` returns
	 * `dice_hash_templates<Policy>::dice_hash(u)` for a `u` of the type `U`. `using is_avalanching = void;`
	 * promises it for every policy, also for a policy of your own. A specialization that derives from a class with
	 * `is_avalanching`, for example another `dice_hash_overload`, gets the promise of that class.
	 * @tparam Policy The policy to use.
	 * @tparam T The custom type.
	 */
	template<Policies::HashPolicy Policy, typename T>
	struct dice_hash_overload {
		/** Helper type.
         * It is used for a static_assert.
         * If a specific version of a template function needs to be disabled via static_assert, there can be a problem.
         * If the evaluation does not need the type T of the template, it will always be evaluated, even if the specific function
         * is not used/instantiated. This is a workaround. It will contain false for every type, however that is not directly knowable.
         */
		template<typename>
		struct AlwaysFalse : std::false_type {};

		/** Default implementation of the dice_hash function.
         * It will simply not compile. For every type there should be an specific overload.
         * @tparam T Type of the value to hash.
         * @return Nothing. It WILL NOT compile.
         */
		static std::size_t dice_hash(T const &) noexcept {
			static_assert(AlwaysFalse<T>::value,
						  "The hash function is not defined for this type. You need to add an implementation yourself");
			return 0;
		}
	};

	/** Type traits and constants used by dice_hash_templates.
	 * The traits sit outside of the class because gcc 15 crashes with an internal compiler
	 * error when the requires clause of a member function template uses a variable template
	 * of the same class and that function is called with a non-dependent argument.
	 */
	namespace internal {
#ifdef __SIZEOF_INT128__
		template<typename T>
		inline constexpr bool is_int128 = std::is_same_v<std::remove_cv_t<T>, __int128> || std::is_same_v<std::remove_cv_t<T>, unsigned __int128>;
#else
		template<typename T>
		inline constexpr bool is_int128 = false;
#endif // __SIZEOF_INT128__

		/** Types which are hashed by their value, not by their content.
		 * `long double` is not one of them, so dice-hash has no hash for it. With gcc and clang on x86
		 * and x86_64 it has padding bytes with undefined content, so two equal values could get
		 * different hashes.
		 * @tparam T The type to check.
		 */
		template<typename T>
		inline constexpr bool is_fundamental = (std::is_fundamental_v<T> && !std::is_same_v<std::remove_cv_t<T>, long double>)
											   || std::is_same_v<std::remove_cv_t<T>, std::byte> || is_int128<T>;

		/** Hashes of the types which hold no value.
		 * A type which holds no value has nothing to hash, so it gets a fixed constant. The two
		 * constants only have to be different from each other and from the error value of the
		 * policy, which dice_hash_templates checks. They are the same for every policy.
		 */
		inline constexpr std::size_t monostate_hash = static_cast<std::size_t>(0x734085ee0280dfa9ull);
		inline constexpr std::size_t nullopt_hash = static_cast<std::size_t>(0x5003c53fd5e09c85ull);
	}// namespace internal

	/** Class which contains all dice_hash functions.
	 * @tparam Policy The Policy the hash is based on.
	 */
	template<Policies::HashPolicy Policy>
	class dice_hash_templates {
		static_assert(internal::monostate_hash != Policy::ErrorValue,
					  "the error value of the policy collides with the hash of std::monostate");
		static_assert(internal::nullopt_hash != Policy::ErrorValue,
					  "the error value of the policy collides with the hash of std::nullopt");

	private:
		/** Calculates the hash over an ordered container.
         * An example would be a vector, a map, an array or a list.
         * Needs a ForwardIterator in the Container-type, and an member type "value_type".
         *
         * @tparam Container The container type (vector, map, list, etc).
         * @param container The container to calculate the hash value of.
         * @return The combined hash of all values inside of the container.
         */
		template<typename Container>
		static std::size_t dice_hash_ordered_container(Container const &container) noexcept {
			typename Policy::HashState hash_state(container.size());
			std::size_t item_hash;
			for (const auto &item : container) {
				item_hash = dice_hash(item);
				hash_state.add(item_hash);
			}
			return hash_state.digest();
		}

		/** Calculates the hash over an unordered container.
         * An example would be a unordered_map or an unordered_set.
         * It uses the dice_hash_invertible_combine because a specific layout of data cannot be assumed.
         * Needs a ForwardIterator in the Container-type, and an member type "value_type".
         *
         * @tparam Container The container type (unordered_map/set etc).
         * @param container The container to calculate the hash value of.
         * @return The combined hash of all Values inside of the container.
         */
		template<typename Container>
		static std::size_t dice_hash_unordered_container(Container const &container) noexcept {
			std::size_t h{};
			for (auto const &it : container) {
				h = Policy::hash_invertible_combine({h, dice_hash(it)});
			}
			return h;
		}

		/** Helper function for hashing tuples.
         * It is a wrapper for hash_and_combine.
         * This function can be called with the help of std::make_index_sequence.
         * @tparam TupleArgs The types used in the tuple.
         * @tparam ids Generated by std::make_index_sequence. Needed for indexing the tuple values.
         * @param tuple The tuple to hash.
         * @return Hash value.
         */
		template<typename... TupleArgs, std::size_t... ids>
		static std::size_t dice_hash_tuple(std::tuple<TupleArgs...> const &tuple, std::index_sequence<ids...> const &) {
			return Policy::hash_combine({dice_hash(std::get<ids>(tuple))...});
		}

	public:
		/** Base case for dice_hash.
         * This case is only chosen if no other match is found in this struct.
         * Than it tries to find a specialization of dice::hash::dice_hash_overload and
         * if none is found, this function will not compile.
         * @tparam T The type to hash.
         * @return Hash value.
         */
		template<typename T>
		static std::size_t dice_hash(T const &t) noexcept {
			return dice_hash_overload<Policy, T>::dice_hash(t);
		}

		/** Implementation for fundamentals.
         * @tparam T Fundamental type.
         * @param fundamental Value to hash.
         * @return Hash value.
         */
		template<typename T> requires internal::is_fundamental<std::decay_t<T>>
		static std::size_t dice_hash(T const &fundamental) noexcept {
			return Policy::hash_fundamental(fundamental);
		}

		/** Implementation for string types.
         * @tparam CharT A char type. See the definition of std::string for more information.
         * @param str The string to hash.
         * @return Hash value.
         */
		template<typename CharT>
		static std::size_t dice_hash(std::basic_string<CharT> const &str) noexcept {
			return Policy::hash_bytes(str.data(), sizeof(CharT) * str.size());
		}

		/** Implementation for string view.
         * @tparam CharT A char type. See the definition of std::string for more information.
         * @param sv The string view to hash.
         * @return Hash value.
         */
		template<typename CharT>
		static std::size_t dice_hash(std::basic_string_view<CharT> const &sv) noexcept {
			return Policy::hash_bytes(sv.data(), sizeof(CharT) * sv.size());
		}

		/** Implementation for raw pointers.
         * CAUTION: hashes the POINTER, not the OBJECT POINTED TO!
         * @tparam T A pointer type.
         * @param ptr The pointer to hash.
         * @return Hash value.
         */
		template<typename T>
		static std::size_t dice_hash(T *ptr) noexcept {
			return Policy::hash_fundamental(ptr);
		}

		/** Implementation for unique pointers.
         * CAUTION: hashes the POINTER, not the OBJECT POINTED TO!
         * @tparam T A unique pointer type.
         * @param ptr The pointer to hash.
         * @return Hash value.
         */
		template<typename T>
		static std::size_t dice_hash(std::unique_ptr<T> const &ptr) noexcept {
			return dice_hash(ptr.get());
		}

		/** implementation for shared pointers.
         * CAUTION: hashes the POINTER, not the OBJECT POINTED TO!
         * @tparam T A shared pointer type.
         * @param ptr The pointer to hash.
         * @return Hash value.
         */
		template<typename T>
		static std::size_t dice_hash(std::shared_ptr<T> const &ptr) noexcept {
			return dice_hash(ptr.get());
		}

		/** Implementation for std arrays.
        * It will use different implementations if the type is fundamental or not.
        * @tparam T The type of the values.
        * @tparam N The number of values.
        * @param arr The array itself.
        * @return Hash value.
        */
		template<typename T, std::size_t N>
		static std::size_t dice_hash(std::array<T, N> const &arr) noexcept {
			if constexpr (internal::is_fundamental<T>) {
				return Policy::hash_bytes(arr.data(), sizeof(T) * N);
			} else {
				return dice_hash_ordered_container(arr);
			}
		}

		/** Implementation for vectors.
         * It will use different implementations for fundamental and non-fundamental types.
         * @tparam T The type of the values.
         * @param vec The vector itself.
         * @return Hash value.
         */
		template<typename T>
		static std::size_t dice_hash(std::vector<T> const &vec) noexcept {
			if constexpr (internal::is_fundamental<T>) {
				static_assert(!std::is_same_v<std::decay_t<T>, bool>,
							  "vector of booleans has a special implementation which results in errors!");
				return Policy::hash_bytes(vec.data(), sizeof(T) * vec.size());
			} else {
				return dice_hash_ordered_container(vec);
			}
		}

		/** Implementation for byte spans
		 * @param bytes byte span to hash
		 * @return Hash value.
		 */
		template<typename T, std::size_t Extent>
		static std::size_t dice_hash(std::span<T, Extent> const &span) noexcept {
			if constexpr (internal::is_fundamental<T>) {
				return Policy::hash_bytes(span.data(), span.size_bytes());
			} else {
				return dice_hash_ordered_container(span);
			}
		}

		/** Implementation for tuples.
         * Will hash every entry and then combine the hashes.
         * @tparam TupleArgs The types of the tuple values.
         * @param tpl The tuple itself.
         * @return Hash value.
         */
		template<typename... TupleArgs>
		static std::size_t dice_hash(std::tuple<TupleArgs...> const &tpl) noexcept {
			return dice_hash_tuple(tpl, std::make_index_sequence<sizeof...(TupleArgs)>());
		}

		/** Implementation for pairs.
         * Will hash the entries and then combine them.
         * @tparam T Type of the first value.
         * @tparam V Type of the second value.
         * @param p The pair itself.
         * @return Hash value.
         */
		template<typename T, typename V>
		static std::size_t dice_hash(std::pair<T, V> const &p) noexcept {
			return Policy::hash_combine({dice_hash(p.first), dice_hash(p.second)});
		}

		/** Overload for std::monostate.
         * It is needed so its usage in std::variant is possible.
         * @return The constant which stands for a value that holds nothing.
         */
		static std::size_t dice_hash(std::monostate const &) noexcept {
			return internal::monostate_hash;
		}

		/** Overload for std::nullopt_t.
         * @return The constant which stands for a value that holds nothing.
         */
		static std::size_t dice_hash(std::nullopt_t const &) noexcept {
			return internal::nullopt_hash;
		}

		/** Implementation for optionals.
         * Hashes an index together with the contained value.
         * The index is 0 if the optional is empty and 1 if it holds a value.
         * An empty optional uses std::nullopt as its value.
         * @tparam T Type of the optional.
         * @param opt The optional itself.
         * @return Hash value.
         */
		template<typename T>
		static std::size_t dice_hash(std::optional<T> const &opt) noexcept {
			if (!opt.has_value()) {
				static constexpr std::size_t index = 0;
				return dice_hash(std::tie(index, std::nullopt));
			} else {
				static constexpr std::size_t index = 1;
				return dice_hash(std::tie(index, *opt));
			}
		}

		/** Implementation for variant.
         * Hashes the index of the active alternative together with its value.
         * The index is part of the hash, so two alternatives with equal content
         * still get different hashes.
         * @tparam VariantArgs Types of the possible values.
         * @param var The variant itself.
         * @return Hash value, the error value of the policy if the variant is valueless_by_exception.
         */
		template<typename... VariantArgs>
		static std::size_t dice_hash(std::variant<VariantArgs...> const &var) noexcept {
			if (var.valueless_by_exception()) {
				return Policy::ErrorValue;
			}
			std::size_t const index = var.index();
			return std::visit([&index](auto const &arg) { return dice_hash(std::tie(index, arg)); }, var);
		}

		/** Implementation for ordered container.
         * It uses a custom type trait to check if the type is in fact an ordered container.
         * CAUTION: If you want to add another type to the trait, you might need to do it before this is included!
         * @tparam T The container type.
         * @param container The container itself.
         * @return Hash value.
         */
		template<typename T>
		requires is_ordered_container_v<T> static std::size_t dice_hash(T const &container) noexcept {
			return dice_hash_ordered_container(container);
		}

		/** Implementation for unordered container.
         * It uses a custom type trait to check if the type is in fact an unordered container.
         * CAUTION: If you want to add another type to the trait, you might need to do it before this is included!
         * @tparam T The container type.
         * @param container The container itself.
         * @return Hash value.
         */
		template<typename T>
		requires is_unordered_container_v<T> static std::size_t dice_hash(T const &container) noexcept {
			return dice_hash_unordered_container(container);
		}
	};

	/** Traits that decide which `DiceHash` declares the member type `is_avalanching`.
	 * A result is avalanching if every bit of the input changes each bit of the result with a
	 * probability of about one half.
	 */
	namespace internal {
		/** Tells which functions of a policy give avalanching results.
		 * `fundamental<T>`: `hash_fundamental` for the type `T`.
		 * `bytes`: `hash_bytes`, for every length.
		 * `combine`: `hash_combine` and `HashState`. True if they give an avalanching result also
		 * for input hashes from the paths of `dice_hash_templates` that do not avalanche.
		 * `combine_keeps`: `hash_combine` and `HashState`. True if they give an avalanching result
		 * when all input hashes are avalanching.
		 * A policy without a specialization promises nothing, so `DiceHash` never declares
		 * `is_avalanching` for it, except for `std::monostate`, `std::nullopt_t`, `std::nullptr_t`
		 * and the types whose `dice_hash_overload` declares `is_avalanching`.
		 * @tparam Policy The policy.
		 */
		template<typename Policy>
		struct avalanching_functions {
			template<typename T>
			static constexpr bool fundamental = false;
			static constexpr bool bytes = false;
			static constexpr bool combine = false;
			static constexpr bool combine_keeps = false;
		};

		/** The number of bytes that the policies of dice-hash hash for a value of the fundamental type
		 * `T`: the value bytes of a floating point type (`float_value_size`), `sizeof(T)` for the
		 * other types.
		 */
		template<typename T>
		constexpr std::size_t fundamental_hashed_size() noexcept {
			if constexpr (std::is_floating_point_v<T>) {
				return float_value_size<T>;
			} else {
				return sizeof(T);
			}
		}

		/** `hash_fundamental` uses `hash_int` for the types with up to 8 bytes, except the floating
		 * point types, and for the floating point types with 8 value bytes (`double`). The other
		 * floating point types (`float`, `long double` where it is larger than `double`) and the
		 * types with more than 8 bytes go through `hash_bytes` over their value bytes.
		 * `hash_int` multiplies and rotates, so some input bits never change some output bits.
		 * `hash_bytes` is MurmurHash64A. When 4 to 7 bytes follow the last full block of 8 bytes,
		 * some output bits flip with a probability of 0.44 instead of 0.5. This affects `float`
		 * and every string whose length leaves such a rest.
		 * `hash_combine` and `HashState` mix each input hash and mix the result again at the end.
		 */
		template<>
		struct avalanching_functions<Policies::Martinus> {
			template<typename T>
			static constexpr bool fundamental = fundamental_hashed_size<T>() != sizeof(std::size_t)
												&& (fundamental_hashed_size<T>() > sizeof(std::size_t) || std::is_floating_point_v<T>)
												&& fundamental_hashed_size<T>() % 8 < 4;
			static constexpr bool bytes = false;
			static constexpr bool combine = true;
			static constexpr bool combine_keeps = true;
		};

		/** Every function hashes the bytes with XXH3.
		 */
		template<>
		struct avalanching_functions<Policies::xxh3> {
			template<typename T>
			static constexpr bool fundamental = true;
			static constexpr bool bytes = true;
			static constexpr bool combine = true;
			static constexpr bool combine_keeps = true;
		};

		/** `hash_fundamental` uses `wyhash64` for the integer types with up to 64 bits, and `wyhash`
		 * over the bytes for the other types: floating point values (their value bytes), pointers,
		 * `__int128` and `unsigned __int128`. Both avalanche, in every `-std` mode.
		 * `hash_combine` and `HashState` call `_wymix` once per input hash. `_wymix` with a fixed
		 * first argument does not mix every bit of the second argument. So they keep the avalanche
		 * of avalanching input hashes, but they do not create it.
		 */
		template<>
		struct avalanching_functions<Policies::wyhash> {
			template<typename T>
			static constexpr bool fundamental = true;
			static constexpr bool bytes = true;
			static constexpr bool combine = false;
			static constexpr bool combine_keeps = true;
		};

		/** `hash_fundamental` and `hash_bytes` use `rapidhash_withSeed`.
		 * `hash_combine` and `HashState` call `rapid_mix` once per input hash. rapidhash runs in its
		 * protected mode, so `rapid_mix(a, b)` is `a ^ b` xor the two halves of the 128-bit product of
		 * `a` and `b`. With a fixed first argument it does not mix every bit of the second argument.
		 * So they keep the avalanche of avalanching input hashes, but they do not create it.
		 */
		template<>
		struct avalanching_functions<Policies::rapidhash> {
			template<typename T>
			static constexpr bool fundamental = true;
			static constexpr bool bytes = true;
			static constexpr bool combine = false;
			static constexpr bool combine_keeps = true;
		};

		/** How well `dice_hash_templates<Policy>::dice_hash` mixes the result for a type.
		 */
		enum class mixing {
			/** Nothing is known. Types with a `dice_hash_overload` that does not declare a true
			 * `is_avalanching`, and results of a combine that does not avalanche over parts that do
			 * not avalanche.
			 */
			unknown,
			/** A path of `dice_hash_templates` whose result does not avalanche. */
			plain,
			/** Every input bit changes each bit of the result with a probability of about one half.
			 * Types with only one value are avalanching, because they have no input bit. Types whose
			 * `dice_hash_overload` declares `is_avalanching` are avalanching by the promise of the user.
			 */
			avalanching
		};

		/** The mixing of a type with its cv-qualifiers and references removed. The specializations
		 * below follow the overloads of `dice_hash_templates`.
		 * @tparam Policy The policy.
		 * @tparam T The type to hash, without cv-qualifiers and references.
		 */
		template<typename Policy, typename T>
		struct mixing_of;

		template<typename Policy, typename T>
		inline constexpr mixing mixing_v = mixing_of<Policy, std::remove_cvref_t<T>>::value;

		/** Mixing of `hash_fundamental`. */
		template<typename Policy, typename T>
		constexpr mixing fundamental_mixing() noexcept {
			if constexpr (std::is_null_pointer_v<T>) {
				return mixing::avalanching;
			} else {
				return avalanching_functions<Policy>::template fundamental<T> ? mixing::avalanching : mixing::plain;
			}
		}

		/** Mixing of `hash_bytes`. */
		template<typename Policy>
		constexpr mixing bytes_mixing() noexcept {
			return avalanching_functions<Policy>::bytes ? mixing::avalanching : mixing::plain;
		}

		/** Mixing of `hash_combine` or `HashState` over the hashes of values of the types `Parts`. */
		template<typename Policy, typename... Parts>
		constexpr mixing combine_mixing() noexcept {
			if constexpr (((mixing_v<Policy, Parts> == mixing::unknown) || ...)) {
				return mixing::unknown;
			} else if constexpr (avalanching_functions<Policy>::combine) {
				return mixing::avalanching;
			} else if constexpr (avalanching_functions<Policy>::combine_keeps
								 && ((mixing_v<Policy, Parts> == mixing::avalanching) && ...)) {
				return mixing::avalanching;
			} else {
				return mixing::unknown;
			}
		}

		/** Mixing of a type that `dice_hash_templates` hashes with `dice_hash_overload<Policy, T>`.
		 * The result is avalanching if the overload declares the member type `is_avalanching`,
		 * except if that type has a `value` that is false.
		 */
		template<typename Policy, typename T>
		constexpr mixing overload_mixing() noexcept {
			if constexpr (requires { typename dice_hash_overload<Policy, T>::is_avalanching; }) {
				using promise = typename dice_hash_overload<Policy, T>::is_avalanching;
				if constexpr (requires { promise::value; }) {
					return static_cast<bool>(promise::value) ? mixing::avalanching : mixing::unknown;
				} else {
					return mixing::avalanching;
				}
			} else {
				return mixing::unknown;
			}
		}

		/** Mixing of a `std::vector`, `std::array` or `std::span` that holds values of the type `T`.
		 * `dice_hash_templates` hashes it as one block with `hash_bytes` if `hash_range_as_bytes<T>`
		 * is true, otherwise value by value with `HashState` over the hashes of the values. Ranges of
		 * floating point values are hashed value by value.
		 */
		template<typename Policy, typename T>
		constexpr mixing sequence_mixing() noexcept {
			if constexpr (hash_range_as_bytes<T>) {
				return bytes_mixing<Policy>();
			} else {
				return combine_mixing<Policy, T>();
			}
		}

		/** The type of the elements that a range-based for loop over a `Container const &` gives. */
		template<typename Container>
		using element_t = std::remove_cvref_t<decltype(*std::begin(std::declval<Container const &>()))>;

		/** Fundamental types, the containers of `is_ordered_container` and `is_unordered_container`,
		 * and types with a `dice_hash_overload`.
		 * The hash of an unordered container is the xor of the hashes of its elements. This keeps
		 * relations between results, for example `h({a, b}) ^ h({a, c}) == h({b, c})`. So it is
		 * never avalanching.
		 */
		template<typename Policy, typename T>
		struct mixing_of {
			static constexpr mixing value = [] {
				if constexpr (is_fundamental<T>) {
					return fundamental_mixing<Policy, T>();
				} else if constexpr (is_ordered_container_v<T>) {
					return combine_mixing<Policy, element_t<T>>();
				} else if constexpr (is_unordered_container_v<T>) {
					return mixing_v<Policy, element_t<T>> == mixing::unknown ? mixing::unknown : mixing::plain;
				} else {
					return overload_mixing<Policy, T>();
				}
			}();
		};

		template<typename Policy, typename CharT>
		struct mixing_of<Policy, std::basic_string<CharT>> {
			static constexpr mixing value = bytes_mixing<Policy>();
		};

		template<typename Policy, typename CharT>
		struct mixing_of<Policy, std::basic_string_view<CharT>> {
			static constexpr mixing value = bytes_mixing<Policy>();
		};

		template<typename Policy, typename T>
		struct mixing_of<Policy, T *> {
			static constexpr mixing value = fundamental_mixing<Policy, T *>();
		};

		template<typename Policy, typename T>
		struct mixing_of<Policy, std::unique_ptr<T>> {
			static constexpr mixing value = mixing_v<Policy, typename std::unique_ptr<T>::pointer>;
		};

		template<typename Policy, typename T>
		struct mixing_of<Policy, std::shared_ptr<T>> {
			static constexpr mixing value = mixing_v<Policy, typename std::shared_ptr<T>::element_type *>;
		};

		template<typename Policy, typename T, std::size_t N>
		struct mixing_of<Policy, std::array<T, N>> {
			static constexpr mixing value = sequence_mixing<Policy, T>();
		};

		/** `std::vector<bool>` has no hash, its `dice_hash` does not compile. */
		template<typename Policy, typename T>
		struct mixing_of<Policy, std::vector<T>> {
			static constexpr mixing value = std::is_same_v<std::remove_cv_t<T>, bool> ? mixing::unknown : sequence_mixing<Policy, T>();
		};

		template<typename Policy, typename T, std::size_t Extent>
		struct mixing_of<Policy, std::span<T, Extent>> {
			static constexpr mixing value = sequence_mixing<Policy, T>();
		};

		template<typename Policy, typename... Ts>
		struct mixing_of<Policy, std::tuple<Ts...>> {
			static constexpr mixing value = combine_mixing<Policy, Ts...>();
		};

		template<typename Policy, typename T, typename V>
		struct mixing_of<Policy, std::pair<T, V>> {
			static constexpr mixing value = combine_mixing<Policy, T, V>();
		};

		template<typename Policy>
		struct mixing_of<Policy, std::monostate> {
			static constexpr mixing value = mixing::avalanching;
		};

		template<typename Policy>
		struct mixing_of<Policy, std::nullopt_t> {
			static constexpr mixing value = mixing::avalanching;
		};

		/** An optional combines its index with its value, or with `std::nullopt` if it is empty.
		 * `std::nullopt_t` is avalanching, so it is left out.
		 */
		template<typename Policy, typename T>
		struct mixing_of<Policy, std::optional<T>> {
			static constexpr mixing value = combine_mixing<Policy, std::size_t, T>();
		};

		/** A variant combines its index with the value of the active alternative. */
		template<typename Policy, typename... Ts>
		struct mixing_of<Policy, std::variant<Ts...>> {
			static constexpr mixing value = combine_mixing<Policy, std::size_t, Ts...>();
		};

		/** Base of `DiceHash` that declares `is_avalanching` if `avalanching` is true. */
		template<bool avalanching>
		struct avalanching_marker {};

		template<>
		struct avalanching_marker<true> {
			using is_avalanching = void;
		};
	}// namespace internal

	/** `std::true_type` if `DiceHash<T, Policy>` declares `is_avalanching`, otherwise `std::false_type`.
	 * A `dice_hash_overload<Policy, X>` whose `dice_hash` returns `dice_hash_templates<Policy>::dice_hash(t)`
	 * for a `t` of the type `T` can declare `using is_avalanching = avalanching_like<T, Policy>;`. `T` is
	 * the type that the overload hashes, not `X` or a type that contains `X`.
	 * @tparam T The type that the overload passes to `dice_hash_templates<Policy>::dice_hash`.
	 * @tparam Policy The policy.
	 */
	template<typename T, Policies::HashPolicy Policy>
	using avalanching_like = std::bool_constant<internal::mixing_v<Policy, T> == internal::mixing::avalanching>;

	/** Wrapper class for the dice::hash::dice_hash function.
     * It is a typical hash interface.
     *
     * `DiceHash` declares the member type `is_avalanching` (as `void`) exactly if its result is
     * avalanching: every bit of the input changes each bit of the result with a probability of
     * about one half. A hash table can then use the lowest bits of the result directly, for example
     * with a mask. This is the convention of ankerl::unordered_dense. On a 64-bit platform:
     * - Never avalanching: unordered containers, because their hash is the xor of the hashes of
     *   their elements. Types with a `dice_hash_overload` that does not declare a true
     *   `is_avalanching`, and every type that contains one.
     * - Always avalanching: `std::monostate`, `std::nullopt_t` and `std::nullptr_t`, because they
     *   have only one value.
     * - Avalanching by the promise of the user: types whose `dice_hash_overload` declares
     *   `is_avalanching` for the policy (see `dice_hash_overload`). The types that contain them
     *   follow the rules below.
     * - `xxh3`: every other type.
     * - `wyhash` and `rapidhash`: fundamental types, pointers, smart pointers, strings, string
     *   views, and vectors, arrays and spans of fundamental types. Pairs, tuples, optionals,
     *   variants and the other containers only if all their parts are avalanching.
     * - `Martinus`: pairs, tuples, optionals, variants, ordered containers, and vectors, arrays and
     *   spans of floating point types and of types that are not fundamental. `__int128`,
     *   `unsigned __int128`, and `long double` where it is larger than `double`. Not avalanching
     *   are the other fundamental types, pointers, smart pointers, strings, string views, and
     *   vectors, arrays and spans of the other fundamental types.
     * - A policy of your own: no other type.
     * `DiceHash<T>` without a policy uses `wyhash`. The README explains the reasons.
     * @tparam T The type to define the hash for.
     * @tparam Policy The Policy defines how the hash works on a basic level. The default is `Policies::wyhash`.
     */
	template<typename T, Policies::HashPolicy Policy = Policies::wyhash>
	struct DiceHash : private Policy,
					  public internal::avalanching_marker<internal::mixing_v<Policy, T> == internal::mixing::avalanching> {
		/** Policy function for combining already hashed values.
		 * This using declaration is equal to a handwritten wrapper function.
		 *@param list Initializer list of std::size_t hashes.
		 * @return Single hash value.
		 */
        using Policy::hash_combine;

		/** Policy function for combining already hashed values in a invertible fashion.
		 * This using declaration is equal to a handwritten wrapper function.
		 *@param list Initializer list of std::size_t hashes.
		 * @return Single hash value.
		 */
        using Policy::hash_invertible_combine;

        /** Overloaded operator to calculate a hash.
         * Simply calls the dice_hash function for the specified type.
         * @param t The value to calculate the hash of.
         * @return Hash value.
         */
        std::size_t operator()(T const &t) const noexcept {
			return dice_hash_templates<Policy>::dice_hash(t);
		}

		/** Function to check if a hash is equal to an error value.
		 * Simple wrapper for equality checking of the Policy error value.
		 * @param to_check The hash value to check.
		 * @return True if value is an error value, false otherwise.
		 */
		[[nodiscard]] static constexpr bool is_faulty(std::size_t to_check) noexcept {
			return to_check == Policy::ErrorValue;
		}
	};

    template <typename T>
    using DiceHashMartinus = DiceHash<T, Policies::Martinus>;
    template <typename T>
    using DiceHashxxh3 = DiceHash<T, Policies::xxh3>;
    template <typename T>
    using DiceHashwyhash = DiceHash<T, Policies::wyhash>;
	template <typename T>
	using DiceHashrapidhash = DiceHash<T, Policies::rapidhash>;
}// namespace dice::hash
#endif//DICE_HASH_DICEHASH_HPP