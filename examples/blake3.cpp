#include <dice/hash/blake/Blake3.hpp>

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
	using namespace std::string_view_literals;
	using namespace dice::hash::blake3;

	auto data1 = as_bytes(std::span<char const>{"spherical cow"sv});
	auto data2 = as_bytes(std::span<char const>{"hello world"sv});
	auto data3 = as_bytes(std::span<char const>{"penguins"sv});

	{ // stateful hashing
		Blake3 blake;
		blake.digest(data1);
		blake.digest(data2);

		std::vector<std::byte> output;
		output.resize(789);

		std::move(blake).finish(output);

		print_bytes(output);
	}

	{ // one-off hashing
		std::vector<std::byte> output;
		output.resize(58);
		Blake3<>::hash_single(data3, output);

		print_bytes(output);
	}
}
