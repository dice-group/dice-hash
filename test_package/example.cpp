#include <dice/hash.hpp>
#include <iostream>

int main() {
	std::cout << "wyhash(42): "
			  << dice::hash::DiceHash<int, dice::hash::Policies::wyhash>()(42)
			  << std::endl;
}