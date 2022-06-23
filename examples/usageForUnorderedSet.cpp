#include <dice/hash.hpp>
#include <iostream>
#include <unordered_set>

int main() {
    std::unordered_set<int> setDefault;
	std::unordered_set<int, dice::hash::DiceHash<int>> setDice;
	for (int i = 0; i < 100; ++i) {
		setDefault.insert(i);
        setDice.insert(i);
	}
    std::cout << "Content of the unordered set with std::hash:\n";
    for (auto i : setDefault) {
        std::cout << i << '\t';
    }
	std::cout << "\nContent of the unordered set with DiceHash:\n";
	for (auto i : setDice) {
		std::cout << i << '\t';
	}
	std::cout << '\n';
}