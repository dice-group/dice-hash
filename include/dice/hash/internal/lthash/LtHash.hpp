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

#include <array>
#include <vector>

#include "dice/hash/internal/blake2xb/Blake2xb.hpp"
#include "dice/hash/internal/lthash/MathEngine.hpp"

namespace dice::hash::internal::lthash {

	namespace detail {
		template <size_t B>
		struct Bits;

		template<>
		struct Bits<16> {
			static constexpr uint64_t data_mask = 0xffffffffffffffffULL;
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
			static constexpr uint64_t data_mask = 0xffffffffffffffffULL;
			static constexpr bool needs_padding = false;
			static constexpr size_t bits_per_element = 32;
		};
	} // namespace detail

	/**
	 * @brief LtHash ported from folly::experimental::crypto
	 * @tparam B
	 * @tparam N
	 */
	template<size_t n_bits_per_elem, size_t n_elems>
		requires (n_elems >= 1000 && ((n_bits_per_elem == 16 && n_elems % 32 == 0)
										 || (n_bits_per_elem == 20 && n_elems % 24 == 0)
										 || (n_bits_per_elem == 32 && n_elems % 16 == 0)))
	struct LtHash {
	private:
		using Bits = detail::Bits<n_bits_per_elem>;
		using MathEngine = detail::MathEngine<Bits>;

		static constexpr size_t elems_per_uint64() {
			return Bits::needs_padding ? (sizeof(uint64_t) * 8) / (n_bits_per_elem + 1)
									   : (sizeof(uint64_t) * 8) / n_bits_per_elem;
		}

	public:
		static constexpr size_t element_bits = n_bits_per_elem;
		static constexpr bool needs_padding = Bits::needs_padding;
		static constexpr size_t element_count = n_elems;
		static constexpr size_t elements_per_uint64 = elems_per_uint64();
		static constexpr size_t checksum_len = (n_elems / elements_per_uint64) * sizeof(uint64_t);
		static constexpr std::array<std::byte, checksum_len> default_checksum{};

	private:
		std::vector<std::byte> key_;
		std::vector<std::byte> checksum_;

		void hash_object(std::span<std::byte, checksum_len> out, std::span<std::byte const> obj) noexcept {
			blake2xb::Blake2xb<checksum_len> blake{key_};
			blake.digest(obj);
			std::move(blake).finish(out);

			if constexpr (needs_padding) {
				MathEngine::clear_padding_bits(out);
			}
		}

	public:
		explicit LtHash(std::span<std::byte const, checksum_len> initial_checksum = default_checksum) noexcept(!needs_padding) : checksum_(checksum_len) {
			set_checksum(initial_checksum);
		}

		~LtHash() noexcept {
			clear_key();
		}

		template<size_t supplied_key_len>
			requires (supplied_key_len == std::dynamic_extent || (supplied_key_len >= crypto_generichash_blake2b_KEYBYTES_MIN
																 && supplied_key_len <= crypto_generichash_blake2b_KEYBYTES_MAX))
		void set_key(std::span<std::byte const, supplied_key_len> key) noexcept(supplied_key_len != std::dynamic_extent /* or key within bounds */) {
			if constexpr (supplied_key_len == std::dynamic_extent) {
				if (key.size() < crypto_generichash_blake2b_KEYBYTES_MIN || key.size() > crypto_generichash_blake2b_KEYBYTES_MAX) [[unlikely]] {
					throw std::runtime_error{"Invalid Blake2b key size"};
				}
			}

			sodium_memzero(key_.data(), key_.size());
			key_.resize(key.size());
			std::copy(key.begin(), key.end(), key_.begin());
		}

		void clear_key() noexcept {
			sodium_memzero(key_.data(), key_.size());
			key_.resize(0);
		}

		void set_checksum(std::span<std::byte const, checksum_len> new_checksum) noexcept(!needs_padding) {
			std::copy(new_checksum.begin(), new_checksum.end(), checksum_.begin());
			if constexpr (needs_padding) {
				if (!MathEngine::check_padding_bits(checksum_)) [[unlikely]] {
					throw std::runtime_error{"Invalid checksum: found non-zero padding bits"};
				}
			}
		}

		void reset() noexcept {
			std::fill(checksum_.begin(), checksum_.end(), std::byte{0});
		}

		LtHash &combine_add(LtHash const &other) {
			if (!key_equal(other)) [[unlikely]] {
				throw std::runtime_error{"Cannot combine hashes with different keys"};
			}

			MathEngine::add(checksum_, other.checksum_, checksum_);
			return *this;
		}

		LtHash &combine_remove(LtHash const &other) {
			if (!key_equal(other)) [[unlikely]] {
				throw std::runtime_error{"Cannot combine hashes with different keys"};
			}

			MathEngine::sub(checksum_, other.checksum_, checksum_);
			return *this;
		}

		LtHash &add(std::span<std::byte const> obj) noexcept {
			using H = std::array<std::byte, checksum_len>;
			H h;
			hash_object(h, obj);
			MathEngine::add(checksum_, h, checksum_);
			return *this;
		}

		LtHash &remove(std::span<std::byte const> obj) noexcept {
			using H = std::array<std::byte, checksum_len>;
			H h;
			hash_object(h, obj);
			MathEngine::sub(checksum_, h, checksum_);
			return *this;
		}

		[[nodiscard]] std::span<std::byte const, checksum_len> checksum() const noexcept {
			return {*reinterpret_cast<std::byte const (*)[checksum_len]>(checksum_.data())};
		}

		[[nodiscard]] bool key_equal(std::span<std::byte const> other_key) const noexcept {
			return std::equal(key_.begin(), key_.end(), other_key.begin(), other_key.end());
		}

		[[nodiscard]] bool key_equal(LtHash const &other) const noexcept {
			return key_equal(other.key_);
		}

		bool operator==(LtHash const &other) const noexcept {
			return sodium_memcmp(checksum_.data(), other.checksum_.data(), checksum_.size()) == 0;
		}

		bool operator!=(LtHash const &other) const noexcept {
			return !LtHash::operator==(other);
		}
	};

	using LtHash16 = LtHash<16, 1024>;
	using LtHash20 = LtHash<20, 1008>;
	using LtHash32 = LtHash<32, 1024>;

} // namespace dice::hash::internal::lthash

#endif//DICE_HASH_LTHASH_HPP
