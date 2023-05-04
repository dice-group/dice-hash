#include <dice/hash/blake2/Blake2b.hpp>

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
	using namespace dice::hash::blake2b;

	auto data1 = as_bytes(std::span<char const>{"spherical cow"sv});
	auto data2 = as_bytes(std::span<char const>{"hello world"sv});
	auto data3 = as_bytes(std::span<char const>{"penguins"sv});

	{ // stateful hashing
		Blake2b blake{max_output_extent};
		blake.digest(data1);
		blake.digest(data2);

		std::vector<std::byte> output;
		output.resize(max_output_extent);

		std::move(blake).finish(output);

		print_bytes(output);
	}

	{ // one-off hashing
		std::vector<std::byte> output;
		output.resize(32);
		Blake2b<>::hash_single(data3, output);

		print_bytes(output);
	}
}
