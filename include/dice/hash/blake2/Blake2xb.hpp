#ifndef DICE_HASH_BLAKE2XB_HPP
#define DICE_HASH_BLAKE2XB_HPP

#ifndef DICE_HASH_WITH_SODIUM
#error "Cannot include Blake2Xb if libsodium support is disabled."
#else
/**
 * @brief Implementation of Blake2xb (https://www.blake2.net/blake2x.pdf)
 * @note Implementation adapted from https://github.com/facebook/folly/blob/main/folly/experimental/crypto/Blake2xb.h
 */

#include "dice/hash/blake2/Blake2b.hpp"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <limits>
#include <cassert>

#include <sodium.h>

namespace dice::hash::blake2xb {

	namespace detail {
		template<typename T>
		inline std::byte const *byte_iter(T const &value) noexcept {
			return reinterpret_cast<std::byte const *>(&value);
		}

		template<typename T>
		inline std::byte *byte_iter_mut(T &value) noexcept {
			return reinterpret_cast<std::byte *>(&value);
		}

		template<typename T>
		inline T little_endian(T const &value) noexcept {
			if constexpr (std::endian::native == std::endian::little) {
				return value;
			} else {
				auto const *input_beg = byte_iter(value);
				auto const *input_end = input_beg + sizeof(T);
				T output;

				std::reverse_copy(input_beg, input_end, byte_iter_mut(output));
				return output;
			}
		}
	} // namespace detail

	inline constexpr size_t min_output_extent = 1;
	inline constexpr size_t max_output_extent = std::numeric_limits<uint32_t>::max() - 1;
	inline constexpr size_t dynamic_output_extent = std::dynamic_extent;

	using ::dice::hash::blake2b::salt_extent;
	using ::dice::hash::blake2b::default_salt;

	using ::dice::hash::blake2b::personality_extent;
	using ::dice::hash::blake2b::default_personality;

	using ::dice::hash::blake2b::min_key_extent;
	using ::dice::hash::blake2b::max_key_extent;
	using ::dice::hash::blake2b::default_key_extent;
	using ::dice::hash::blake2b::dynamic_key_extent;

	using ::dice::hash::blake2b::generate_key;

	/**
	 * @brief Blake2xb ported from folly::experimental::crypto
	 */
	template<size_t OutputExtent = dynamic_output_extent>
		requires (OutputExtent == dynamic_output_extent || (OutputExtent >= min_output_extent && OutputExtent <= max_output_extent))
	struct Blake2xb {
		static constexpr size_t output_extent = OutputExtent;

	private:
		static constexpr uint32_t dynamic_output_extend_magic = std::numeric_limits<uint32_t>::max();

		struct ParamBlock {
			uint8_t digest_len;
			uint8_t key_len;
			uint8_t fanout;
			uint8_t depth;
			uint32_t leaf_len;
			uint32_t node_off;
			uint32_t xof_digest_len;
			uint8_t node_depth;
			uint8_t inner_len;
			uint8_t reserved[14];
			uint8_t salt[salt_extent];
			uint8_t personality[personality_extent];
		};

		ParamBlock param_{};
		crypto_generichash_blake2b_state state_;

		void init_state(std::span<std::byte const> key) {
			static constexpr std::array<uint64_t, 8> init_vec{0x6a09e667f3bcc908ULL,
															  0xbb67ae8584caa73bULL,
															  0x3c6ef372fe94f82bULL,
															  0xa54ff53a5f1d36f1ULL,
															  0x510e527fade682d1ULL,
															  0x9b05688c2b3e6c1fULL,
															  0x1f83d9abfb41bd6bULL,
															  0x5be0cd19137e2179ULL};

#if SODIUM_LIBRARY_VERSION_MAJOR > 10 || (SODIUM_LIBRARY_VERSION_MAJOR == 10 && SODIUM_LIBRARY_VERSION_MINOR >= 2)
			// In libsodium 1.0.17, the crypto_generichash_blake2b_state struct was made
			// opaque. We have to copy the internal definition of the real struct here
			// so we can properly initialize it.
			// see https://github.com/jedisct1/libsodium/blob/master/src/libsodium/crypto_generichash/blake2b/ref/blake2.h
			struct Blake2bState {
				uint64_t h[8];
				uint64_t t[2];
				uint64_t f[2];
				uint8_t buf[256];
				size_t buflen;
				uint8_t last_node;
			};
			auto *state = reinterpret_cast<Blake2bState *>(&state_);
#else
			auto *state = &state_;
#endif

			static_assert(sizeof(ParamBlock) == sizeof(init_vec));
			std::span<uint64_t const, init_vec.size()> param{*reinterpret_cast<uint64_t const(*)[init_vec.size()]>(&param_)};

			// state->h = init_vec xor param
			for (size_t ix = 0; ix < init_vec.size(); ++ix) {
				state->h[ix] = init_vec[ix] ^ detail::little_endian(param[ix]);
			}

			// zero everything between state->t and state->last_node (inclusive)
			std::fill(detail::byte_iter_mut(state->t),
					  detail::byte_iter_mut(state->last_node) + sizeof(state->last_node),
					  std::byte{});

			if (!key.empty()) {
				std::array<std::byte, 128> block; {
					auto write_end = std::copy(key.begin(), key.end(), block.begin());
					std::fill(write_end, block.end(), std::byte{});
				}

				digest(block);
				sodium_memzero(block.data(), block.size());// erase key from stack
			}
		}

		struct PrivateTag {};
		static constexpr PrivateTag private_tag{};

		Blake2xb(PrivateTag,
				 size_t output_len,
				 std::span<std::byte const> key,
				 std::span<std::byte const, salt_extent> salt,
				 std::span<std::byte const, personality_extent> personality) {

			if (output_len == 0) {
				output_len = dynamic_output_extend_magic;
			} else if (output_len > max_output_extent) {
				throw std::runtime_error{"Output length too large"};
			}

			if (!key.empty()) {
				if (key.size() < min_key_extent || key.size() > max_key_extent) {
					throw std::runtime_error{"Invalid blake2b key size"};
				}
			}

			if (auto const res = sodium_init(); res == -1) {
				throw std::runtime_error{"Could not initialize sodium"};
			}

			param_.digest_len = crypto_generichash_blake2b_BYTES_MAX;
			param_.key_len = static_cast<uint8_t>(key.size());
			param_.fanout = 1;
			param_.depth = 1;
			param_.xof_digest_len = detail::little_endian(output_len);

			std::copy(salt.begin(), salt.end(), detail::byte_iter_mut(param_.salt));
			std::copy(personality.begin(), personality.end(), detail::byte_iter_mut(param_.personality));

			init_state(key);
		}

	public:
		explicit Blake2xb(size_t output_len,
						  std::span<std::byte const> key = {},
						  std::span<std::byte const, salt_extent> salt = default_salt,
						  std::span<std::byte const, personality_extent> personality = default_personality) /*noexcept(sodium is initialized && output_len is within size constraints && key.size() is within size constaints)*/
			requires (output_extent == dynamic_output_extent)
			: Blake2xb{private_tag, output_len, key, salt, personality} {
		}

		explicit Blake2xb(std::span<std::byte const> key = {},
						  std::span<std::byte const, salt_extent> salt = default_salt,
						  std::span<std::byte const, personality_extent> personality = default_personality) /*noexcept(sodium is initialized && key.size() is within size constraints)*/
			: Blake2xb{private_tag, output_extent == dynamic_output_extent ? 0 : output_extent, key, salt, personality} {
		}

		void digest(std::span<std::byte const> data) noexcept {
			auto const res = crypto_generichash_blake2b_update(&state_,
															   reinterpret_cast<unsigned char const *>(data.data()),
															   data.size());
			// cannot fail, see: https://github.com/jedisct1/libsodium/blob/8d9ab6cd764926d4bf1168b122f4a3ff4ea686a0/src/libsodium/crypto_generichash/blake2b/ref/blake2b-ref.c#L263
			assert(res == 0);
		}

		void finish(std::span<std::byte, output_extent> out) && noexcept(output_extent != dynamic_output_extent) {
			if constexpr (output_extent == dynamic_output_extent) {
				auto const expected_out_len = detail::little_endian(param_.xof_digest_len);
				if (expected_out_len != dynamic_output_extend_magic && out.size() != expected_out_len) {
					// was known in advance and does not match
					throw std::runtime_error{"Buffer length must match output length"};
				}
			}

			std::array<std::byte, crypto_generichash_blake2b_BYTES_MAX> h0;
			auto res = crypto_generichash_blake2b_final(&state_,
														reinterpret_cast<unsigned char *>(h0.data()),
														h0.size());
			// cannot fail on proper use, see: https://github.com/jedisct1/libsodium/blob/8d9ab6cd764926d4bf1168b122f4a3ff4ea686a0/src/libsodium/crypto_generichash/blake2b/ref/blake2b-ref.c#L299
			assert(res == 0);

			param_.key_len = 0;
			param_.fanout = 0;
			param_.depth = 0;
			param_.leaf_len = detail::little_endian(static_cast<uint32_t>(crypto_generichash_blake2b_BYTES_MAX));
			param_.xof_digest_len = detail::little_endian(static_cast<uint32_t>(out.size()));
			param_.node_depth = 0;
			param_.inner_len = crypto_generichash_blake2b_BYTES_MAX;

			size_t pos = 0;
			size_t remaining = out.size();

			while (remaining > 0) {
				param_.node_off = detail::little_endian(static_cast<uint32_t>(pos / crypto_generichash_blake2b_BYTES_MAX));

				size_t const len = std::min(static_cast<size_t>(crypto_generichash_blake2b_BYTES_MAX), remaining);
				param_.digest_len = static_cast<uint8_t>(len);

				init_state({});
				res = crypto_generichash_blake2b_update(&state_,
														reinterpret_cast<unsigned char const *>(h0.data()),
														h0.size());
				assert(res == 0);

				res = crypto_generichash_blake2b_final(&state_,
													   reinterpret_cast<unsigned char *>(out.data()) + pos,
													   len);
				assert(res == 0);

				pos += len;
				remaining -= len;
			}
		}

		static void hash_single(std::span<std::byte const> data,
								std::span<std::byte, output_extent> out,
								std::span<std::byte const> key = {},
								std::span<std::byte const, salt_extent> salt = default_salt,
								std::span<std::byte const, personality_extent> personality = default_personality) /*noexcept(sodium is initialized && key is within size constraints)*/ {

			Blake2xb blake{out.size(), key, salt, personality};
			blake.digest(data);
			std::move(blake).finish(out);
		}
	};

} // namespace dice::hash::blake2xb

#endif//DICE_HASH_WITH_SODIUM
#endif//DICE_HASH_BLAKE2XB_HPP
