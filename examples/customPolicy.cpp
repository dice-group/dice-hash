#include <Dice/hash/DiceHash.hpp>
#include <initializer_list>
#include <iostream>
#include <numeric>


struct MyCustomPolicy {
	// needed for bad_variant_access
	inline static constexpr std::size_t ErrorValue = 42;
	template<typename T>
	static std::size_t hash_fundamental(T x) noexcept {
		return static_cast<std::size_t>(42 * x);
	}
	static std::size_t hash_bytes(void const *ptr, std::size_t len) noexcept {
		return len;
	}
	static std::size_t hash_combine(std::initializer_list<std::size_t> hashes) noexcept {
		return std::accumulate(hashes.begin(), hashes.end(), 0, [](auto sum, auto x) { return sum xor x; });
	}
    static std::size_t hash_invertible_combine(std::initializer_list<std::size_t> hashes) noexcept {
        return std::accumulate(hashes.begin(), hashes.end(), 0, [](auto sum, auto x) { return sum xor x; });
    }

	class HashState {
	private:
		std::size_t result = 0;
		//some Hashstates need to know how many elements need to be hashed
	public:
		explicit HashState(std::size_t size) noexcept {}
		void add(std::size_t hash) noexcept {
			result = result xor hash;
		}
		std::size_t digest() noexcept {
			return result;
		}
	};
};

struct NotWorklingPolicy {};


int main() {
	std::cout << Dice::hash::DiceHash<int, MyCustomPolicy>()(42) << std::endl;
    //std::cout << Dice::hash::DiceHash<int, NotWorklingPolicy>()(42) << std::endl;
}