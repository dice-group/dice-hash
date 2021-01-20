#ifndef HYPERTRIE_DICEHASH_HPP
#define HYPERTRIE_DICEHASH_HPP

/** @file
 * @brief Home of the DiceHash implementation.
 *
 * To speed up tests of the Hypertrie and Tentris we needed to be able to serialize a Hypertrie and save it.
 * However the last hash function was not "stable", i.e. it chose two different random seeds, so the results differed.
 * Because of that (and to not worry about versioning problems) this hash function was created.
 */

#include "Dice/hash/DiceHashDefinitions.hpp"
#include "Dice/hash/martinus_robinhood_hash.hpp"

namespace Dice::hash {

	/** Home of the implementation specific things.
	 */
	namespace detail {
		using Dice::hash::martinus::hash_bytes;
		using Dice::hash::martinus::hash_combine;
		using Dice::hash::martinus::HashState;

		/** Wrapper for fundamental types.
		 * Chooses the correct basic hash function for a given type.
		 * @tparam T Fundamental type or pointer (using constrains).
		 * @param x The value to hash.
		 * @return Hash value.
		 */
		template<typename T>
		requires std::is_fundamental_v<std::decay_t<T>> or std::is_pointer_v<std::decay_t<T>>
		inline std::size_t hash_primitive(T x) noexcept {
			if constexpr (sizeof(std::decay_t<T>) == sizeof(size_t)) {
				return Dice::hash::martinus::hash_int(*reinterpret_cast<size_t const *>(&x));
			} else if constexpr (sizeof(std::decay_t<T>) > sizeof(size_t) or std::is_floating_point_v<std::decay_t<T>>) {
				return hash_bytes(&x, sizeof(x));
			} else {
				return Dice::hash::martinus::hash_int(static_cast<size_t>(x));
			}
		}

		/** Combines two hashes to a new hash.
         * This function is commutative and invertible.
         * It is used in the unordered container functions. However this __will__ be replaced in the future.
         * @param a First hash.
         * @param b Second hash.
         * @return Combination of a and b.
         */
		inline std::size_t dice_hash_invertible_combine(std::size_t a, std::size_t b) {
			return a xor b;
		}

		/** Combine n hashes to a new hash.
		 * Uses the base definition of Dice::hash::detail::dice_hash_invertible_combine for two elements.
		 * So all properties are from that definition.
		 * @tparam Args Needed so any number of arguments can be used.
		 * @param a First hash.
		 * @param args All other hashes.
		 * @return Combination of all hashes.
		 */
		template <typename ...Args>
		inline std::size_t dice_hash_invertible_combine(std::size_t a, Args ...args) {
			return dice_hash_invertible_combine(a, dice_hash_invertible_combine(args...));
		}

		/** Combine n hashes to a new hash.
		 * Uses the Dice::hash::martinus::hash_combine for all elements at once.
		 * So this is simply a wrapper for it.
		 * @tparam Args std::size_t.
		 * @param args Hashes. All std::size_t from type.
		 * @return Combination of all hashes.
		 */
		template <typename ...Args>
		inline std::size_t dice_hash_combine(Args ...args) {
			return hash_combine({args...});
		}


		/** Calculates the hash over an ordered container.
         * An example would be a vector, a map, an array or a list.
         * Needs a ForwardIterator in the Container-type, and an member type "value_type".
         *
         * @tparam Container The container type (vector, map, list, etc).
         * @param container The container to calculate the hash value of.
         * @return The combined hash of all values inside of the container.
         */
		template<typename Container>
		inline std::size_t dice_hash_ordered_container(Container const &container) noexcept {
			detail::HashState hash_state(container.size());
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
		inline std::size_t dice_hash_unordered_container(Container const &container) noexcept {
			std::size_t h{};
			for (auto const &it : container) {
				h = dice_hash_invertible_combine(h, dice_hash(it));
			}
			return h;
		}

		/** Helper function for tuple hashing.
	     * It simply hashes every parameter and then combines them.
	     * CAUTION: The order of the parameters CAN make a difference!
	     * @tparam Ts Parameter pack. The hash function must be defined for every type used.
	     * @param values The values to hash and combine.
	     * @return Hash value.
         */
		template<typename... Ts>
		inline std::size_t hash_and_combine(Ts &&...values) {
			return detail::hash_combine(std::initializer_list<std::size_t>{dice_hash(std::forward<Ts>(values))...});
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
		inline std::size_t hash_tuple(std::tuple<TupleArgs...> const &tuple, std::index_sequence<ids...> const &) {
			return hash_and_combine(std::get<ids>(tuple)...);
		}

	}// namespace detail

	template<typename T>
	requires std::is_fundamental_v<std::decay_t<T>>
	inline std::size_t dice_hash(T const &fundamental) noexcept {
		return detail::hash_primitive(fundamental);
	}


	template<typename CharT>
	inline std::size_t dice_hash(std::basic_string<CharT> const &str) noexcept {
		return detail::hash_bytes(str.data(), sizeof(CharT) * str.size());
	}

	template<typename CharT>
	inline std::size_t dice_hash(std::basic_string_view<CharT> const &sv) noexcept {
		return detail::hash_bytes(sv.data(), sizeof(CharT) * sv.size());
	}

	template<typename T>
	inline std::size_t dice_hash(T *ptr) noexcept {
		return detail::hash_primitive(ptr);
	}

	template<typename T>
	inline std::size_t dice_hash(std::unique_ptr<T> const &ptr) noexcept {
		return dice_hash(ptr.get());
	}

	template<typename T>
	inline std::size_t dice_hash(std::shared_ptr<T> const &ptr) noexcept {
		return dice_hash(ptr.get());
	}

	template<typename T, std::size_t N>
	inline std::size_t dice_hash(std::array<T, N> const &arr) noexcept {
		if constexpr (std::is_fundamental_v<T>) {
			return detail::hash_bytes(arr.data(), sizeof(T) * N);
		} else {
			return detail::dice_hash_ordered_container(arr);
		}
	}

	template<typename T>
	inline std::size_t dice_hash(std::vector<T> const &vec) noexcept {
		if constexpr (std::is_fundamental_v<T>) {
			static_assert(!std::is_same_v<std::decay_t<T>, bool>,
						  "vector of booleans has a special implementation which results into errors!");
			return detail::hash_bytes(vec.data(), sizeof(T) * vec.size());
		} else {
			return detail::dice_hash_ordered_container(vec);
		}
	}

	template<typename... TupleArgs>
	inline std::size_t dice_hash(std::tuple<TupleArgs...> const &tpl) noexcept {
		return detail::hash_tuple(tpl, std::make_index_sequence<sizeof...(TupleArgs)>());
	}

	template<typename T, typename V>
	inline std::size_t dice_hash(std::pair<T, V> const &p) noexcept {
		return detail::hash_and_combine(p.first, p.second);
	}

    template <>
    inline std::size_t dice_hash(std::monostate const&) noexcept {
		return Dice::hash::martinus::seed;
	}

    template<typename ...VariantArgs>
    inline std::size_t dice_hash(std::variant<VariantArgs...> const &var) noexcept {
        try {
            return std::visit([]<typename T>(T &&arg) { return dice_hash(std::forward<T>(arg)); }, var);
        }
        catch (std::bad_variant_access const &) {
            return Dice::hash::martinus::seed;
        }
    }

	template<typename T>
	requires is_ordered_container_v<T>
	inline std::size_t dice_hash(T const &container) noexcept {
		return detail::dice_hash_ordered_container(container);
	}

	template<typename T>
	requires is_unordered_container_v<T>
	inline std::size_t dice_hash(T const &container) noexcept {
		return detail::dice_hash_unordered_container(container);
	}

}// namespace Dice::hash

#endif//HYPERTRIE_DICEHASH_HPP