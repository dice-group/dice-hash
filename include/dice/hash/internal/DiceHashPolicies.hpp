#ifndef DICE_HASH_DICEHASHPOLICIES_HPP
#define DICE_HASH_DICEHASHPOLICIES_HPP

#include "martinus_robinhood_hash.hpp"
#include "wyhash.h"
// exposes the definition of XXH3_state_t, so the streaming state can be held by
// value instead of on the heap
#ifndef XXH_STATIC_LINKING_ONLY
#define XXH_STATIC_LINKING_ONLY
#endif
#include <xxhash.h>
#include <bit>
#include <type_traits>
#include "rapidhash.h"

namespace dice::hash::Policies {
    template<typename T>
    concept HashPolicy =
    std::is_convertible_v<decltype(T::ErrorValue), std::size_t>
    &&std::is_nothrow_invocable_r_v<std::size_t, decltype(T::template hash_fundamental<int>), int>
    &&std::is_nothrow_invocable_r_v<std::size_t, decltype(T::template hash_fundamental<long>), long>
    &&std::is_nothrow_invocable_r_v<std::size_t, decltype(T::template hash_fundamental<std::size_t>), std::size_t>
    &&std::is_nothrow_invocable_r_v<std::size_t, decltype(T::hash_bytes), void const *, std::size_t>
    &&std::is_nothrow_invocable_r_v<std::size_t, decltype(T::hash_combine), std::initializer_list<std::size_t>>
    &&std::is_nothrow_invocable_r_v<std::size_t, decltype(T::hash_invertible_combine), std::initializer_list<std::size_t>>
    &&std::is_nothrow_constructible_v<typename T::HashState, std::size_t>
    &&std::is_nothrow_invocable_r_v<void, decltype(&T::HashState::add), typename T::HashState &, std::size_t>
    &&std::is_nothrow_invocable_r_v<std::size_t, decltype(&T::HashState::digest), typename T::HashState &>;

	struct wyhash {
		inline static constexpr uint64_t kSeed = 0xe17a1465UL;
		inline static constexpr uint64_t kWyhashSalt[4] = {
				dice::hash::wyhash::_wyp[0],
				dice::hash::wyhash::_wyp[1],
				dice::hash::wyhash::_wyp[2],
				dice::hash::wyhash::_wyp[3]
		};
		inline static constexpr std::size_t ErrorValue = kSeed;

		template<typename T>
		static std::size_t hash_fundamental(T x) noexcept {
			if constexpr (std::is_integral_v<T>) {
				return static_cast<std::size_t>(dice::hash::wyhash::wyhash64(kSeed, x));
			}
			return static_cast<std::size_t>(dice::hash::wyhash::wyhash(&x, sizeof(T), kSeed, kWyhashSalt));
		}

		static std::size_t hash_bytes(void const *ptr, std::size_t len) noexcept {
			return static_cast<std::size_t>(dice::hash::wyhash::wyhash(ptr, len, kSeed, kWyhashSalt));
		}

		static std::size_t hash_combine(std::initializer_list<size_t> hashes) noexcept {
			uint64_t state = kSeed;
			for (auto hash : hashes) {
				state = dice::hash::wyhash::_wymix(state, hash);
			}
			return static_cast<std::size_t>(state);
		}

		static std::size_t hash_invertible_combine(std::initializer_list<size_t> hashes) noexcept {
			std::size_t result = 0;
			for (auto hash : hashes) {
				result = result xor hash;
			}
			return result;
		}

		class HashState {
		private:
			uint64_t state = kSeed;
		public:
			explicit HashState(std::size_t) noexcept {}
			void add (std::size_t hash) noexcept {
				state = dice::hash::wyhash::_wymix(state, static_cast<uint64_t>(hash));
			}
            [[nodiscard]] std::size_t digest() noexcept {
				return static_cast<std::size_t>(state);
			}
		};
	};

	struct xxh3 {
		inline static constexpr std::size_t size_t_bits = 8 * sizeof(std::size_t);
		inline static constexpr std::size_t seed = std::size_t(0xA24BAED4963EE407UL);
		inline static constexpr std::size_t ErrorValue = seed;

		template<typename T>
		static std::size_t hash_fundamental(T x) noexcept {
			return hash_bytes(&x, sizeof(x));
		}
		static std::size_t hash_bytes(void const *ptr, std::size_t len) noexcept {
			return static_cast<std::size_t>(XXH3_64bits_withSeed(ptr, len, seed));
		}
		static std::size_t hash_combine(std::initializer_list<std::size_t> hashes) noexcept {
			return hash_bytes(hashes.begin(), hashes.size() * sizeof(std::size_t));
		}
		static std::size_t hash_invertible_combine(std::initializer_list<size_t> hashes) noexcept {
			std::size_t result = 0;
			for (auto hash : hashes) {
				result = result xor hash;
			}
			return result;
		}
		class HashState {
		private:
			XXH3_state_t hash_state{};

		public:
			explicit HashState(std::size_t) noexcept {
				XXH3_64bits_reset_withSeed(&hash_state, seed);
			}

			void add(std::size_t hash) noexcept {
				XXH3_64bits_update(&hash_state, &hash, sizeof(std::size_t));
			}
			[[nodiscard]] std::size_t digest() noexcept {
				return static_cast<std::size_t>(XXH3_64bits_digest(&hash_state));
			}
		};
	};

	struct Martinus {
		static constexpr std::size_t ErrorValue = dice::hash::martinus::seed;
		template<typename T>
		static std::size_t hash_fundamental(T x) noexcept {
			if constexpr (sizeof(std::decay_t<T>) == sizeof(size_t)) {
				return dice::hash::martinus::hash_int(std::bit_cast<size_t>(x));
			} else if constexpr (sizeof(std::decay_t<T>) > sizeof(size_t) or std::is_floating_point_v<std::decay_t<T>>) {
				return hash_bytes(&x, sizeof(x));
			} else {
				return dice::hash::martinus::hash_int(static_cast<size_t>(x));
			}
		}
		static std::size_t hash_bytes(void const *ptr, std::size_t len) noexcept {
			return dice::hash::martinus::hash_bytes(ptr, len);
		}
		static std::size_t hash_combine(std::initializer_list<size_t> hashes) noexcept {
			return dice::hash::martinus::hash_combine(hashes);
		}
		static std::size_t hash_invertible_combine(std::initializer_list<size_t> hashes) noexcept {
			std::size_t result = 0;
			for (auto hash : hashes) {
				result = result xor hash;
			}
			return result;
		}
		class HashState {
		private:
			dice::hash::martinus::HashState state;

		public:
			explicit HashState(std::size_t size) noexcept : state(size) {}
			void add(std::size_t hash) noexcept {
				state.add(hash);
			}
			[[nodiscard]] std::size_t digest() noexcept {
				return state.digest();
			}
		};
	};

	struct rapidhash {
		inline static constexpr uint64_t kSeed = RAPID_SEED;
		inline static constexpr std::size_t ErrorValue = kSeed;

		template<typename T>
		static std::size_t hash_fundamental(T x) noexcept {
			return static_cast<std::size_t>(rapidhash_withSeed(&x, sizeof(T), kSeed));
		}

		static std::size_t hash_bytes(void const *ptr, std::size_t len) noexcept {
			return static_cast<std::size_t>(rapidhash_withSeed(ptr, len, kSeed));
		}

		static std::size_t hash_combine(std::initializer_list<size_t> hashes) noexcept {
			uint64_t state = kSeed;
			for (auto hash : hashes) {
				state = rapid_mix(state, hash);
			}
			return static_cast<std::size_t>(state);
		}

		static std::size_t hash_invertible_combine(std::initializer_list<size_t> hashes) noexcept {
			std::size_t result = 0;
			for (auto hash : hashes) {
				result = result ^ hash;
			}
			return result;
		}

		class HashState {
		private:
			uint64_t state = kSeed;
		public:
			explicit HashState(std::size_t) noexcept {}
			void add (std::size_t hash) noexcept {
				state = rapid_mix(state, static_cast<uint64_t>(hash));
			}
			[[nodiscard]] std::size_t digest() noexcept {
				return static_cast<std::size_t>(state);
			}
		};
	};
}// namespace dice::hash::Policies
#endif//DICE_HASH_DICEHASHPOLICIES_HPP
