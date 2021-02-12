#include <Dice/hash/DiceHash.hpp>
#include <iostream>
// not needed. It is included into DiceHash.hpp
//#include <Dice/hash/DiceHashPolicies.hpp>


int main() {
	std::cout << "DiceHash<int,...>()(42):\n"
			  << "    xxhash policy: " << Dice::hash::DiceHash<int, Dice::hash::Policies::xxhash>()(42) << '\n'
			  << "  martinus policy: " << Dice::hash::DiceHash<int, Dice::hash::Policies::Martinus>()(42) << '\n'
			  << "    wyhash policy: " << Dice::hash::DiceHash<int, Dice::hash::Policies::wyhash>()(42) << std::endl;
}