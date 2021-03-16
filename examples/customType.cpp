#include <Dice/hash/DiceHash.hpp>
#include <iostream>
#include <string>

class MyCustomClass {
	int a;
	char b;
	std::string c;

public:
	MyCustomClass(int a, char b, std::string c)
		: a(a), b(b), c(std::move(c)) {}

	template<Dice::hash::Policies::HashPolicy, typename>
	friend class Dice::hash::dice_hash_overload;
};

namespace Dice::hash {
	template<typename Policy>
	struct dice_hash_overload<Policy, MyCustomClass> {
		static std::size_t dice_hash(MyCustomClass const &s) noexcept {
			return dice_hash_templates<Policy>::dice_hash(std::make_tuple(s.a, s.b, s.c));
		}
	};
}// namespace Dice::hash

int main() {
	MyCustomClass objC{42, 'c', "hello World!"};
	Dice::hash::DiceHash<MyCustomClass> hasher;
	std::cout << "hasher(objC): " << hasher(objC) << '\n';
}