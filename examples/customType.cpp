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

	template <typename Policy>
	friend std::size_t Dice::hash::dice_hash_templates<Policy>::dice_hash(MyCustomClass const& t) noexcept;
public:
	MyCustomClass(int a, char b, std::string c)
	: a(a), b(b), c(std::move(c)) {}
};

namespace Dice::hash {
	/** Policy agnostic.
	 *
	 * @param t
	 * @return
	 */
	template <>
	std::size_t dice_hash_base::dice_hash(MyCustomStruct const& t) noexcept {
		return (t.a + t.b) * t.c.length();
	}

	/** For a specific Policy.
	 *
	 */
	 template <>
	 template <>
	 std::size_t dice_hash_templates<Dice::hash::Policies::Martinus>::dice_hash(MyCustomClass const& t) noexcept {
		 return dice_hash(std::make_tuple(t.a, t.b, t.c));
	 }
}// namespace Dice::hash


int main() {
	Dice::hash::DiceHash<MyCustomStruct> hashForMyCustomStruct;
    Dice::hash::DiceHash<MyCustomClass> hashForMyCustomClass;
	MyCustomStruct objS{42, 'c', "hello World!"};
    MyCustomClass  objC{42, 'c', "hello World!"};
	std::cout << "hashForMyCustomStruct(obj): " << hashForMyCustomStruct(objS) << '\n';
    std::cout << "hashForMyCustomStruct(obj): " << hashForMyCustomClass(objC) << '\n';
}
