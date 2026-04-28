#ifndef DICE_HASH_DICEHASHPOLICIES_HPP
#define DICE_HASH_DICEHASHPOLICIES_HPP

#include "martinus_robinhood_hash.hpp"
#include "wyhash.h"
#ifdef __x86_64__
#include "xxhash.hpp"
#endif
#include <type_traits>
#include "rapidhash.h"

namespace dice::hash::Policies {
    template<typename T>
    concept HashPolicy =
    std::is_convertible_v<decltype(T::ErrorValue), std::size_t>
	&& std::is_convertible_v<decltype(T::SeedValue), std::size_t>
    && std::is_nothrow_invocable_r_v<std::size_t, decltype(T::template hash_fundamental<int>), int, std::size_t>
    && std::is_nothrow_invocable_r_v<std::size_t, decltype(T::template hash_fundamental<long>), long, std::size_t>
    && std::is_nothrow_invocable_r_v<std::size_t, decltype(T::template hash_fundamental<std::size_t>), std::size_t, std::size_t>
    && std::is_nothrow_invocable_r_v<std::size_t, decltype(T::hash_bytes), void const *, std::size_t, std::size_t>
    && std::is_nothrow_invocable_r_v<std::size_t, decltype(T::hash_combine), std::initializer_list<std::size_t>, std::size_t>
    && std::is_nothrow_invocable_r_v<std::size_t, decltype(T::hash_invertible_combine), std::initializer_list<std::size_t>, std::size_t>
    && std::is_nothrow_constructible_v<typename T::HashState, std::size_t, std::size_t>
    && std::is_nothrow_invocable_r_v<void, decltype(&T::HashState::add), typename T::HashState &, std::size_t>
    && std::is_nothrow_invocable_r_v<std::size_t, decltype(&T::HashState::digest), typename T::HashState &>;

	struct wyhash {
		static constexpr uint64_t kWyhashSalt[4] = {
				dice::hash::wyhash::_wyp[0],
				dice::hash::wyhash::_wyp[1],
				dice::hash::wyhash::_wyp[2],
				dice::hash::wyhash::_wyp[3]
		};

		static constexpr uint64_t SeedValue = 0xe17a1465UL;
		static constexpr std::size_t ErrorValue = SeedValue;

		template<typename T>
		static std::size_t hash_fundamental(T x, std::size_t seed) noexcept {
			if constexpr (std::is_integral_v<T>) {
				return static_cast<std::size_t>(dice::hash::wyhash::wyhash64(seed, x));
			}
			return static_cast<std::size_t>(dice::hash::wyhash::wyhash(&x, sizeof(T), seed, kWyhashSalt));
		}

		static std::size_t hash_bytes(void const *ptr, std::size_t len, std::size_t seed) noexcept {
			return static_cast<std::size_t>(dice::hash::wyhash::wyhash(ptr, len, seed, kWyhashSalt));
		}

		static std::size_t hash_combine(std::initializer_list<size_t> hashes, std::size_t seed) noexcept {
			uint64_t state = seed;
			for (auto hash : hashes) {
				state = dice::hash::wyhash::_wymix(state, hash);
			}
			return static_cast<std::size_t>(state);
		}

		static std::size_t hash_invertible_combine(std::initializer_list<size_t> hashes, std::size_t seed) noexcept {
			std::size_t result = seed;
			for (auto hash : hashes) {
				result = result xor hash;
			}
			return result;
		}

		class HashState {
		private:
			uint64_t state;
		public:
			explicit HashState([[maybe_unused]] std::size_t size, std::size_t seed) noexcept
				: state{seed} {
			}

			void add (std::size_t hash) noexcept {
				state = dice::hash::wyhash::_wymix(state, static_cast<uint64_t>(hash));
			}
            [[nodiscard]] std::size_t digest() noexcept {
				return static_cast<std::size_t>(state);
			}
		};
	};

#ifdef __x86_64__
	struct xxh3 {
		inline static constexpr std::size_t size_t_bits = 8 * sizeof(std::size_t);
		inline static constexpr std::size_t SeedValue = std::size_t(0xA24BAED4963EE407UL);
		inline static constexpr std::size_t ErrorValue = SeedValue;

		template<typename T>
		static std::size_t hash_fundamental(T x, std::size_t seed) noexcept {
			return hash_bytes(&x, sizeof(x), seed);
		}
		static std::size_t hash_bytes(void const *ptr, std::size_t len, std::size_t seed) noexcept {
			return xxh::xxhash3<size_t_bits>(ptr, len, seed);
		}
		static std::size_t hash_combine(std::initializer_list<std::size_t> hashes, std::size_t seed) noexcept {
			return xxh::xxhash3<size_t_bits>(hashes, seed);
		}
		static std::size_t hash_invertible_combine(std::initializer_list<size_t> hashes, std::size_t seed) noexcept {
			std::size_t result = seed;
			for (auto hash : hashes) {
				result = result xor hash;
			}
			return result;
		}
		class HashState {
		private:
			xxh::hash3_state64_t hash_state;

		public:
            explicit HashState([[maybe_unused]] std::size_t size, std::size_t seed) noexcept
				: hash_state{seed} {
            }

			void add(std::size_t hash) noexcept {
				hash_state.update(&hash, sizeof(std::size_t));
			}
            [[nodiscard]] std::size_t digest() noexcept {
				return hash_state.digest();
			}
		};
	};
#endif

	struct Martinus {
		static constexpr std::size_t SeedValue = dice::hash::martinus::seed;
		static constexpr std::size_t ErrorValue = SeedValue;

		template<typename T>
		static std::size_t hash_fundamental(T x, std::size_t seed) noexcept {
			return hash_bytes(&x, sizeof(x), seed);
		}
		static std::size_t hash_bytes(void const *ptr, std::size_t len, std::size_t seed) noexcept {
			return dice::hash::martinus::hash_bytes(ptr, len, seed);
		}
		static std::size_t hash_combine(std::initializer_list<size_t> hashes, std::size_t seed) noexcept {
			return dice::hash::martinus::hash_combine(hashes, seed);
		}
		static std::size_t hash_invertible_combine(std::initializer_list<size_t> hashes, std::size_t seed) noexcept {
			std::size_t result = seed;
			for (auto hash : hashes) {
				result = result xor hash;
			}
			return result;
		}
		class HashState {
		private:
			dice::hash::martinus::HashState state;

		public:
			HashState(std::size_t size, std::size_t seed) noexcept
				: state{size, seed} {
			}
			void add(std::size_t hash) noexcept {
				state.add(hash);
			}
			[[nodiscard]] std::size_t digest() noexcept {
				return state.digest();
			}
		};
	};

	struct rapidhash {
		inline static constexpr uint64_t SeedValue = RAPID_SEED;
		inline static constexpr std::size_t ErrorValue = SeedValue;

		template<typename T>
		static std::size_t hash_fundamental(T x, std::size_t seed) noexcept {
			return static_cast<std::size_t>(rapidhash_withSeed(&x, sizeof(T), seed));
		}

		static std::size_t hash_bytes(void const *ptr, std::size_t len, std::size_t seed) noexcept {
			return static_cast<std::size_t>(rapidhash_withSeed(ptr, len, seed));
		}

		static std::size_t hash_combine(std::initializer_list<size_t> hashes, std::size_t seed) noexcept {
			uint64_t state = seed;
			for (auto hash : hashes) {
				state = rapid_mix(state, hash);
			}
			return static_cast<std::size_t>(state);
		}

		static std::size_t hash_invertible_combine(std::initializer_list<size_t> hashes, std::size_t seed) noexcept {
			std::size_t result = seed;
			for (auto hash : hashes) {
				result = result ^ hash;
			}
			return result;
		}

		class HashState {
		private:
			uint64_t state;
		public:
			explicit HashState([[maybe_unused]] std::size_t size, std::size_t seed) noexcept
				: state{seed} {
			}
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
