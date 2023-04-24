#ifndef DICE_HASH_LTHASH_HPP
#define DICE_HASH_LTHASH_HPP

#include "dice/hash/internal/lthash/Blake2xb.hpp"
#include "dice/hash/internal/lthash/MathEngine.hpp"

namespace dice::hash::internal::lthash {

	namespace detail {
		template <size_t B>
		struct Bits;

		template<>
		struct Bits<16> {
			static constexpr uint64_t data_mask = 0xffffffffffffffffULL;
			static constexpr bool needs_padding = false;
		};

		template<>
		struct Bits<20> {
			// In binary this mask looks like:
			// 00 <1 repeated 20 times> 0 <1 repeated 20 times> 0 <1 repeated 20 times>
			static constexpr uint64_t data_mask = ~0xC000020000100000ULL;
			static constexpr bool needs_padding = true;
		};

		template<>
		struct Bits<32> {
			static constexpr uint64_t data_mask = 0xffffffffffffffffULL;
			static constexpr bool needs_padding = false;
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
		static constexpr size_t elems_per_uint64() {
			return detail::Bits<n_bits_per_elem>::needsPadding ? (sizeof(uint64_t) * 8) / (n_bits_per_elem + 1)
															   : (sizeof(uint64_t) * 8) / n_bits_per_elem;
		}

		using Bits_t = detail::Bits<n_bits_per_elem>;
		static constexpr uint64_t data_mask = Bits_t::data_mask;
		static constexpr bool needs_padding = Bits_t::needs_padding;

	public:
		static constexpr size_t checksum_len = (n_elems / elems_per_uint64<n_bits_per_elem>()) * sizeof(uint64_t);
		static constexpr std::array<std::byte, checksum_len> default_checksum{};

	private:
		std::vector<std::byte> key_;
		std::vector<std::byte> checksum_;

		void hash_object(std::span<std::byte> out, std::span<std::byte const> obj) noexcept {
			blake2xb::Blake2xb blake{out.size(), key_};
			blake.digest(obj);
			std::move(blake).finish(out);

			if constexpr (needs_padding) {
				detail::MathEngine::clear_padding_bits(data_mask, out);
			}
		}

	public:
		explicit LtHash(std::span<std::byte const, checksum_len> initial_checksum = default_checksum) noexcept : checksum_(checksum_len) {
			set_checksum(initial_checksum);
		}

		~LtHash() noexcept {
			clear_key();
		}

		void set_key(std::span<std::byte const> key) {
			if (key.size() < crypto_generichash_blake2b_KEYBYTES_MIN || key.size() > crypto_generichash_blake2b_KEYBYTES_MAX) {
				throw std::runtime_error{"Invalid Blake2b key size"};
			}

			clear_key();
			key_ = std::vector<std::byte>{key.begin(), key.end()};
		}

		void clear_key() noexcept {
			sodium_memzero(key_.data(), key_.size());
			key_.resize(0);
		}

		void set_checksum(std::span<std::byte const, checksum_len> new_checksum) {
			std::copy(new_checksum.begin(), new_checksum.end(), checksum_.begin());

			if constexpr (needs_padding) {
				if (!detail::MathEngine::check_padding_bits(data_mask, checksum_)) [[unlikely]] {
					throw std::runtime_error{"Invalid checksum: found non-zero padding bits"};
				}
			}
		}

		void reset_checksum() noexcept {
			std::fill(checksum_.begin(), checksum_.end(), std::byte{0});
		}

		LtHash &combine_add(LtHash const &other) {
			if (!key_equal(other)) [[unlikely]] {
				throw std::runtime_error{"Cannot combine hashes with different keys"};
			}

			detail::MathEngine::add(data_mask, checksum_, other.checksum_, checksum_);
			return *this;
		}

		LtHash &combine_remove(LtHash const &other) {
			if (!key_equal(other)) [[unlikely]] {
				throw std::runtime_error{"Cannot combine hashes with different keys"};
			}

			detail::MathEngine::sub(data_mask, checksum_, other.checksum_, checksum_);
			return *this;
		}

		LtHash &add(std::span<std::byte const> obj) noexcept {
			using H = std::array<std::byte, checksum_len>;
			H h;
			hash_object(h, obj);
			detail::MathEngine::add(data_mask, n_bits_per_elem, checksum_, h, checksum_);
			return *this;
		}

		LtHash &remove(std::span<std::byte const> obj) noexcept {
			using H = std::array<std::byte, checksum_len>;
			H h;
			hash_object(h, obj);
			detail::MathEngine::sub(data_mask, n_bits_per_elem, checksum_, h, checksum_);
			return *this;
		}

		[[nodiscard]] std::span<std::byte const, checksum_len> checksum() const noexcept {
			return checksum_;
		}

		bool key_equal(LtHash const &other) const noexcept {
			return std::equal(key_.begin(), key_.end(), other.key_.begin(), other.key_.end());
		}

		bool key_equal(std::span<std::byte const> other_key) const noexcept {
			return std::equal(key_.begin(), key_.end(), other_key.begin(), other_key.end());
		}

		bool operator==(LtHash const &other) const noexcept {
			return sodium_memcmp(checksum_.data(), other.checksum_.data(), checksum_.size()) == 0;
		}

		bool operator!=(LtHash const &other) const noexcept {
			return !LtHash::operator==(*this, other);
		}
	};

} // namespace dice::hash::internal::lthash

#endif//DICE_HASH_LTHASH_HPP
