#include <Dice/hash/DiceHash.hpp>
#include <iostream>
#include <string>

int main() {
	Dice::hash::DiceHash<int> hashForInt;
	std::cout << "hashForInt(42): " << hashForInt(42) << '\n';
	Dice::hash::DiceHash<std::string> hashForString;
	std::cout << "hashForString(\"Hello World!\"): " << hashForString("Hello World!") << '\n';
}