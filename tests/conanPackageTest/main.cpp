#include <Dice/hash/DiceHash.hpp>
#include <iostream>

int main() {
	std::cout << "wyhash(42): "
			  << Dice::hash::DiceHash<int, Dice::hash::Policies::wyhash>()(42)
			  << std::endl;
}