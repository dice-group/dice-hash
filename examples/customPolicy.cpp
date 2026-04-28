#include <dice/hash.hpp>
#include <initializer_list>
#include <iostream>
#include <numeric>


struct MyCustomPolicy {
	// needed for bad_variant_access
	static constexpr std::size_t SeedValue = 42;
	static constexpr std::size_t ErrorValue = SeedValue;
	template<typename T>
	static std::size_t hash_fundamental(T x, std::size_t seed) noexcept {
		return static_cast<std::size_t>(seed * x);
	}
	static std::size_t hash_bytes([[maybe_unused]] void const *ptr, std::size_t len, [[maybe_unused]] std::size_t seed) noexcept {
		return len;
	}
	static std::size_t hash_combine(std::initializer_list<std::size_t> hashes, [[maybe_unused]] std::size_t seed) noexcept {
		return std::accumulate(hashes.begin(), hashes.end(), 0, [](auto sum, auto x) { return sum xor x; });
	}
    static std::size_t hash_invertible_combine(std::initializer_list<std::size_t> hashes, [[maybe_unused]] std::size_t seed) noexcept {
        return std::accumulate(hashes.begin(), hashes.end(), 0, [](auto sum, auto x) { return sum xor x; });
    }

	class HashState {
	private:
		std::size_t result = 0;
		//some Hashstates need to know how many elements need to be hashed
	public:
		HashState([[maybe_unused]] std::size_t size, std::size_t seed) noexcept
			: result{seed} {
		}
		void add(std::size_t hash) noexcept {
			result = result xor hash;
		}
		[[nodiscard]] constexpr std::size_t digest() const noexcept {
			return result;
		}
	};
};

struct NotWorkingPolicy {};


int main() {
	std::cout << dice::hash::DiceHash<int, MyCustomPolicy>()(42) << std::endl;
    //std::cout << dice::hash::DiceHash<int, NotWorkingPolicy>()(42) << std::endl;
}