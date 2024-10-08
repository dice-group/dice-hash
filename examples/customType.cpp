#include <dice/hash.hpp>
#include <iostream>
#include <string>

class MyCustomClass {
	int a;
	char b;
	std::string c;

public:
	MyCustomClass(int a, char b, std::string c)
		: a(a), b(b), c(std::move(c)) {}

	// With this, the DiceHash can work on private data
	template<dice::hash::Policies::HashPolicy, typename>
	friend struct dice::hash::dice_hash_overload;
};

namespace dice::hash {
	template<typename Policy>
	struct dice_hash_overload<Policy, MyCustomClass> {
		static std::size_t dice_hash(MyCustomClass const &s) noexcept {
			return dice_hash_templates<Policy>::dice_hash(std::make_tuple(s.a, s.b, s.c));
		}
	};
}// namespace dice::hash

int main() {
	MyCustomClass objC{42, 'c', "hello World!"};
	dice::hash::DiceHash<MyCustomClass> hasher;
	std::cout << "hasher(objC): " << hasher(objC) << '\n';
}