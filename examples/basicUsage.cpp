#include <dice/hash.hpp>
#include <iostream>
#include <string>

int main() {
	dice::hash::DiceHash<int> hashForInt;
	std::cout << "hashForInt(42): " << hashForInt(42) << '\n';
	dice::hash::DiceHash<std::string> hashForString;
	std::cout << "hashForString(\"Hello World!\"): " << hashForString("Hello World!") << '\n';
}