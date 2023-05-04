#include <catch2/catch_all.hpp>
#include <dice/hash/blake2/Blake2xb.hpp>

#include "TestBlake2xb_data.hpp"

using namespace dice::hash;

// integer value -> hexadecimal ascii representation (e.g 0 => '0', 10 => 'a')
static constexpr std::array<char, 16> encode_lut{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
												 'a', 'b', 'c', 'd', 'e', 'f'};

static char hex_encode(uint8_t const half_octet) noexcept {
	assert(half_octet <= 15);
	return encode_lut[half_octet];
}

std::string to_hex(std::span<std::byte const> bytes) noexcept {
	if (bytes.empty()) {
		return "";
	}

	std::string buf;
	for (auto const byte : bytes) {
		auto const lower = static_cast<uint8_t>(byte) & 0b1111;
		auto const higher = (static_cast<uint8_t>(byte) >> 4) & 0b1111;

		buf.push_back(hex_encode(higher));
		buf.push_back(hex_encode(lower));
	}

	return buf;
}

void check(std::span<std::byte> k, std::vector<std::string> const &data) {
	auto const &input = hash_input;

	for (auto const &expected_hex : data) {
		SECTION(std::string{k.empty() ? "keyless, " : "keyed, "} + "output len: " + std::to_string(expected_hex.size() / 2)) {
			std::vector<std::byte> actual;
			actual.resize(expected_hex.size() / 2);

			blake2xb::Blake2xb<>::hash_single(input, actual, k);
			std::string const actual_hex = to_hex(actual);

			CHECK(expected_hex == actual_hex);
		}
	}
}

/**
 * @note tests are adapted from: https://github.com/facebook/folly/blob/main/folly/experimental/crypto/test/Blake2xbTest.cpp
 */
TEST_CASE("Blake2xb", "[DiceHash]") {
	SECTION("keyless") {
		check({}, test_data);
	}

	SECTION("keyed") {
		check(key, keyed_test_data);
	}
}
