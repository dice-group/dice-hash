#include <catch2/catch_all.hpp>

#include <dice/hash.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace dice::tests::hash::avalanche {
	/** A type with its own `dice_hash_overload`, which hashes the member. */
	struct Id {
		std::uint64_t value;
	};
}// namespace dice::tests::hash::avalanche

namespace dice::hash {
	/** The result is the hash of a `std::uint64_t`. So the overload declares `is_avalanching` as
	 * `avalanching_like<std::uint64_t, Policy>`, which is true exactly for the policies that mark
	 * `DiceHash<std::uint64_t, Policy>`.
	 */
	template<typename Policy>
	struct dice_hash_overload<Policy, dice::tests::hash::avalanche::Id> {
		using is_avalanching = avalanching_like<std::uint64_t, Policy>;
		static std::size_t dice_hash(dice::tests::hash::avalanche::Id const &id) noexcept {
			return dice_hash_templates<Policy>::dice_hash(id.value);
		}
	};
}// namespace dice::hash

/** Statistical checks of the member type `is_avalanching` of `DiceHash`.
 *
 * The test flips each bit of an input, hashes the input again, and counts for each pair of input
 * bit and output bit how often the output bit flips. For an avalanching hash each of these flip
 * probabilities is about one half.
 *
 * Inputs: inputs of up to 16 bits use every value. Larger inputs use 20000 random values from a
 * `std::mt19937_64` with a fixed seed, so the test is deterministic.
 *
 * Bound: a flip probability is estimated from `pairs` independent pairs of inputs, so the standard
 * deviation of the estimate is 0.5 / sqrt(pairs). The bound is 6 standard deviations, and at least
 * 0.025. For 20000 random inputs this is 0.025, which is 7 standard deviations (0.0035). The chance
 * that a true probability of one half lands outside is below 3e-12 per pair of bits. The rows with
 * random inputs have up to about 140000 pairs of bits per policy, so there a hash that avalanches
 * fails with a chance below 1e-6, also with another seed. Types with 8 bits have 128 pairs of inputs
 * per input bit, so their bound is 0.265. Types with 16 bits have 32768 pairs, so their bound is
 * 0.025. These rows use every value, so they do not depend on the seed.
 *
 * `bool` is not in the list: it has a single bit, so each output bit flips with a probability of
 * either 0 or 1.
 */
namespace dice::tests::hash::avalanche {
	using dice::hash::DiceHash;
	using dice::hash::Policies::Martinus;
	using dice::hash::Policies::rapidhash;
	using dice::hash::Policies::wyhash;
	using dice::hash::Policies::xxh3;

	template<typename T, typename Policy>
	inline constexpr bool marked = requires { typename DiceHash<T, Policy>::is_avalanching; };

	inline constexpr std::size_t random_inputs = 20000;
	inline constexpr std::uint64_t seed = 0x5eed;

	/** The pair of input bit and output bit whose flip probability is farthest from one half. */
	struct Worst {
		std::size_t input_bit = 0;
		std::size_t output_bit = 0;
		double probability = 0.5;
		double distance = 0.0;
		double bound = 0.0;
	};

	/** Flips each input bit and counts the flips of each output bit.
	 * @param n_bytes Size of the input in bytes.
	 * @param make Makes the value to hash from `n_bytes` bytes.
	 * @param hash The hash function.
	 * @return The worst pair of bits and the bound for its distance from one half.
	 */
	template<typename Make, typename Hash>
	Worst flip_bits(std::size_t n_bytes, Make const &make, Hash const &hash) {
		constexpr std::size_t output_bits = 8 * sizeof(std::size_t);
		std::size_t const input_bits = 8 * n_bytes;
		bool const every_value = input_bits <= 16;
		std::size_t const inputs = every_value ? std::size_t{1} << input_bits : random_inputs;
		// with every value, the inputs x and x with a flipped bit form one pair, counted twice
		std::size_t const pairs = every_value ? inputs / 2 : inputs;

		std::vector<std::size_t> flips(input_bits * output_bits, 0);
		std::vector<unsigned char> bytes(n_bytes);
		std::mt19937_64 random{seed};
		for (std::size_t input = 0; input < inputs; ++input) {
			for (std::size_t b = 0; b < n_bytes; ++b) {
				bytes[b] = static_cast<unsigned char>(every_value ? input >> (8 * b) : random());
			}
			std::size_t const original = hash(make(bytes.data()));
			for (std::size_t i = 0; i < input_bits; ++i) {
				auto const bit = static_cast<unsigned char>(1U << (i % 8));
				bytes[i / 8] ^= bit;
				std::size_t changed = original ^ hash(make(bytes.data()));
				bytes[i / 8] ^= bit;
				for (; changed != 0; changed &= changed - 1) {
					++flips[i * output_bits + static_cast<std::size_t>(std::countr_zero(changed))];
				}
			}
		}

		Worst worst;
		worst.bound = std::max(0.025, 6 * 0.5 / std::sqrt(static_cast<double>(pairs)));
		for (std::size_t i = 0; i < input_bits; ++i) {
			for (std::size_t j = 0; j < output_bits; ++j) {
				double const probability = static_cast<double>(flips[i * output_bits + j]) / static_cast<double>(inputs);
				if (std::abs(probability - 0.5) > worst.distance) {
					worst.input_bit = i;
					worst.output_bit = j;
					worst.probability = probability;
					worst.distance = std::abs(probability - 0.5);
				}
			}
		}
		return worst;
	}

	template<typename T>
	T load(unsigned char const *bytes) noexcept {
		T value;
		std::memcpy(&value, bytes, sizeof(T));
		return value;
	}

	std::string load_string(unsigned char const *bytes, std::size_t length) {
		return std::string(reinterpret_cast<char const *>(bytes), length);
	}

	template<typename T, typename Policy, typename Make>
	Worst flip_bits_of(std::size_t n_bytes, Make const &make) {
		DiceHash<T, Policy> const hasher{};
		return flip_bits(n_bytes, make, hasher);
	}

	/** Checks that the hash of `T` avalanches if `DiceHash<T, Policy>` is marked.
	 * @param name Name of the input, for the report.
	 * @param n_bytes Size of the input in bytes.
	 * @param make Makes the value to hash from `n_bytes` bytes.
	 */
	template<typename T, typename Policy, typename Make>
	void check_if_marked(std::string_view name, std::size_t n_bytes, Make const &make) {
		if constexpr (marked<T, Policy>) {
			Worst const worst = flip_bits_of<T, Policy>(n_bytes, make);
			INFO(name << ": input bit " << worst.input_bit << " flips output bit " << worst.output_bit
					  << " with probability " << worst.probability << ", bound " << worst.bound);
			CHECK(worst.distance <= worst.bound);
		}
	}

	/** Checks that the hash of `T` does not avalanche and that `DiceHash<T, Policy>` is not marked. */
	template<typename T, typename Policy, typename Make>
	void check_not_avalanching(std::string_view name, std::size_t n_bytes, Make const &make) {
		CHECK_FALSE(marked<T, Policy>);
		Worst const worst = flip_bits_of<T, Policy>(n_bytes, make);
		INFO(name << ": input bit " << worst.input_bit << " flips output bit " << worst.output_bit
				  << " with probability " << worst.probability << ", bound " << worst.bound);
		CHECK(worst.distance > worst.bound);
	}

	TEMPLATE_TEST_CASE("Every marked DiceHash avalanches", "[DiceHash][is_avalanching][avalanche]",
					   Martinus, xxh3, wyhash, rapidhash) {
		using Policy = TestType;
		using u8 = std::uint8_t;
		using u16 = std::uint16_t;
		using u32 = std::uint32_t;
		using u64 = std::uint64_t;
		using u128 = unsigned __int128;

		check_if_marked<u8, Policy>("std::uint8_t", 1, &load<u8>);
		check_if_marked<std::int8_t, Policy>("std::int8_t", 1, &load<std::int8_t>);
		check_if_marked<u16, Policy>("std::uint16_t", 2, &load<u16>);
		check_if_marked<std::int32_t, Policy>("std::int32_t", 4, &load<std::int32_t>);
		check_if_marked<u64, Policy>("std::uint64_t", 8, &load<u64>);
		check_if_marked<u128, Policy>("unsigned __int128", 16, &load<u128>);
		check_if_marked<float, Policy>("float", 4, &load<float>);
		check_if_marked<double, Policy>("double", 8, &load<double>);
		check_if_marked<u64 *, Policy>("std::uint64_t *", 8, &load<u64 *>);
		check_if_marked<Id, Policy>("Id, a std::uint64_t through its dice_hash_overload", 8, [](unsigned char const *b) {
			return Id{load<u64>(b)};
		});

		for (std::size_t const length : std::array<std::size_t, 5>{3, 7, 12, 16, 17}) {
			check_if_marked<std::string, Policy>("std::string of length " + std::to_string(length), length,
												 [length](unsigned char const *b) { return load_string(b, length); });
		}
		check_if_marked<std::u16string, Policy>("std::u16string of length 3", 6, [](unsigned char const *b) {
			return std::u16string{load<char16_t>(b), load<char16_t>(b + 2), load<char16_t>(b + 4)};
		});
		check_if_marked<std::vector<u32>, Policy>("std::vector<std::uint32_t> of 3 elements", 12, [](unsigned char const *b) {
			return std::vector<u32>{load<u32>(b), load<u32>(b + 4), load<u32>(b + 8)};
		});

		check_if_marked<std::pair<u64, u64>, Policy>("std::pair<std::uint64_t, std::uint64_t>", 16, [](unsigned char const *b) {
			return std::pair{load<u64>(b), load<u64>(b + 8)};
		});
		check_if_marked<std::pair<u32, u8>, Policy>("std::pair<std::uint32_t, std::uint8_t>", 5, [](unsigned char const *b) {
			return std::pair{load<u32>(b), load<u8>(b + 4)};
		});
		check_if_marked<std::tuple<u8, u16, u32>, Policy>("std::tuple<std::uint8_t, std::uint16_t, std::uint32_t>", 7, [](unsigned char const *b) {
			return std::tuple{load<u8>(b), load<u16>(b + 1), load<u32>(b + 3)};
		});
		check_if_marked<std::pair<std::string, u32>, Policy>("std::pair<std::string, std::uint32_t> with a string of length 7", 11, [](unsigned char const *b) {
			return std::pair{load_string(b, 7), load<u32>(b + 7)};
		});
		check_if_marked<std::optional<u64>, Policy>("std::optional<std::uint64_t> with a value", 8, [](unsigned char const *b) {
			return std::optional{load<u64>(b)};
		});
		check_if_marked<std::variant<u32, std::string>, Policy>("std::variant<std::uint32_t, std::string> with a string of length 5", 5, [](unsigned char const *b) {
			return std::variant<u32, std::string>{load_string(b, 5)};
		});
		check_if_marked<std::set<u64>, Policy>("std::set<std::uint64_t> of 3 elements", 24, [](unsigned char const *b) {
			return std::set<u64>{load<u64>(b), load<u64>(b + 8), load<u64>(b + 16)};
		});
		check_if_marked<std::map<u32, u32>, Policy>("std::map<std::uint32_t, std::uint32_t> of 2 elements", 16, [](unsigned char const *b) {
			return std::map<u32, u32>{{load<u32>(b), load<u32>(b + 4)}, {load<u32>(b + 8), load<u32>(b + 12)}};
		});
		check_if_marked<std::vector<std::pair<u32, u32>>, Policy>("std::vector<std::pair<std::uint32_t, std::uint32_t>> of 1 element", 8, [](unsigned char const *b) {
			return std::vector{std::pair{load<u32>(b), load<u32>(b + 4)}};
		});
		check_if_marked<std::vector<std::string>, Policy>("std::vector<std::string> of 2 strings of length 5", 10, [](unsigned char const *b) {
			return std::vector{load_string(b, 5), load_string(b + 5, 5)};
		});
		check_if_marked<std::vector<u64 *>, Policy>("std::vector<std::uint64_t *> of 1 element", 8, [](unsigned char const *b) {
			return std::vector{load<u64 *>(b)};
		});
		check_if_marked<std::pair<std::unordered_set<u64>, u64>, Policy>("std::pair<std::unordered_set<std::uint64_t>, std::uint64_t> with a set of 2 elements", 24, [](unsigned char const *b) {
			return std::pair{std::unordered_set<u64>{load<u64>(b), load<u64>(b + 8)}, load<u64>(b + 16)};
		});
	}

	TEST_CASE("The paths of Martinus that are not marked do not avalanche", "[DiceHash][is_avalanching][avalanche]") {
		using Policy = Martinus;
		using u32 = std::uint32_t;
		using u64 = std::uint64_t;

		// hash_int
		check_not_avalanching<std::uint8_t, Policy>("std::uint8_t", 1, &load<std::uint8_t>);
		check_not_avalanching<std::uint16_t, Policy>("std::uint16_t", 2, &load<std::uint16_t>);
		check_not_avalanching<std::int32_t, Policy>("std::int32_t", 4, &load<std::int32_t>);
		check_not_avalanching<u64, Policy>("std::uint64_t", 8, &load<u64>);
		check_not_avalanching<double, Policy>("double", 8, &load<double>);
		check_not_avalanching<u64 *, Policy>("std::uint64_t *", 8, &load<u64 *>);
		// hash_bytes with 4 to 7 bytes after the last full block of 8 bytes
		check_not_avalanching<float, Policy>("float", 4, &load<float>);
		check_not_avalanching<std::string, Policy>("std::string of length 7", 7, [](unsigned char const *b) { return load_string(b, 7); });
		check_not_avalanching<std::string, Policy>("std::string of length 13", 13, [](unsigned char const *b) { return load_string(b, 13); });
		check_not_avalanching<std::vector<u32>, Policy>("std::vector<std::uint32_t> of 3 elements", 12, [](unsigned char const *b) {
			return std::vector<u32>{load<u32>(b), load<u32>(b + 4), load<u32>(b + 8)};
		});
	}

	TEMPLATE_TEST_CASE("hash_combine and HashState avalanche for plain inputs exactly if the policy says so", "[DiceHash][is_avalanching][avalanche]",
					   Martinus, xxh3, wyhash, rapidhash) {
		using Policy = TestType;
		using u64 = std::uint64_t;
		constexpr bool combine = dice::hash::internal::avalanching_functions<Policy>::combine;

		auto const check = [](std::string_view name, Worst const &worst) {
			INFO(name << ": input bit " << worst.input_bit << " flips output bit " << worst.output_bit
					  << " with probability " << worst.probability << ", bound " << worst.bound);
			if constexpr (combine) {
				CHECK(worst.distance <= worst.bound);
			} else {
				CHECK(worst.distance > worst.bound);
			}
		};
		auto const one_word = [](unsigned char const *b) { return load<u64>(b); };
		auto const two_words = [](unsigned char const *b) { return std::array{load<u64>(b), load<u64>(b + 8)}; };

		check("hash_combine of 1 word", flip_bits(8, one_word, [](u64 x) { return Policy::hash_combine({x}); }));
		check("hash_combine of 2 words", flip_bits(16, two_words, [](std::array<u64, 2> const &x) { return Policy::hash_combine({x[0], x[1]}); }));
		check("HashState of 1 word", flip_bits(8, one_word, [](u64 x) {
				  typename Policy::HashState state{1};
				  state.add(x);
				  return state.digest();
			  }));
		check("HashState of 2 words", flip_bits(16, two_words, [](std::array<u64, 2> const &x) {
				  typename Policy::HashState state{2};
				  state.add(x[0]);
				  state.add(x[1]);
				  return state.digest();
			  }));
	}

	TEMPLATE_TEST_CASE("The hash of an unordered container keeps the relations of xor", "[DiceHash][is_avalanching]",
					   Martinus, xxh3, wyhash, rapidhash) {
		using Policy = TestType;
		DiceHash<std::unordered_set<std::uint64_t>, Policy> const hasher{};
		std::mt19937_64 random{seed};
		for (int i = 0; i < 100; ++i) {
			std::uint64_t const a = random();
			std::uint64_t const b = random();
			std::uint64_t const c = random();
			REQUIRE((hasher({a, b}) ^ hasher({a, c})) == hasher({b, c}));
		}
		REQUIRE(hasher({}) == 0);
	}
}// namespace dice::tests::hash::avalanche
