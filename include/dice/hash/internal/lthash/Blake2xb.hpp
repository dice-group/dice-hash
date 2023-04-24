#ifndef DICE_HASH_BLAKE2XB_HPP
#define DICE_HASH_BLAKE2XB_HPP

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <span>
#include <vector>

#include <sodium.h>

namespace dice::hash::internal::blake2xb {

	/**
	 * @brief Blake2xb ported from folly::experimental::crypto
	 */
	struct Blake2xb {
		static constexpr size_t min_output_len = 1;
		static constexpr size_t max_output_len = 0xfffffffeULL;
		static constexpr size_t unknown_output_len = 0;
		static constexpr size_t salt_len = crypto_generichash_blake2b_SALTBYTES;
		static constexpr size_t personality_len = crypto_generichash_blake2b_PERSONALBYTES;

		static constexpr std::array<std::byte, salt_len> default_salt{};
		static constexpr std::array<std::byte, personality_len> default_personality{};

	private:
		static constexpr size_t unknown_output_len_magic = std::numeric_limits<uint32_t>::max();

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
			uint16_t rfu_head;
			uint32_t rfu_tail[3];
			uint32_t salt[4];
			uint32_t personality[4];
		};
		static_assert(sizeof(ParamBlock) == 64);

		ParamBlock param_{};
		crypto_generichash_blake2b_state state_;
		bool output_len_known_;

		void init_state(std::span<std::byte const> key);

	public:
		explicit Blake2xb(size_t output_len,
						  std::span<std::byte const> key = {},
						  std::span<std::byte const, salt_len> salt = default_salt,
						  std::span<std::byte const, personality_len> personality = default_personality);

		void digest(std::span<std::byte const> data) noexcept;

		void finish(std::span<std::byte> out) &&;
		[[nodiscard]] std::vector<std::byte> finish() && noexcept;

		static void hash_single(std::span<std::byte> out,
								std::span<std::byte const> data,
								std::span<std::byte const> key = {},
								std::span<std::byte const, salt_len> salt = default_salt,
								std::span<std::byte const, personality_len> personality = default_personality);

		[[nodiscard]] static std::vector<std::byte> hash_single(std::span<std::byte const> data,
																std::span<std::byte const> key = {},
																std::span<std::byte const, salt_len> salt = default_salt,
																std::span<std::byte const, personality_len> personality = default_personality) noexcept;
	};

} // namespace dice::hash::internal::blake2xb

#endif//DICE_HASH_BLAKE2XB_HPP
