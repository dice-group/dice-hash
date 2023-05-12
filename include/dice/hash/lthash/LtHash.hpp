#ifndef DICE_HASH_LTHASH_HPP
#define DICE_HASH_LTHASH_HPP

#ifndef DICE_HASH_WITH_SODIUM
#error "Cannot include LtHash if libsodium support is disabled."
#else
/**
 * Implementation of LtHash from the following paper:
 * 		Title: Securing Update Propagation with Homomorphic Hashing
 * 		Authors: Kevin Lewi and Wonho Kim and Ilya Maykov and Stephen Weis
 * 		Year: 2019
 * 		Url: https://eprint.iacr.org/2019/227
 *
 * @note Implementation adapted from https://github.com/facebook/folly/blob/main/folly/experimental/crypto/LtHash.h
 */

#include <array>
#include <cstring>
#include <vector>
#include <utility>
#include <memory>

#include "dice/hash/blake2/Blake2Xb.hpp"
#include "dice/hash/lthash/MathEngine.hpp"

namespace dice::hash::lthash {

	namespace detail {
		template <size_t n_bits_per_element>
		struct Bits;

		template<>
		struct Bits<16> {
			static constexpr bool needs_padding = false;
			static constexpr size_t bits_per_element = 16;
		};

		template<>
		struct Bits<20> {
			// In binary this mask looks like:
			// 00 <1 repeated 20 times> 0 <1 repeated 20 times> 0 <1 repeated 20 times>
			static constexpr uint64_t data_mask = ~0xC000020000100000ULL;
			static constexpr bool needs_padding = true;
			static constexpr size_t bits_per_element = 20;
		};

		template<>
		struct Bits<32> {
			static constexpr bool needs_padding = false;
			static constexpr size_t bits_per_element = 32;
		};

		template<size_t size, size_t align>
		struct AlignedByteBuffer {
			alignas(align) std::byte data_[size];
		};
	} // namespace detail

	/**
	 * @brief LtHash ported from folly::experimental::crypto
	 * @tparam n_bits_per_elem how many bits the individual state elements occupy
	 * @tparam n_elems how many individual state elements there are
	 * @tparam MathEngineT the math engine/instruction set to use for computations (defaults to the best your platform supports in order (best to worst): AVX2, SSE2, x86_64)
	 */
	template<size_t n_bits_per_elem, size_t n_elems, template<typename> typename MathEngineT = DefaultMathEngine>
		requires (((n_bits_per_elem == 16 && n_elems % 32 == 0)
				  	|| (n_bits_per_elem == 20 && n_elems % 24 == 0)
				  	|| (n_bits_per_elem == 32 && n_elems % 16 == 0))
				 && MathEngine<MathEngineT, detail::Bits<n_bits_per_elem>>)
	struct LtHash {
	private:
		using Bits = detail::Bits<n_bits_per_elem>;
		using MathEngine = MathEngineT<Bits>;

	public:
		static constexpr bool needs_padding = Bits::needs_padding;
		static constexpr size_t element_bits = n_bits_per_elem;
		static constexpr size_t element_count = n_elems;

		static constexpr size_t elements_per_uint64 = needs_padding ? (sizeof(uint64_t) * 8) / (element_bits + 1)
																	: (sizeof(uint64_t) * 8) / element_bits;

		static constexpr size_t checksum_len = (element_count / elements_per_uint64) * sizeof(uint64_t);

		static constexpr std::array<std::byte, checksum_len> default_checksum{};

	private:
		size_t key_len_;
		std::byte key_[blake2xb::max_key_extent];

		alignas(MathEngine::min_buffer_align) std::byte checksum_[checksum_len];


		void set_checksum_unchecked(std::span<std::byte const, checksum_len> new_checksum) noexcept {
			std::memcpy(checksum_, new_checksum.data(), checksum_len);
		}

		[[nodiscard]] std::span<std::byte, checksum_len> checksum_mut() noexcept {
			return checksum_;
		}

		template<size_t supplied_key_len>
			requires (supplied_key_len == std::dynamic_extent || (supplied_key_len >= blake2xb::min_key_extent
																 && supplied_key_len <= blake2xb::max_key_extent))
		void set_key_unchecked(std::span<std::byte const, supplied_key_len> new_key) noexcept {
			assert(new_key.size() >= blake2xb::min_key_extent && new_key.size() <= blake2xb::max_key_extent);
			sodium_memzero(key_, key_len_);
			std::memcpy(key_, new_key.data(), new_key.size());
			key_len_ = new_key.size();
		}

		[[nodiscard]] std::span<std::byte const> key() const noexcept {
			return {key_, key_len_};
		}

		void hash_object(std::span<std::byte, checksum_len> out, std::span<std::byte const> obj) const noexcept {
			blake2xb::Blake2Xb<checksum_len>::hash_single(obj, out, key());

			if constexpr (needs_padding) {
				MathEngine::clear_padding_bits(out);
			}
		}

	public:
		/**
		 * @brief construct an LtHash using the (optionally) given initial_checksum
		 */
		explicit LtHash(std::span<std::byte const, checksum_len> initial_checksum = default_checksum) noexcept : key_len_{0} {
			set_checksum_unchecked(initial_checksum);
		}

		LtHash(LtHash const &other) noexcept = default;

		LtHash(LtHash &&other) noexcept : key_len_{other.key_len_} {
			if (auto const k = other.key(); !k.empty()) {
				set_key_unchecked(k);
			}

			other.clear_key();
			set_checksum_unchecked(other.checksum());
		}

		LtHash &operator=(LtHash const &other) noexcept {
			if (this == &other) [[unlikely]] {
				return *this;
			}

			clear_key();
			if (auto const k = other.key(); !k.empty()) {
				set_key_unchecked(k);
			}

			set_checksum_unchecked(other.checksum());
			return *this;
		}

		LtHash &operator=(LtHash &&other) noexcept {
			assert(this != &other);

			clear_key();
			if (auto const k = other.key(); !k.empty()) {
				set_key_unchecked(k);
			}

			other.clear_key();
			set_checksum_unchecked(other.checksum());
			return *this;
		}

		~LtHash() noexcept {
			clear_key();
		}

		/**
		 * @brief Checks if the internal Blake2Xb key is equal to the given key
		 * @note this function is not secured against timing attacks
		 */
		[[nodiscard]] bool key_equal(std::span<std::byte const> other_key) const noexcept {
			if (key_len_ != other_key.size()) {
				return false;
			}

			return std::memcmp(key_, other_key.data(), key_len_) == 0;
		}

		/**
		 * @brief Checks if *this and other have the same key for their Blake2Xb instances
		 * @note this functions is not secured against timing attacks
		 */
		[[nodiscard]] bool key_equal(LtHash const &other) const noexcept {
			return key_equal(other.key());
		}

		/**
		 * @brief Sets the internal key for the Blake2Xb instance to the given key; securely erases the old key
		 * @throws std::invalid_argument if key.size() is not in blake2xb::min_key_extent..blake2xb::max_key_extent (inclusive); only if supplied_key_len == std::dynamic_extent
		 */
		template<size_t supplied_key_len>
			requires (supplied_key_len == std::dynamic_extent || (supplied_key_len >= blake2xb::min_key_extent
																 && supplied_key_len <= blake2xb::max_key_extent))
		void set_key(std::span<std::byte const, supplied_key_len> key) noexcept(supplied_key_len != std::dynamic_extent) {
			if constexpr (supplied_key_len == std::dynamic_extent) {
				if (key.size() < blake2xb::min_key_extent || key.size() > blake2xb::max_key_extent) [[unlikely]] {
					throw std::invalid_argument{"Invalid key size for Blake2Xb"};
				}
			}

			set_key_unchecked(key);
		}

		/**
		 * @brief Clears the internal key for the Blake2Xb instance by securely erasing it
		 */
		void clear_key() noexcept {
			sodium_memzero(key_, key_len_);
			key_len_ = 0;
		}

		[[nodiscard]] std::span<std::byte const, checksum_len> checksum() const noexcept {
			return checksum_;
		}

		/**
		 * @brief Checks if this->checksum() is equal to other_checksum (i.e. represent the same multiset)
		 * @note this function is _not_ secured against timing attacks
		 */
		[[nodiscard]] bool checksum_equal(std::span<std::byte const, checksum_len> other_checksum) const noexcept {
			return std::memcmp(checksum_, other_checksum.data(), checksum_len) == 0;
		}

		/**
		 * @brief Checks if *this and other have the same checksum (i.e. represent the same multiset)
		 * @note this function is _not_ secured against timing attacks
		 */
		[[nodiscard]] bool checksum_equal(LtHash const &other) const noexcept {
			return checksum_equal(other.checksum());
		}

		/**
		 * @brief Checks if this->checksum() is equal to other_checksum (i.e. represent the same multiset)
		 * @note this function is secured against timing attacks
		 */
		[[nodiscard]] bool checksum_equal_constant_time(std::span<std::byte const, checksum_len> other_checksum) const noexcept {
			return sodium_memcmp(checksum_, other_checksum.data(), checksum_len) == 0;
		}

		/**
		 * @brief Checks if *this and other have the same checksum (i.e. represent the same multiset)
		 * @note this function is secured against timing attacks
		 */
		[[nodiscard]] bool checksum_equal_constant_time(LtHash const &other) const noexcept {
			return checksum_equal_constant_time(other.checksum());
		}

		/**
		 * @brief Explicitly sets the current checksum to the given one
		 * @throws std::invalid_argument if new_checksum has invalid padding; only if needs_padding
		 */
		void set_checksum(std::span<std::byte const, checksum_len> new_checksum) noexcept(!needs_padding) {
			set_checksum_unchecked(new_checksum);
			if constexpr (needs_padding) {
				if (!MathEngine::check_padding_bits(checksum())) [[unlikely]] {
					throw std::invalid_argument{"Invalid checksum: found non-zero padding bits"};
				}
			}
		}

		/**
		 * @brief Clears the current checksum
		 */
		void clear_checksum() noexcept {
			std::memset(checksum_, 0, checksum_len);
		}

		/**
		 * @brief Adds another LtHash to *this (via multiset-union)
		 * @param other another LtHash instance with the same key as *this
		 * @return reference to *this
		 */
		LtHash &combine_add(LtHash const &other) /*noexcept(this->key_equal(other))*/ {
			if (!key_equal(other)) [[unlikely]] {
				throw std::runtime_error{"Cannot combine hashes with different keys"};
			}

			MathEngine::add(checksum_mut(), other.checksum());
			return *this;
		}

		/**
		 * @brief Removes another LtHash from *this (via multiset-minus)
		 * @param other another LtHash instance with the same key as *this
		 * @return reference to *this
		 */
		LtHash &combine_remove(LtHash const &other) /*noexcept(this->key_equal(other))*/ {
			if (!key_equal(other)) [[unlikely]] {
				throw std::runtime_error{"Cannot combine hashes with different keys"};
			}

			MathEngine::sub(checksum_mut(), other.checksum());
			return *this;
		}

		/**
		 * @brief Adds a single object to this LtHash instance
		 * @param obj object to add
		 * @return reference to *this
		 */
		LtHash &add(std::span<std::byte const> obj) noexcept {
			alignas(MathEngine::min_buffer_align) std::array<std::byte, checksum_len> obj_hash;
			hash_object(obj_hash, obj);
			MathEngine::add(checksum_mut(), std::span<std::byte const, checksum_len>{obj_hash});
			return *this;
		}

		/**
		 * @brief Removes a single object from this LtHash instance
		 * @param obj object to remove
		 * @return reference to *this
		 */
		LtHash &remove(std::span<std::byte const> obj) noexcept {
			alignas(MathEngine::min_buffer_align) std::array<std::byte, checksum_len> obj_hash;
			hash_object(obj_hash, obj);
			MathEngine::sub(checksum_mut(), std::span<std::byte const, checksum_len>{obj_hash});
			return *this;
		}

		/**
		 * @brief Checks if *this and other have the same checksum (i.e. represent the same multiset)
		 * @note this function is _not_ secured against timing attacks
		 */
		bool operator==(LtHash const &other) const noexcept {
			return checksum_equal(other);
		}

		/**
		 * @brief Checks if *this and other _do not_ have the same checksum (i.e. represent the same multiset)
		 * @note this function _is_ secured against timing attacks
		 */
		bool operator!=(LtHash const &other) const noexcept {
			return !LtHash::operator==(other);
		}
	};

	using LtHash16 = LtHash<16, 1024>;
	using LtHash20 = LtHash<20, 1008>;
	using LtHash32 = LtHash<32, 1024>;

} // namespace dice::hash::lthash

#endif//DICE_HASH_WITH_SODIUM
#endif//DICE_HASH_LTHASH_HPP
