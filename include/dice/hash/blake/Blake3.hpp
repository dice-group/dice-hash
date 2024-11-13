#ifndef DICE_HASH_BLAKE3_HPP
#define DICE_HASH_BLAKE3_HPP

#include <algorithm>
#include <cstddef>
#include <span>
#include <limits>
#include <random>

#include <blake3.h>

namespace dice::hash::blake3 {

	inline constexpr size_t dynamic_output_extent = std::dynamic_extent;
	inline constexpr size_t min_key_extent = BLAKE3_KEY_LEN;
	inline constexpr size_t max_key_extent = BLAKE3_KEY_LEN;
	inline constexpr size_t default_key_extent = BLAKE3_KEY_LEN;

	/**
	 * @brief Generates a random key by filling key_out using std::random_device
	 */
	inline void generate_key(std::span<std::byte, default_key_extent> key_out) {
		using byte_utype = std::underlying_type_t<std::byte>;

		std::random_device rng;
		std::uniform_int_distribution<byte_utype> dist{std::numeric_limits<byte_utype>::min(), std::numeric_limits<byte_utype>::max()};

		std::generate(key_out.begin(), key_out.end(), [&]() {
			return static_cast<std::byte>(dist(rng));
		});
	}

	template<size_t OutputExtent = dynamic_output_extent>
	struct Blake3 {
		/**
		 * @brief if known at compile time, the size of the resulting hash, otherwise dynamic_output_extent
		 */
		static constexpr size_t output_extent = OutputExtent;

		static constexpr size_t min_key_extent = ::dice::hash::blake3::min_key_extent;
		static constexpr size_t max_key_extent = ::dice::hash::blake3::max_key_extent;
		static constexpr size_t default_key_extent = ::dice::hash::blake3::default_key_extent;

	private:
		blake3_hasher state_;

	public:
		Blake3() noexcept {
			blake3_hasher_init(&state_);
		}

		/**
		 * @brief Construct a BLAKE3 instance
		 * @param key a blake3 key
		 */
		explicit Blake3(std::span<std::byte const, default_key_extent> key) noexcept {
			blake3_hasher_init_keyed(&state_, reinterpret_cast<uint8_t const *>(key.data()));
		}

		/**
		 * @brief digests data into the underlying BLAKE2Xb state
		 */
		void digest(std::span<std::byte const> data) noexcept {
			blake3_hasher_update(&state_, data.data(), data.size());
		}

		/**
		 * @brief produces the hash corresponding to the previously digested bytes
		 * @param out location to write the hash to, if output_extent == dynamic_output_extent and the output length was specified on construction
		 * 			the length of the span has to match the previously provided length
		 */
		void finish(std::span<std::byte, output_extent> out) && noexcept {
			blake3_hasher_finalize(&state_, reinterpret_cast<uint8_t *>(out.data()), out.size());
		}

		/**
		 * @brief convenience function to hash a single byte-span
		 */
		static void hash_single(std::span<std::byte const> data,
								std::span<std::byte, output_extent> out) noexcept {
			Blake3 blake;
			blake.digest(data);
			std::move(blake).finish(out);
		}

		/**
		 * @brief convenience function to hash a single byte-span
		 */
		static void hash_single(std::span<std::byte const> data,
								std::span<std::byte, output_extent> out,
								std::span<std::byte const, default_key_extent> key) noexcept {
			Blake3 blake{key};
			blake.digest(data);
			std::move(blake).finish(out);
		}
	};

} // namespace dice::hash::blake3

#endif//DICE_HASH_BLAKE3_HPP
