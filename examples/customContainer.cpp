#include <Dice/hash/DiceHash.hpp>
#include <iostream>
#include <unordered_map>
#include <vector>

struct MyVector {
	std::vector<int> vec;
	auto begin() const noexcept {
		return vec.begin();
	}
	auto end() const noexcept {
		return vec.end();
	}
	auto size() const noexcept {
		return vec.size();
	}
};

struct MyMap {
	std::unordered_map<int, int> map;
	auto begin() const noexcept {
		return map.begin();
	}
	auto end() const noexcept {
		return map.end();
	}
	auto size() const noexcept {
		return map.size();
	}
};

/* DiceHash is already defined for generic (homogeneous) ordered and unordered container.
 * You simply need to tell it that your type is such a container.
 * This happens via specializing the type trait as shown below.
 * After that DiceHash can already be used on your type (as long as its content is hashable as well).
 */
namespace Dice::hash {
	template<>
	struct is_ordered_container<MyVector> : std::true_type {};
}// namespace Dice::hash

namespace Dice::hash {
	template<>
	struct is_unordered_container<MyMap> : std::true_type {};
}// namespace Dice::hash


int main() {
	MyVector vec{{1, 2, 3, 4, 5}};
	Dice::hash::DiceHash<MyVector> hashForCustomOrderedContainer;
	std::cout << "hashForCustomOrderedContainer(vec): " << hashForCustomOrderedContainer(vec) << '\n';
	MyMap map;
	map.map[0] = 0;
	map.map[1] = 1;
	map.map[2] = 2;
	map.map[42] = 42;
	Dice::hash::DiceHash<MyMap> hashForCustomUnorderedContainer;
	std::cout << "hashForCustomUnorderedContainer(map): " << hashForCustomUnorderedContainer(map) << '\n';
}