#ifndef DICE_HASH_BLAKE2B_HPP
#define DICE_HASH_BLAKE2B_HPP

/**
 * @brief A thin wrapper around libsodium's Blake2b implementation
 */

#if __has_include(<sodium.h>)

#include <sodium.h>

#include <algorithm>
#include <cassert>
#include <random>
#include <span>
#include <stdexcept>

namespace dice::hash::blake2b {

	inline constexpr size_t min_output_extent = crypto_generichash_blake2b_BYTES_MIN;
	inline constexpr size_t max_output_extent = crypto_generichash_blake2b_BYTES_MAX;
	inline constexpr size_t default_output_extent = crypto_generichash_blake2b_BYTES;
	inline constexpr size_t dynamic_output_extent = std::dynamic_extent;

	inline constexpr size_t salt_extent = crypto_generichash_blake2b_SALTBYTES;
	inline constexpr std::array<std::byte, salt_extent> default_salt{};

	inline constexpr size_t personality_extent = crypto_generichash_blake2b_PERSONALBYTES;
	inline constexpr std::array<std::byte, personality_extent> default_personality{};

	inline constexpr size_t min_key_extent = crypto_generichash_blake2b_KEYBYTES_MIN;
	inline constexpr size_t max_key_extent = crypto_generichash_blake2b_KEYBYTES_MAX;
	inline constexpr size_t default_key_extent = crypto_generichash_blake2b_KEYBYTES;

	/**
	 * @brief Generates a random key by filling key_out using std::random_device
	 */
	template<size_t KeyExtent>
		requires (KeyExtent == std::dynamic_extent || (KeyExtent >= min_key_extent && KeyExtent <= max_key_extent))
	void generate_key(std::span<std::byte, KeyExtent> key_out) {
		if constexpr (KeyExtent == std::dynamic_extent) {
			if (key_out.size() < min_key_extent || key_out.size() > max_key_extent) {
				throw std::runtime_error{"Invalid blake2b key size"};
			}
		}

		using byte_utype = std::underlying_type_t<std::byte>;

		std::random_device rng;
		std::uniform_int_distribution<byte_utype> dist{std::numeric_limits<byte_utype>::min(), std::numeric_limits<byte_utype>::max()};

		std::generate(key_out.begin(), key_out.end(), [&]() {
			return static_cast<std::byte>(dist(rng));
		});
	}

	namespace detail {
		template<size_t InnerOutputExtent>
		struct Blake2bInner {
			crypto_generichash_blake2b_state state_;
		};

		template<>
		struct Blake2bInner<dynamic_output_extent> {
			crypto_generichash_blake2b_state state_;
			size_t specified_output_len_;
		};
	} // namespace detail

	template<size_t OutputExtent = dynamic_output_extent>
		requires (OutputExtent == dynamic_output_extent || (OutputExtent >= min_output_extent && OutputExtent <= max_output_extent))
	struct Blake2b {
		/**
		 * @brief if known at compile time, the size of the resulting hash, otherwise dynamic_output_extent
		 */
		static constexpr size_t output_extent = OutputExtent;

	private:
		detail::Blake2bInner<output_extent> inner_;

		void init(size_t output_len,
				  std::span<std::byte const> key,
				  std::span<std::byte const, salt_extent> salt,
				  std::span<std::byte const, personality_extent> personality) {

			if (output_len < min_output_extent || output_len > max_output_extent) {
				throw std::runtime_error{"Invalid blake2b output size"};
			}

			if (!key.empty()) {
				if (key.size() < min_key_extent || key.size() > max_key_extent) {
					throw std::runtime_error{"Invalid blake2b key size"};
				}
			}

			if (auto const res = sodium_init(); res == -1) {
				throw std::runtime_error{"Could not initialize sodium"};
			}

			if constexpr (output_extent == dynamic_output_extent) {
				inner_.specified_output_len_ = output_len;
			}

			auto const res = crypto_generichash_blake2b_init_salt_personal(&inner_.state_,
																		   reinterpret_cast<unsigned char const *>(key.data()),
																		   key.size(),
																		   output_len,
																		   reinterpret_cast<unsigned char const *>(salt.data()),
																		   reinterpret_cast<unsigned char const *>(personality.data()));
			// cannot fail here, all invariants have been checked,
			// see: https://github.com/jedisct1/libsodium/blob/d787d2b1cf13ad2e69c3a7ebc3fb7b68b6430774/src/libsodium/crypto_generichash/blake2b/ref/generichash_blake2b.c#LL69C47-L69C47
			// and: https://github.com/jedisct1/libsodium/blob/d787d2b1cf13ad2e69c3a7ebc3fb7b68b6430774/src/libsodium/crypto_generichash/blake2b/ref/blake2b-ref.c#L148
			// and: https://github.com/jedisct1/libsodium/blob/d787d2b1cf13ad2e69c3a7ebc3fb7b68b6430774/src/libsodium/crypto_generichash/blake2b/ref/blake2b-ref.c#L216
			assert(res == 0);
		}

	public:
		/**
		 * @brief Construct a BLAKE2b instance
		 * @param output_len either a dynamically determined concrete length (>= min_output_extent && <= max_output_extent)
		 * @param key optionally a key with a length (>= min_key_length && <= max_key_length)
		 * @param salt BLAKE2b salt
		 * @param personality BLAKE2b personality
		 */
		explicit Blake2b(size_t output_len,
						 std::span<std::byte const> key = {},
						 std::span<std::byte const, salt_extent> salt = default_salt,
						 std::span<std::byte const, personality_extent> personality = default_personality) /*noexcept(sodium is initialized && output_len is within size constraints && key.size() is within size constaints)*/
			requires (output_extent == dynamic_output_extent) {
			init(output_len, key, salt, personality);
		}

		/**
		 * @brief Constructs a BLAKE2b instance either using a statically determined output length of output_extent
		 * @param key optionally a key with a length (>= min_key_length && <= max_key_length)
		 * @param salt BLAKE2b salt
		 * @param personality BLAKE2b personality
		 */
		explicit Blake2b(std::span<std::byte const> key = {},
						 std::span<std::byte const, salt_extent> salt = default_salt,
						 std::span<std::byte const, personality_extent> personality = default_personality) /*noexcept(sodium is initialized && key.size() is within size constraints)*/
			requires (output_extent != dynamic_output_extent) {
			init(output_extent, key, salt, personality);
		}

		/**
		 * @brief digests data into the underlying BLAKE2b state
		 */
		void digest(std::span<std::byte const> data) noexcept {
			auto const res = crypto_generichash_blake2b_update(&inner_.state_,
															   reinterpret_cast<unsigned char const *>(data.data()),
															   data.size());
			// cannot fail, see: https://github.com/jedisct1/libsodium/blob/8d9ab6cd764926d4bf1168b122f4a3ff4ea686a0/src/libsodium/crypto_generichash/blake2b/ref/blake2b-ref.c#L263
			assert(res == 0);
		}

		/**
		 * @brief produces the hash corresponding to the previously digested bytes
		 * @param out location to write the hash to, if output_extent == dynamic_output_extent and the output length was specified on construction
		 * 			the length of the span has to match the previously provided length
		 */
		void finish(std::span<std::byte, output_extent> out) && noexcept(output_extent != dynamic_output_extent /*|| output is within size constraints*/) {
			if constexpr (output_extent == dynamic_output_extent) {
				auto const expected_out_len = inner_.specified_output_len_;
				if (expected_out_len != 0 && out.size() != expected_out_len) {
					// was known in advance and does not match
					throw std::runtime_error{"Buffer length must match output length"};
				}
			}

			auto const res = crypto_generichash_blake2b_final(&inner_.state_,
															  reinterpret_cast<unsigned char *>(out.data()),
															  out.size());
			// cannot fail, all invariants have been checked, see: https://github.com/jedisct1/libsodium/blob/d787d2b1cf13ad2e69c3a7ebc3fb7b68b6430774/src/libsodium/crypto_generichash/blake2b/ref/blake2b-ref.c#L292
			assert(res == 0);
		}

		/**
		 * @brief returns the possibly runtime-determined output length or the underlying BLAKE2b
		 * @note different to Blake2b::output_extent this also considers values provided to the constructor at runtime.
		 * @return required length for output buffer, if known otherwise unknown_output_extent
		 */
		[[nodiscard]] constexpr size_t concrete_output_extent() const noexcept {
			if constexpr (output_extent == dynamic_output_extent) {
				return inner_.specified_output_len_;
			} else {
				return output_extent;
			}
		}

		/**
		 * @brief convenience function to hash a single byte-span
		 */
		static void hash_single(std::span<std::byte const> data,
								std::span<std::byte, output_extent> out,
								std::span<std::byte const> key = {},
								std::span<std::byte const, salt_extent> salt = default_salt,
								std::span<std::byte const, personality_extent> personality = default_personality) /*noexcept(sodium is initialized && output is within size constraints && key is within size constraints)*/ {
			auto blake = [&]() {
				if constexpr (output_extent == dynamic_output_extent) {
					return Blake2b{out.size(), key, salt, personality};
				} else {
					return Blake2b{key, salt, personality};
				}
			}();

			blake.digest(data);
			std::move(blake).finish(out);
		}
	};

} // namespace dice::hash::blake2b

#else
#error "Cannot include Blake2b.hpp if sodium is not available"
#endif

#endif//DICE_HASH_BLAKE2B_HPP
