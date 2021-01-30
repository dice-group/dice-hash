#ifndef DICE_HASH_DICEHASHPOLICIES_HPP
#define DICE_HASH_DICEHASHPOLICIES_HPP

#include "Dice/hash/martinus_robinhood_hash.hpp"
#include "Dice/hash/xxhash.hpp"

namespace Dice::hash::Policies {

	struct xxhash {
        inline static constexpr std::size_t size_t_bits = 8 * sizeof(std::size_t);
        inline static constexpr std::size_t seed = std::size_t(0xA24BAED4963EE407UL);
		static constexpr std::size_t ErrorValue = seed;

		template <typename T>
		static std::size_t hash_fundamental(T x) noexcept {
            return hash_bytes(&x, sizeof(x));
		}
		static std::size_t hash_bytes(void const *ptr, size_t len) noexcept {
            return xxh::xxhash3<size_t_bits>(ptr, len, seed);
		}
		static std::size_t hash_combine(std::initializer_list<std::size_t> hashes) noexcept {
            return xxh::xxhash3<size_t_bits>(hashes, seed);
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
			xxh::hash3_state64_t hash_state{seed};

		public:
			HashState(std::size_t) {}

		    void add(std::size_t hash)	noexcept {
				hash_state.update(&hash, sizeof(std::size_t));
			}
			std::size_t digest() noexcept {
				return hash_state.digest();
			}
		};
	};

	struct Martinus {
		static constexpr size_t ErrorValue = Dice::hash::martinus::seed;
		template<typename T>
		static std::size_t hash_fundamental(T x) noexcept {
			if constexpr (sizeof(std::decay_t<T>) == sizeof(size_t)) {
				return Dice::hash::martinus::hash_int(*reinterpret_cast<size_t const *>(&x));
			} else if constexpr (sizeof(std::decay_t<T>) > sizeof(size_t) or std::is_floating_point_v<std::decay_t<T>>) {
				return hash_bytes(&x, sizeof(x));
			} else {
				return Dice::hash::martinus::hash_int(static_cast<size_t>(x));
			}
		}
		static std::size_t hash_bytes(void const *ptr, size_t len) noexcept {
			return Dice::hash::martinus::hash_bytes(ptr, len);
		}
		static std::size_t hash_combine(std::initializer_list<size_t> hashes) noexcept {
			return Dice::hash::martinus::hash_combine(hashes);
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
			Dice::hash::martinus::HashState state;

		public:
			HashState(std::size_t size) : state(size) {}
			void add(std::size_t hash) noexcept {
				state.add(hash);
			}
			[[nodiscard]] std::size_t digest() noexcept {
				return state.digest();
			}
		};
	};
}// namespace Dice::hash::Policies
#endif//DICE_HASH_DICEHASHPOLICIES_HPP
