#include <Dice/hash/DiceHash.hpp>
#include <iostream>
#include <string>

struct MyCustomStruct {
	int a;
	char b;
	std::string c;
};

class MyCustomClass {
	int a;
	char b;
	std::string c;
public:
	MyCustomClass(int a, char b, std::string c)
	: a(a), b(b), c(std::move(c)) {}

	template <typename Policy, typename T>
	friend std::size_t Dice::hash::dice_hash_overload<Policy, T>::dice_hash(T const&) noexcept;
};

namespace Dice::hash {
	template <typename Policy>
	struct dice_hash_overload<Policy, MyCustomStruct> {
		static std::size_t dice_hash(MyCustomStruct const& s) noexcept {
			return dice_hash_templates<Policy>::dice_hash(std::make_tuple(s.a, s.b, s.c));
		}
	};

	template <typename Policy>
	struct dice_hash_overload<Policy, MyCustomClass> {
		static std::size_t dice_hash(MyCustomClass const& s) noexcept{
            return dice_hash_templates<Policy>::dice_hash(std::make_tuple(s.a, s.b, s.c));
		}
	};
}// namespace Dice::hash

template <typename T>
std::size_t getHash(T const& t) {
	Dice::hash::DiceHash<T> hasher;
	return hasher(t);
}

int main() {
	MyCustomStruct objS{42, 'c', "hello World!"};
    MyCustomClass  objC{42, 'c', "hello World!"};
	std::cout << "getHash(objS): " << getHash(objS) << '\n';
    std::cout << "getHash(objC): " << getHash(objC) << '\n';
}