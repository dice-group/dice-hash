#include <Dice/hash/DiceHash.hpp>
#include <iostream>
#include <string>

int main() {
	using HashInt = Dice::hash::DiceHash<int>;
	HashInt hashInt;
	Dice::hash::DiceHash<std::string> hashString;
	auto first = hashInt(42);
	auto second = hashString("Hello World!");
	// Dice::hash::DiceHash<int>::hash_combine is actually static, but this is shorter:
	auto combination = HashInt::hash_combine({first, second});
	auto invertibleCombination = HashInt::hash_invertible_combine({first, second});
	std::cout << "first: " << first << "\nsecond: " << second << "\ncombination: " << combination << "\ninvertibleCombination: " << invertibleCombination << '\n';
}