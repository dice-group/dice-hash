#include <Dice/hash/DiceHash.hpp>
#include <iostream>
#include <string>

struct MyCustomStruct {
	int a;
	char b;
	std::string c;
};

namespace Dice::hash {
	template<>
	std::size_t dice_hash(MyCustomStruct const &myCustomStruct) noexcept {
		return (dice_hash(myCustomStruct.a) + dice_hash(myCustomStruct.b)) * dice_hash(myCustomStruct.c);
	}
}// namespace Dice::hash


int main() {
	Dice::hash::DiceHash<MyCustomStruct> hashForMyCustomStruct;
	MyCustomStruct obj{42, 'c', "hello World!"};
	std::cout << "hashForMyCustomStruct(obj): " << hashForMyCustomStruct(obj) << '\n';
}
