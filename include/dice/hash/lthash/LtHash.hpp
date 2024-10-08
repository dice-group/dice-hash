#ifndef DICE_HASH_LTHASH_HPP
#define DICE_HASH_LTHASH_HPP

/**
 * Implementation of LtHash from the following paper:
 * 		Title: Securing Update Propagation with Homomorphic Hashing
 * 		Authors: Kevin Lewi and Wonho Kim and Ilya Maykov and Stephen Weis
 * 		Year: 2019
 * 		Url: https://eprint.iacr.org/2019/227
 *
 * @note Implementation adapted from https://github.com/facebook/folly/blob/main/folly/experimental/crypto/LtHash.h
 */

#if __has_include(<sodium.h>)

#include <array>
#include <cstring>
#include <vector>
#include <utility>
#include <memory>

#include <sodium.h>

#include "dice/hash/blake/Blake3.hpp"
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

		template<size_t min_key_extent, size_t max_key_extent>
		struct Key {
			std::array<std::byte, max_key_extent> key_{};
			size_t key_len_{};

			[[nodiscard]] constexpr std::span<std::byte const> get() const noexcept {
				return {key_.data(), key_len_};
			}

			constexpr void clear() noexcept {
				if (!std::is_constant_evaluated()) {
					sodium_memzero(key_.data(), key_.size());
				} else {
					std::fill(key_.begin(), key_.end(), std::byte{0});
				}

				key_len_ = 0;
			}

			template<size_t supplied_key_len>
				requires (supplied_key_len == std::dynamic_extent || (supplied_key_len >= min_key_extent
																	 && supplied_key_len <= max_key_extent))
			constexpr void set_unchecked(std::span<std::byte const, supplied_key_len> new_key) noexcept {
				assert(new_key.size() >= min_key_extent && new_key.size() <= max_key_extent);

				clear();
				std::copy(new_key.begin(), new_key.end(), key_.begin());
				key_len_ = new_key.size();
			}
		};

		template<size_t KeyExtent>
		struct Key<KeyExtent, KeyExtent> {
			std::array<std::byte, KeyExtent> key_{};

			[[nodiscard]] constexpr std::span<std::byte const, KeyExtent> get() const noexcept {
				return key_;
			}

			constexpr void clear() noexcept {
				if (!std::is_constant_evaluated()) {
					sodium_memzero(key_.data(), key_.size());
				} else {
					std::fill(key_.begin(), key_.end(), std::byte{0});
				}
			}

			constexpr void set_unchecked(std::span<std::byte const, KeyExtent> new_key) noexcept {
				clear();
				std::copy(new_key.begin(), new_key.end(), key_.begin());
			}
		};
	} // namespace detail

	/**
	 * @brief LtHash ported from folly::experimental::crypto
	 * @tparam n_bits_per_elem how many bits the individual state elements occupy
	 * @tparam n_elems how many individual state elements there are
	 * @tparam MathEngineT the math engine/instruction set to use for computations (defaults to the best your platform supports; in order (best to worst): AVX2, SSE2, x86_64)
	 */
	template<size_t n_bits_per_elem, size_t n_elems, template<size_t> typename HashT = blake3::Blake3, template<typename> typename MathEngineT = DefaultMathEngine>
	struct LtHash {
		static_assert((n_bits_per_elem == 16 && n_elems % 32 == 0)
					  || (n_bits_per_elem == 20 && n_elems % 24 == 0)
					  || (n_bits_per_elem == 32 && n_elems % 16 == 0));

		static_assert(MathEngine<MathEngineT, detail::Bits<n_bits_per_elem>>);

	private:
		template<size_t, size_t, template<size_t> typename, template<typename> typename>
		friend struct LtHash;

		using Bits = detail::Bits<n_bits_per_elem>;
		using MathEngine = MathEngineT<Bits>;

	public:
		static constexpr bool needs_padding = Bits::needs_padding;

		static constexpr size_t element_bits = n_bits_per_elem;
		static constexpr size_t element_count = n_elems;

		static constexpr size_t elements_per_uint64 = needs_padding ? (sizeof(uint64_t) * 8) / (element_bits + 1)
																	: (sizeof(uint64_t) * 8) / element_bits;

		static constexpr size_t checksum_len = (element_count / elements_per_uint64) * sizeof(uint64_t);
		static constexpr size_t checksum_align = MathEngine::min_buffer_align;

		static constexpr std::array<std::byte, checksum_len> default_checksum{};

	private:
		using Hash = HashT<checksum_len>;

		detail::Key<Hash::min_key_extent, Hash::max_key_extent> key_;
		alignas(checksum_align) std::array<std::byte, checksum_len> checksum_;


		constexpr void set_checksum_unchecked(std::span<std::byte const, checksum_len> new_checksum) noexcept {
			std::copy(new_checksum.begin(), new_checksum.end(), checksum_.begin());
		}

		[[nodiscard]] constexpr std::span<std::byte, checksum_len> checksum_mut() noexcept {
			return checksum_;
		}

		void hash_object(std::span<std::byte, checksum_len> out, std::span<std::byte const> obj) const noexcept {
			Hash::hash_single(obj, out, key_.get());

			if constexpr (needs_padding) {
				MathEngine::clear_padding_bits(out);
			}
		}

	public:
		/**
		 * @brief construct an LtHash using the (optionally) given initial_checksum
		 */
		explicit constexpr LtHash(std::span<std::byte const, checksum_len> initial_checksum = default_checksum) noexcept {
			set_checksum_unchecked(initial_checksum);
		}

		constexpr LtHash(LtHash const &other) noexcept = default;

		template<template<typename> typename MathEngineT2>
		constexpr LtHash(LtHash<n_bits_per_elem, n_elems, HashT, MathEngineT2> const &other) noexcept : key_{other.key_},
																										checksum_{other.checksum_} {
		}

		constexpr LtHash(LtHash &&other) noexcept : key_{other.key_},
													checksum_{other.checksum_} {
			other.clear_key();
		}


		template<template<typename> typename MathEngineT2>
		constexpr LtHash(LtHash<n_bits_per_elem, n_elems, HashT, MathEngineT2> &&other) noexcept : key_{other.key_},
																								   checksum_{other.checksum_} {
			other.clear_key();
		}

		constexpr LtHash &operator=(LtHash const &other) noexcept {
			if (this == &other) [[unlikely]] {
				return *this;
			}

			clear_key();
			key_ = other.key_;
			checksum_ = other.checksum_;
			return *this;
		}

		constexpr LtHash &operator=(LtHash &&other) noexcept {
			assert(this != &other);

			clear_key();
			key_ = other.key_;
			other.clear_key();
			checksum_ = other.checksum_;
			return *this;
		}

		constexpr ~LtHash() noexcept {
			clear_key();
		}

		/**
		 * @brief Checks if the internal Blake2Xb key is equal to the given key
		 * @note this function is not secured against timing attacks
		 */
		[[nodiscard]] constexpr bool key_equal(std::span<std::byte const> other_key) const noexcept {
			auto const this_key = key_.get();
			return std::equal(this_key.begin(), this_key.end(), other_key.begin(), other_key.end());
		}

		/**
		 * @brief Checks if *this and other have the same key for their Blake2Xb instances
		 * @note this functions is not secured against timing attacks
		 */
		[[nodiscard]] constexpr bool key_equal(LtHash const &other) const noexcept {
			return key_equal(other.key_.get());
		}

		/**
		 * @brief Sets the internal key for the Blake2Xb instance to the given key; securely erases the old key
		 * @throws std::invalid_argument if key.size() is not in blake2xb::min_key_extent..blake2xb::max_key_extent (inclusive); only if supplied_key_len == std::dynamic_extent
		 */
		template<size_t supplied_key_len>
			requires (supplied_key_len == std::dynamic_extent || (supplied_key_len >= Hash::min_key_extent
																 && supplied_key_len <= Hash::max_key_extent))
		constexpr void set_key(std::span<std::byte const, supplied_key_len> key) noexcept(supplied_key_len != std::dynamic_extent) {
			if constexpr (supplied_key_len == std::dynamic_extent) {
				if (key.size() < Hash::min_key_extent || key.size() > Hash::max_key_extent) [[unlikely]] {
					throw std::invalid_argument{"Invalid key size for Blake2Xb"};
				}
			}

			key_.set_unchecked(key);
		}

		/**
		 * @brief Clears the internal key for the Blake2Xb instance by securely erasing it
		 */
		constexpr void clear_key() noexcept {
			key_.clear();
		}

		[[nodiscard]] constexpr std::span<std::byte const, checksum_len> checksum() const noexcept {
			return checksum_;
		}

		/**
		 * @brief Checks if this->checksum() is equal to other_checksum (i.e. represent the same multiset)
		 * @note this function is _not_ secured against timing attacks
		 */
		[[nodiscard]] constexpr bool checksum_equal(std::span<std::byte const, checksum_len> other_checksum) const noexcept {
			return std::equal(checksum_.begin(), checksum_.end(), other_checksum.begin());
		}

		/**
		 * @brief Checks if *this and other have the same checksum (i.e. represent the same multiset)
		 * @note this function is _not_ secured against timing attacks
		 */
		[[nodiscard]] constexpr bool checksum_equal(LtHash const &other) const noexcept {
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
		constexpr void set_checksum(std::span<std::byte const, checksum_len> new_checksum) noexcept(!needs_padding) {
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
		constexpr void clear_checksum() noexcept {
			std::fill(checksum_.begin(), checksum_.end(), std::byte{0});
		}

		/**
		 * @brief Adds another LtHash to *this (via multiset-union)
		 * @param other another LtHash instance with the same key as *this
		 * @return reference to *this
		 * @throws std::invalid_argument if !this->key_equal(other)
		 */
		LtHash &combine_add(LtHash const &other) {
			if (!key_equal(other)) [[unlikely]] {
				throw std::invalid_argument{"Cannot combine hashes with different keys"};
			}

			MathEngine::add(checksum_mut(), other.checksum());
			return *this;
		}

		/**
		 * @brief Removes another LtHash from *this (via multiset-minus)
		 * @param other another LtHash instance with the same key as *this
		 * @return reference to *this
		 * @throws std::invalid_argument if !this->key_equal(other)
		 */
		LtHash &combine_remove(LtHash const &other) {
			if (!key_equal(other)) [[unlikely]] {
				throw std::invalid_argument{"Cannot combine hashes with different keys"};
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
		constexpr bool operator==(LtHash const &other) const noexcept {
			return checksum_equal(other);
		}

		/**
		 * @brief Checks if *this and other _do not_ have the same checksum (i.e. represent the same multiset)
		 * @note this function _is_ secured against timing attacks
		 */
		constexpr bool operator!=(LtHash const &other) const noexcept {
			return !LtHash::operator==(other);
		}
	};

	using LtHash16 = LtHash<16, 1024>;
	using LtHash20 = LtHash<20, 1008>;
	using LtHash32 = LtHash<32, 1024>;

} // namespace dice::hash::lthash

#else
#error "Cannot include LtHash.hpp if sodium is not available"
#endif

#endif//DICE_HASH_LTHASH_HPP
