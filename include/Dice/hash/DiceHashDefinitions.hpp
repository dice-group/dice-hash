#ifndef HYPERTRIE_DICEHASHDEFINITIONS_HPP
#define HYPERTRIE_DICEHASHDEFINITIONS_HPP

/** @file
 * @brief Home of the declarations of the dice_hash functions.
 *
 * This is needed because the hash functions need to be able to call each other.
 * Because of that the declarations and definitions need to be separated.
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

#include "Dice/hash/Container_trait.hpp"

/** Home of the DiceHash and its helper functions.
 */
namespace Dice::hash {

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
	template<typename T>
	inline std::size_t dice_hash(T const &) noexcept {
		static_assert(AlwaysFalse<T>::value,
					  "The hash function is not defined for this type. You need to add an implementation yourself");
		return 0;
	}

	/** Implementation for fundamentals.
 * @tparam T Fundamental type.
 * @param fundamental Value to hash.
 * @return Hash value.
 */
	template<typename T>
	requires std::is_fundamental_v<std::decay_t<T>> inline std::size_t dice_hash(T const &fundamental) noexcept;

	/** Implementation for string types.
	 * @tparam CharT A char type. See the definition of std::string for more information.
	 * @param str The string to hash.
	 * @return Hash value.
	 */
	template<typename CharT>
	inline std::size_t dice_hash(std::basic_string<CharT> const &str) noexcept;

	/** Implementation for string view.
	 * @tparam CharT A char type. See the definition of std::string for more information.
	 * @param sv The string view to hash.
	 * @return Hash value.
	 */
	template<typename CharT>
	inline std::size_t dice_hash(std::basic_string_view<CharT> const &sv) noexcept;

	/** Implementation for raw pointers.
	 * CAUTION: hashes the POINTER, not the OBJECT POINTED TO!
	 * @tparam T A pointer type.
	 * @param ptr The pointer to hash.
	 * @return Hash value.
	 */
	template<typename T>
	inline std::size_t dice_hash(T *ptr) noexcept;

	/** Implementation for unique pointers.
	 * CAUTION: hashes the POINTER, not the OBJECT POINTED TO!
	 * @tparam T A unique pointer type.
	 * @param ptr The pointer to hash.
	 * @return Hash value.
	 */
	template<typename T>
	inline std::size_t dice_hash(std::unique_ptr<T> const &ptr) noexcept;

	/** implementation for shared pointers.
	 * CAUTION: hashes the POINTER, not the OBJECT POINTED TO!
	 * @tparam T A shared pointer type.
	 * @param ptr The pointer to hash.
	 * @return Hash value.
	 */
	template<typename T>
	inline std::size_t dice_hash(std::shared_ptr<T> const &ptr) noexcept;

	/** Implementation for std arrays.
	 * It will use different implementations if the type is fundamental or not.
	 * @tparam T The type of the values.
	 * @tparam N The number of values.
	 * @param arr The array itself.
	 * @return Hash value.
	 */
	template<typename T, std::size_t N>
	inline std::size_t dice_hash(std::array<T, N> const &arr) noexcept;

	/** Implementation for vectors.
	 * It will use different implementations for fundamental and non-fundamental types.
	 * @tparam T The type of the values.
	 * @param vec The vector itself.
	 * @return Hash value.
	 */
	template<typename T>
	inline std::size_t dice_hash(std::vector<T> const &vec) noexcept;

	/** Implementation for tuples.
	 * Will hash every entry and then combine the hashes.
	 * @tparam TupleArgs The types of the tuple values.
	 * @param tpl The tuple itself.
	 * @return Hash value.
	 */
	template<typename... TupleArgs>
	inline std::size_t dice_hash(std::tuple<TupleArgs...> const &tpl) noexcept;

	/** Implementation for pairs.
	 * Will hash the entries and then combine them.
	 * @tparam T Type of the first value.
	 * @tparam V Type of the second value.
	 * @param p The pair itself.
	 * @return Hash value.
	 */
	template<typename T, typename V>
	inline std::size_t dice_hash(std::pair<T, V> const &p) noexcept;

	/** Implementation for variant.
	 * Will hash the value which was set.
	 * The hash of a variant of a type is equal to the hash of the type.
	 * For example: a variant of int of 42 is equal to the hash of the int of 42.
	 * If the variant is valueless_by_exception, the seed will be returned.
	 * @tparam VariantArgs Types of the possible values.
	 * @param var The variant itself.
	 * @return Hash value.
	 */
	template<typename... VariantArgs>
	inline std::size_t dice_hash(std::variant<VariantArgs...> const &var) noexcept;

	/** Specialization for std::monostate.
	 * It is needed so its usage in std::variant is possible.
	 * Will simply return the seed.
	 * @return The seed of the hash function.
	 */
	template<>
	inline std::size_t dice_hash(std::monostate const &) noexcept;

	/** Implementation for ordered container.
	 * It uses a custom type trait to check if the type is in fact an ordered container.
	 * CAUTION: If you want to add another type to the trait, you might need to do it before this is included!
	 * @tparam T The container type.
	 * @param container The container itself.
	 * @return Hash value.
	 */
	template<typename T>
	requires is_ordered_container_v<T> inline std::size_t dice_hash(T const &container) noexcept;

	/** Implementation for unordered container.
	 * It uses a custom type trait to check if the type is in fact an unordered container.
	 * CAUTION: If you want to add another type to the trait, you might need to do it before this is included!
	 * @tparam T The container type.
	 * @param container The container itself.
	 * @return Hash value.
	 */
	template<typename T>
	requires is_unordered_container_v<T> inline std::size_t dice_hash(T const &container) noexcept;

	/** Combines two hashes to a new hash.
     * This function is commutative and invertible.
     * It is used in the unordered container functions. However this __will__ be replaced in the future.
     * @param a First hash.
     * @param b Second hash.
     * @return Combination of a and b.
     */
	inline std::size_t dice_hash_invertible_combine(std::size_t a, std::size_t b);

	/** Combine n hashes to a new hash.
     * Uses the base definition of Dice::hash::detail::dice_hash_invertible_combine for two elements.
     * So all properties are from that definition.
     * @tparam Args Needed so any number of arguments can be used.
     * @param a First hash.
     * @param args All other hashes.
     * @return Combination of all hashes.
     */
	template<typename... Args>
	inline std::size_t dice_hash_invertible_combine(std::size_t a, Args... args);

	/** Combine n hashes to a new hash.
     * Uses the Dice::hash::martinus::hash_combine for all elements at once.
     * So this is simply a wrapper for it.
     * @tparam Args std::size_t.
     * @param args Hashes. All std::size_t from type.
     * @return Combination of all hashes.
     */
	template<typename... Args>
	inline std::size_t dice_hash_combine(Args... args);

	/** Wrapper class for the Dice::hash::dice_hash function.
	 * It is a typical hash interface.
	 * @tparam T The type to define the hash for.
	 */
	template<typename T>
	struct DiceHash {
		/** Overloaded operator to calculate a hash.
     * Simply calls the dice_hash function for the specified type.
     * @param t The value to calculate the hash of.
     * @return Hash value.
     */
		std::size_t operator()(T const &t) const noexcept {
			return dice_hash(t);
		}
	};

}// namespace Dice::hash

#endif//HYPERTRIE_DICEHASHDEFINITIONS_HPP
