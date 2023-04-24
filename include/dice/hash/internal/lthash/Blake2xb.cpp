#include "dice/hash/internal/lthash/Blake2xb.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstring>
#include <stdexcept>

namespace dice::hash::internal::blake2xb {

	template<typename T>
	inline static std::byte const *byte_iter(T const &value) noexcept {
		return reinterpret_cast<std::byte const *>(&value);
	}

	template<typename T>
	inline static std::byte *byte_iter_mut(T &value) noexcept {
		return reinterpret_cast<std::byte *>(&value);
	}

	template<typename T>
	inline static T little_endian(T const &value) noexcept {
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

	// In libsodium 1.0.17, the crypto_generichash_blake2b_state struct was made
	// opaque. We have to copy the internal definition of the real struct here
	// so we can properly initialize it.
	// see https://github.com/jedisct1/libsodium/blob/master/src/libsodium/crypto_generichash/blake2b/ref/blake2.h
	#if SODIUM_LIBRARY_VERSION_MAJOR > 10 || (SODIUM_LIBRARY_VERSION_MAJOR == 10 && SODIUM_LIBRARY_VERSION_MINOR >= 2)
	struct _blake2b_state {
		uint64_t h[8];
		uint64_t t[2];
		uint64_t f[2];
		uint8_t buf[256];
		size_t buflen;
		uint8_t last_node;
	};
	#define __LIBSODIUM_BLAKE2B_OPAQUE__ 1
	#endif

	static constexpr std::array<uint64_t, 8> blake2b_init_vec{
			0x6a09e667f3bcc908ULL,
			0xbb67ae8584caa73bULL,
			0x3c6ef372fe94f82bULL,
			0xa54ff53a5f1d36f1ULL,
			0x510e527fade682d1ULL,
			0x9b05688c2b3e6c1fULL,
			0x1f83d9abfb41bd6bULL,
			0x5be0cd19137e2179ULL};

	void Blake2xb::init_state(std::span<std::byte const> key) {
		{
#ifdef __LIBSODIUM_BLAKE2B_OPAQUE__
			auto *state = reinterpret_cast<_blake2b_state *>(&state_);
#else
			auto *state = &state_;
#endif

			auto p = reinterpret_cast<uint64_t const *>(&param_);
			for (size_t ix = 0; ix < blake2b_init_vec.size(); ++ix) {
				state->h[ix] = blake2b_init_vec[ix] ^ little_endian(p[ix]);
			}

			std::fill(byte_iter_mut(state) + sizeof(state->h), byte_iter_mut(state) + sizeof(*state), std::byte{0});
		}

		if (!key.empty()) {
			if (key.size() < crypto_generichash_blake2b_KEYBYTES_MIN || key.size() > crypto_generichash_blake2b_KEYBYTES_MAX) {
				throw std::runtime_error{"Invalid blake2b key size"};
			}

			std::array<std::byte, 128> block;
			auto write_end = std::copy(key.begin(), key.end(), block.begin());
			std::fill(write_end, block.end(), std::byte{0});

			crypto_generichash_blake2b_update(
					&state_,
					reinterpret_cast<std::underlying_type_t<std::byte> *>(block.data()),
					block.size());

			sodium_memzero(block.data(), block.size()); // erase key from stack
		}
	}

	Blake2xb::Blake2xb(size_t output_len,
					   std::span<std::byte const> key,
					   std::span<std::byte const, salt_len> salt,
					   std::span<std::byte const, personality_len> personality) {

		if (auto const res = sodium_init(); res != -1) {
			throw std::runtime_error{"Could not initialize sodium"};
		}

		if (output_len == unknown_output_len) {
			output_len_known_ = false;
			output_len = unknown_output_len_magic;
		} else if (output_len > max_output_len) {
			throw std::runtime_error{"Output length too large"};
		} else {
			output_len_known_ = true;
		}

		param_.digest_len = crypto_generichash_blake2b_BYTES_MAX;
		param_.key_len = static_cast<uint8_t>(key.size());
		param_.fanout = 1;
		param_.depth = 1;
		param_.xof_digest_len = little_endian(static_cast<uint32_t>(output_len));

		std::copy(salt.begin(), salt.end(), byte_iter_mut(param_.salt));
		std::copy(personality.begin(), personality.end(), byte_iter_mut(param_.personality));

		init_state(key);
	}

	void Blake2xb::digest(std::span<std::byte const> data) noexcept {
		auto const res = crypto_generichash_blake2b_update(&state_,
														   reinterpret_cast<std::underlying_type_t<std::byte> const *>(data.data()),
														   data.size());
		// cannot fail, see: https://github.com/jedisct1/libsodium/blob/8d9ab6cd764926d4bf1168b122f4a3ff4ea686a0/src/libsodium/crypto_generichash/blake2b/ref/blake2b-ref.c#L263
		assert(res == 0);
	}

	void Blake2xb::finish(std::span<std::byte> out) && {
		if (output_len_known_) {
			if (static_cast<uint32_t>(out.size()) != little_endian(param_.xof_digest_len)) [[unlikely]] {
				throw std::runtime_error{"out.size() must be equal to blake2 output len"};
			}
		}

		std::array<std::byte, crypto_generichash_blake2b_BYTES_MAX> h0;
		auto res = crypto_generichash_blake2b_final(&state_,
													reinterpret_cast<std::underlying_type_t<std::byte> *>(h0.data()),
													h0.size());
		// cannot fail on proper use, see: https://github.com/jedisct1/libsodium/blob/8d9ab6cd764926d4bf1168b122f4a3ff4ea686a0/src/libsodium/crypto_generichash/blake2b/ref/blake2b-ref.c#L299
		assert(res == 0);

		param_.key_len = 0;
		param_.fanout = 0;
		param_.depth = 0;
		param_.leaf_len = little_endian(static_cast<uint32_t>(crypto_generichash_blake2b_BYTES_MAX));
		param_.inner_len = crypto_generichash_blake2b_BYTES_MAX;

		size_t pos = 0;
		size_t remaining = param_.xof_digest_len;

		while (remaining > 0) {
			param_.node_off = little_endian(static_cast<uint32_t>(pos / crypto_generichash_blake2b_BYTES_MAX));

			size_t const len = std::min(static_cast<size_t>(crypto_generichash_blake2b_BYTES_MAX), remaining);
			param_.digest_len = static_cast<uint8_t>(len);

			init_state({});
			res = crypto_generichash_blake2b_update(&state_,
													reinterpret_cast<std::underlying_type_t<std::byte> *>(h0.data()),
													h0.size());
			assert(res == 0);

			res = crypto_generichash_blake2b_final(&state_,
												   reinterpret_cast<std::underlying_type_t<std::byte> *>(out.data()) + pos,
												   len);
			assert(res == 0);

			pos += len;
			remaining -= len;
		}
	}

	std::vector<std::byte> Blake2xb::finish() && noexcept {
		std::vector<std::byte> out;
		if (output_len_known_) {
			out.reserve(param_.xof_digest_len);
		}

		std::move(*this).finish(out);
		return out;
	}

	void Blake2xb::hash_single(std::span<std::byte> out,
							   std::span<std::byte const> data,
							   std::span<std::byte const> key,
							   std::span<std::byte const, salt_len> salt,
							   std::span<std::byte const, personality_len> personality) {
		Blake2xb blake{out.size(), key, salt, personality};
		blake.digest(data);
		std::move(blake).finish(out);
	}

	std::vector<std::byte> Blake2xb::hash_single(std::span<std::byte const> data, std::span<std::byte const> key, std::span<std::byte const, salt_len> salt, std::span<std::byte const, personality_len> personality) noexcept {
		Blake2xb blake{unknown_output_len, key, salt, personality};
		blake.digest(data);
		return std::move(blake).finish();
	}

} // namespace dice::hash::internal::blake2xb