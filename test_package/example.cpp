#include <dice/hash.hpp>
#include <dice/hash/blake/Blake3.hpp>
#include <iostream>

void print_bytes(std::span<std::byte const> bytes) noexcept {
	for (auto b : bytes) {
		std::cout << std::hex << static_cast<unsigned>(b);
	}
	std::cout << "\n\n";
}

int main() {
	std::cout << "wyhash(42): "
			  << dice::hash::DiceHash<int, dice::hash::Policies::wyhash>()(42)
			  << std::endl;
	std::cout << "blake3(42): ";
	{
		std::vector<std::byte> output;
		output.resize(58);
		std::array<std::byte, 1> data{static_cast<std::byte>(42)};
		dice::hash::blake3::Blake3<>::hash_single(data, output);

		print_bytes(output);
	}
	std::cout << std::endl;
}