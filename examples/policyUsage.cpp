#include <dice/hash.hpp>
#include <iostream>
// not needed. It is included into DiceHash.hpp
//#include <dice/hash/DiceHashPolicies.hpp>


int main() {
	std::cout << "DiceHash<int,...>()(42):\n"
			  << "      xxh3 policy: " << dice::hash::DiceHash<int, dice::hash::Policies::xxh3>()(42) << '\n'
			  << "  martinus policy: " << dice::hash::DiceHash<int, dice::hash::Policies::Martinus>()(42) << '\n'
			  << "    wyhash policy: " << dice::hash::DiceHash<int, dice::hash::Policies::wyhash>()(42) << std::endl;
}