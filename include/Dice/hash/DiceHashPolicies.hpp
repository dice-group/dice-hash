#ifndef DICE_HASH_DICEHASHPOLICIES_HPP
#define DICE_HASH_DICEHASHPOLICIES_HPP

namespace Dice::hash::Policies {

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
