#include <dice/hash.hpp>
#include <iostream>
#include <string>

int main() {
	using HashInt = dice::hash::DiceHash<int>;
	HashInt hashInt;
	dice::hash::DiceHash<std::string> hashString;
	auto first = hashInt(42);
	auto second = hashString("Hello World!");
	// dice::hash::DiceHash<int>::hash_combine is actually static, but this is shorter:
	auto combination = HashInt::hash_combine({first, second});
	auto invertibleCombination = HashInt::hash_invertible_combine({first, second});
	std::cout << "first: " << first << "\nsecond: " << second << "\ncombination: " << combination << "\ninvertibleCombination: " << invertibleCombination << '\n';
}