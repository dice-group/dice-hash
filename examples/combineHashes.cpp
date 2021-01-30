#include <Dice/hash/DiceHash.hpp>
#include <iostream>
#include <string>

int main() {
	Dice::hash::DiceHash<int> hashInt;
	Dice::hash::DiceHash<std::string> hashString;
	auto first = hashInt(42);
	auto second = hashString("Hello World!");
	auto combination = hashInt.hash_combine({first, second});
	auto invertibleCombination = hashInt.hash_invertible_combine({first, second});
	std::cout << "first: " << first << "\nsecond: " << second << "\ncombination: " << combination << "\ninvertibleCombination: " << invertibleCombination << '\n';
}