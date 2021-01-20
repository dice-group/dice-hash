#include <Dice/hash/DiceHash.hpp>
#include <iostream>
#include <unordered_set>

int main() {
	std::unordered_set<int, Dice::hash::DiceHash<int>> set;
	for (int i = 0; i < 100; ++i) {
		set.insert(i);
	}
	for (auto i : set) {
		std::cout << i << '\t';
	}
	std::cout << '\n';
}