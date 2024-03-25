#include <array>
#include <iostream>
#include <catch2/catch_all.hpp>

#include <dice/hash/blake/Blake2b.hpp>

using namespace dice::hash::blake2b;

void print_span(std::span<std::byte const> bytes) noexcept {
	for (auto const b : bytes) {
		std::cout << std::hex << static_cast<unsigned>(b);
	}
	std::cout << std::endl;
}

TEST_CASE("Blake2b", "[DiceHash]") {
	SECTION("keygen") {
		SECTION("static length") {
			std::array<std::byte, 24> key;
			generate_key(std::span{key});
			print_span(key);
		}

		SECTION("dynamic length") {
			std::vector<std::byte> key;
			key.resize(45);
			generate_key(std::span{key});
			print_span(key);
		}

		SECTION("too small") {
			std::array<std::byte, min_key_extent - 1> key;
			CHECK_THROWS(generate_key(std::span<std::byte>{key}));
		}

		SECTION("too big") {
			std::array<std::byte, max_key_extent + 1> key;
			CHECK_THROWS(generate_key(std::span<std::byte>{key}));
		}
	}

	SECTION("hash generation sanity check") {
		auto const data = as_bytes(std::span{"spherical cow"});

		std::array<std::byte, max_output_extent> output1;
		Blake2b<>::hash_single(data, output1);

		crypto_generichash_blake2b_state state;
		crypto_generichash_blake2b_init(&state, nullptr, 0, max_output_extent);
		crypto_generichash_blake2b_update(&state, reinterpret_cast<unsigned char const *>(data.data()), data.size());
		std::array<std::byte, max_output_extent> output2;
		crypto_generichash_blake2b_final(&state, reinterpret_cast<unsigned char *>(output2.data()), output2.size());

		CHECK(output1 == output2);
	}
}
