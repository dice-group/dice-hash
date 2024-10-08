#include <dice/hash/lthash/LtHash.hpp>

#include <cassert>
#include <iostream>
#include <string_view>
#include <vector>

void print_bytes(std::span<std::byte const> bytes) noexcept {
	for (auto b : bytes) {
		std::cout << std::hex << static_cast<unsigned>(b);
	}
	std::cout << "\n\n";
}

int main() {
	using namespace dice::hash::lthash;
	using namespace std::string_view_literals;

	auto obj1 = as_bytes(std::span<char const>{"spherical cow"sv});
	auto obj2 = as_bytes(std::span<char const>{"hello world"sv});

	LtHash16 lthash;
	print_bytes(lthash.checksum());

	lthash.add(obj1);
	std::vector<std::byte> const checksum1{lthash.checksum().begin(), lthash.checksum().end()};
	print_bytes(checksum1);

	lthash.add(obj2);
	std::vector<std::byte> const checksum2{lthash.checksum().begin(), lthash.checksum().end()};
	print_bytes(checksum2);

	assert(checksum1 != checksum2);

	lthash.remove(obj2);
	std::vector<std::byte> const checksum3{lthash.checksum().begin(), lthash.checksum().end()};
	print_bytes(checksum3);

	assert(checksum3 == checksum1);
}
