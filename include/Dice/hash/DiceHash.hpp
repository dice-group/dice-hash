#ifndef DICE_HASH_DICEHASH_HPP
#define DICE_HASH_DICEHASH_HPP

/** @file
 * @brief Home of the DiceHash implementation.
 *
 * To speed up tests of the Hypertrie and Tentris we needed to be able to serialize a Hypertrie and save it.
 * However the last hash function was not "stable", i.e. it chose two different random seeds, so the results differed.
 * Because of that (and to not worry about versioning problems) this hash function was created.
 */

#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include "Dice/hash/martinus_robinhood_hash.hpp"
#include "Dice/hash/Container_trait.hpp"
#include "Dice/hash/DiceHashPolicies.hpp"

/** Home of the DiceHash.
 *
 */
namespace Dice::hash{

	template <typename Policy, typename T>
	struct dice_hash_overload{
        template<typename>
        struct AlwaysFalse : std::false_type {};

		static std::size_t dice_hash(T const&) noexcept {
			static_assert(AlwaysFalse<T>::value,
						  "The hash function is not defined for this type. You need to add an implementation yourself");
			return 0;
		}
	};

	/** Class which contains all dice_hash functions.
	 * @tparam Policy The Policy the hash is based on.
	 */
	template <typename Policy>
	class dice_hash_templates : public Policy{
	private:
        using Policy::hash_fundamental;
		using Policy::hash_bytes;
		using Policy::hash_combine;
        using Policy::hash_invertible_combine;
		using Policy::ErrorValue;
        using typename Policy::HashState;

        template<typename Container>
        static std::size_t dice_hash_ordered_container(Container const &container) noexcept {
            HashState hash_state(container.size());
            std::size_t item_hash;
            for (const auto &item : container) {
                item_hash = dice_hash(item);
                hash_state.add(item_hash);
            }
            return hash_state.digest();
        }

        template<typename Container>
        static std::size_t dice_hash_unordered_container(Container const &container) noexcept {
            std::size_t h{};
            for (auto const &it : container) {
                h = hash_invertible_combine({h, dice_hash(it)});
            }
            return h;
        }

        template<typename... TupleArgs, std::size_t... ids>
        static std::size_t dice_hash_tuple(std::tuple<TupleArgs...> const &tuple, std::index_sequence<ids...> const &) {
            return hash_combine({dice_hash(std::get<ids>(tuple))...});
        }

	public:
        /** Base case for dice_hash.
         * Will never compile, will always static assert to false.
         * @tparam T
         * @return Nothing.
         */
        template<typename T>
        static std::size_t dice_hash(T const &t) noexcept {
			return dice_hash_overload<Policy, T>::dice_hash(t);
        }


        template <typename T>
        requires std::is_fundamental_v<std::decay_t<T>>
        static std::size_t dice_hash(T const &fundamental) noexcept {
            return hash_fundamental(fundamental);
        }

        template <typename CharT>
        static std::size_t dice_hash(std::basic_string<CharT> const &str) noexcept {
            return hash_bytes(str.data(), sizeof(CharT) * str.size());
        }

        template<typename CharT>
        static std::size_t dice_hash(std::basic_string_view<CharT> const &sv) noexcept {
            return hash_bytes(sv.data(), sizeof(CharT) * sv.size());
        }

        template <typename T>
        static std::size_t dice_hash(T *ptr) noexcept {
            return hash_fundamental(ptr);
        }

        template<typename T>
        static std::size_t dice_hash(std::unique_ptr<T> const &ptr) noexcept {
            return dice_hash(ptr.get());
        }

        template<typename T>
        static std::size_t dice_hash(std::shared_ptr<T> const &ptr) noexcept {
            return dice_hash(ptr.get());
        }

        template<typename T, std::size_t N>
        static std::size_t dice_hash(std::array<T, N> const &arr) noexcept {
            if constexpr (std::is_fundamental_v<T>) {
                return hash_bytes(arr.data(), sizeof(T) * N);
            } else {
                return dice_hash_ordered_container(arr);
            }
        }

        template<typename T>
        static std::size_t dice_hash(std::vector<T> const &vec) noexcept {
            if constexpr (std::is_fundamental_v<T>) {
                static_assert(!std::is_same_v<std::decay_t<T>, bool>,
                              "vector of booleans has a special implementation which results into errors!");
                return hash_bytes(vec.data(), sizeof(T) * vec.size());
            } else {
                return dice_hash_ordered_container(vec);
            }
        }

        template<typename... TupleArgs>
        static std::size_t dice_hash(std::tuple<TupleArgs...> const &tpl) noexcept {
            return dice_hash_tuple(tpl, std::make_index_sequence<sizeof...(TupleArgs)>());
        }

        template<typename T, typename V>
        static std::size_t dice_hash(std::pair<T, V> const &p) noexcept {
            return hash_combine({dice_hash(p.first), dice_hash(p.second)});
        }

        static std::size_t dice_hash(std::monostate const &) noexcept {
            return ErrorValue;
        }

        template<typename... VariantArgs>
        static std::size_t dice_hash(std::variant<VariantArgs...> const &var) noexcept {
            try {
                return std::visit([]<typename T>(T &&arg) { return dice_hash(std::forward<T>(arg)); }, var);
            } catch (std::bad_variant_access const &) {
                return ErrorValue;
            }
        }

        template<typename T>
        requires is_ordered_container_v<T>
        static std::size_t dice_hash(T const &container) noexcept {
            return dice_hash_ordered_container(container);
        }

        template<typename T>
        requires is_unordered_container_v<T>
        static std::size_t dice_hash(T const &container) noexcept {
            return dice_hash_unordered_container(container);
        }
	};

    template<typename T, typename Policy=::Dice::hash::Policies::Martinus>
	struct DiceHash : Policy{
        using Policy::hash_combine;
        using Policy::hash_invertible_combine;
        using Policy::ErrorValue;
		std::size_t operator()(T const &t) const noexcept {
			return dice_hash_templates<Policy>::dice_hash(t);
		}
	};


}// namespace Dice::hash::Policy
#endif//DICE_HASH_DICEHASH_HPP