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
#include <vector>
#include <utility>
#include <memory>

#include "dice/hash/blake2/Blake2xb.hpp"
#include "dice/hash/lthash/MathEngine.hpp"

namespace dice::hash::lthash {

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

		template<size_t size, size_t align>
		struct AlignedByteBuffer {
			alignas(align) std::byte data_[size];
		};
	} // namespace detail

	/**
	 * @brief LtHash ported from folly::experimental::crypto
	 * @tparam B
	 * @tparam N
	 */
	template<size_t n_bits_per_elem, size_t n_elems, template<typename> typename MathEngineT = DefaultMathEngine, typename Allocator = std::allocator<std::byte>>
		requires ((n_elems >= 1000 && ((n_bits_per_elem == 16 && n_elems % 32 == 0)
										 || (n_bits_per_elem == 20 && n_elems % 24 == 0)
										 || (n_bits_per_elem == 32 && n_elems % 16 == 0)))
				 && MathEngine<MathEngineT, detail::Bits<n_bits_per_elem>>)
	struct LtHash {
	private:
		using Bits = detail::Bits<n_bits_per_elem>;
		using MathEngine = MathEngineT<Bits>;

		static constexpr size_t elems_per_uint64() {
			return Bits::needs_padding ? (sizeof(uint64_t) * 8) / (n_bits_per_elem + 1)
									   : (sizeof(uint64_t) * 8) / n_bits_per_elem;
		}

	public:
		using allocator_type = Allocator;
		static constexpr size_t element_bits = n_bits_per_elem;
		static constexpr bool needs_padding = Bits::needs_padding;
		static constexpr size_t element_count = n_elems;
		static constexpr size_t elements_per_uint64 = elems_per_uint64();
		static constexpr size_t checksum_len = (n_elems / elements_per_uint64) * sizeof(uint64_t);
		static constexpr std::array<std::byte, checksum_len> default_checksum{};

	private:
		using ChecksumBuf = detail::AlignedByteBuffer<checksum_len, MathEngine::min_buffer_align>;
		using alloc_traits = std::allocator_traits<Allocator>;
		using ChecksumBuf_alloc_traits = typename alloc_traits::template rebind_traits<ChecksumBuf>;
		using ChecksumBuf_allocator = typename ChecksumBuf_alloc_traits::allocator_type;
		using ChecksumBuf_pointer = typename ChecksumBuf_alloc_traits::pointer;

		std::vector<std::byte, Allocator> key_;

		ChecksumBuf_allocator checksum_alloc_;
		ChecksumBuf_pointer checksum_;

		void hash_object(std::span<std::byte, checksum_len> out, std::span<std::byte const> obj) noexcept {
			auto blake = [&]() {
				if (key_.empty()) {
					return blake2xb::Blake2xb<checksum_len>{};
				} else {
					return blake2xb::Blake2xb<checksum_len>{std::span<std::byte const>{key_.data(), key_.size()}};
				}
			}();

			blake.digest(obj);
			std::move(blake).finish(out);

			if constexpr (needs_padding) {
				MathEngine::clear_padding_bits(out);
			}
		}

		void set_checksum_unchecked(std::span<std::byte const, checksum_len> new_checksum) noexcept {
			std::copy(new_checksum.begin(), new_checksum.end(), std::begin(checksum_->data_));
		}

	public:
		explicit LtHash(allocator_type alloc = allocator_type{}) : key_{alloc},
																   checksum_alloc_{alloc},
																   checksum_{ChecksumBuf_alloc_traits::allocate(checksum_alloc_, 1)} {
			set_checksum_unchecked(default_checksum);
		}

		explicit LtHash(std::span<std::byte const, checksum_len> initial_checksum,
						allocator_type alloc = allocator_type{}) : key_{alloc},
																   checksum_alloc_{alloc},
																   checksum_{ChecksumBuf_alloc_traits::allocate(checksum_alloc_, 1)} {
			set_checksum(initial_checksum);
		}

		LtHash(LtHash const &other) : key_{other.key_},
									  checksum_alloc_{ChecksumBuf_alloc_traits::select_on_container_copy_construction(other.checksum_alloc_)} {
			checksum_ = ChecksumBuf_alloc_traits::allocate(checksum_alloc_, 1);
			set_checksum_unchecked(other.checksum_->data_);
		}

		LtHash(LtHash &&other) noexcept : key_{std::move(other.key_)},
										  checksum_alloc_{std::move(other.checksum_alloc_)},
										  checksum_{std::exchange(other.checksum_, ChecksumBuf_pointer{})} {
		}

		LtHash &operator=(LtHash const &other) {
			if (this == &other) {
				return *this;
			}

			clear_key();
			if (!other.key_.empty()) {
				set_key(std::span<std::byte const>{other.key_});
			}

			checksum_alloc_ = ChecksumBuf_alloc_traits::select_on_container_copy_construction(other.checksum_alloc_);
			if (checksum_ == nullptr) {
				checksum_ = ChecksumBuf_alloc_traits::allocate(checksum_alloc_, 1);
			}
			set_checksum_unchecked(other.checksum_->data_);

			return *this;
		}

		LtHash &operator=(LtHash &&other) noexcept {
			if (this == &other) {
				return *this;
			}

			clear_key();
			key_ = std::move(other.key_);
			checksum_alloc_ = std::move(other.checksum_alloc_);
			checksum_ = std::exchange(other.checksum_, ChecksumBuf_pointer{});
			return *this;
		}

		~LtHash() noexcept {
			clear_key();

			if (checksum_ != nullptr) {
				checksum_alloc_.deallocate(checksum_, 1);
			}
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
			set_checksum_unchecked(new_checksum);
			if constexpr (needs_padding) {
				if (!MathEngine::check_padding_bits(checksum_->data_)) [[unlikely]] {
					throw std::runtime_error{"Invalid checksum: found non-zero padding bits"};
				}
			}
		}

		void clear_checksum() noexcept {
			std::fill(std::begin(checksum_->data_), std::end(checksum_->data_), std::byte{0});
		}

		LtHash &combine_add(LtHash const &other) {
			if (!key_equal(other)) [[unlikely]] {
				throw std::runtime_error{"Cannot combine hashes with different keys"};
			}

			MathEngine::add(checksum_->data_, other.checksum_->data_, checksum_->data_);
			return *this;
		}

		LtHash &combine_remove(LtHash const &other) {
			if (!key_equal(other)) [[unlikely]] {
				throw std::runtime_error{"Cannot combine hashes with different keys"};
			}

			MathEngine::sub(checksum_->data_, other.checksum_->data_, checksum_->data_);
			return *this;
		}

		LtHash &add(std::span<std::byte const> obj) noexcept {
			ChecksumBuf h;
			hash_object(h.data_, obj);
			MathEngine::add(checksum_->data_, h.data_, checksum_->data_);
			return *this;
		}

		LtHash &remove(std::span<std::byte const> obj) noexcept {
			ChecksumBuf h;
			hash_object(h.data_, obj);
			MathEngine::sub(checksum_->data_, h.data_, checksum_->data_);
			return *this;
		}

		[[nodiscard]] std::span<std::byte const, checksum_len> checksum() const noexcept {
			return checksum_->data_;
		}

		[[nodiscard]] bool key_equal(std::span<std::byte const> other_key) const noexcept {
			return std::equal(key_.begin(), key_.end(), other_key.begin(), other_key.end());
		}

		[[nodiscard]] bool key_equal(LtHash const &other) const noexcept {
			return key_equal(other.key_);
		}

		bool operator==(LtHash const &other) const noexcept {
			return sodium_memcmp(checksum_->data_, other.checksum_->data_, checksum_len) == 0;
		}

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
