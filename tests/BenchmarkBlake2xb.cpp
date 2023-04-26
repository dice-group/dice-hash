#include <catch2/catch_all.hpp>
#include <dice/hash/blake2xb/Blake2xb.hpp>

/**
* @note Benchmarks adapted from https://github.com/facebook/folly/blob/main/folly/experimental/crypto/test/Blake2xbBenchmark.cpp
*/

using namespace dice::hash::internal;

void benchmark_blake2b(size_t input_size, size_t n) {
	std::array<uint8_t, crypto_generichash_blake2b_BYTES_MAX> result;
	std::vector<uint8_t> input;

	input.resize(input_size);

	for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
		int res = crypto_generichash_blake2b(result.data(), sizeof(result), input.data(), input.size(), nullptr, 0);
		if (res != 0) {
			throw std::runtime_error("blake2b hash failed");
		}
	}
}

void benchmark_blake2b_multiple(size_t input_size, size_t m, size_t n) {
	std::vector<uint8_t> input;
	std::vector<uint8_t> output;
	std::vector<uint8_t> personalization;
	std::array<uint8_t, crypto_generichash_blake2b_BYTES_MAX> h0;

	output.resize(crypto_generichash_blake2b_BYTES_MAX * m);
	input.resize(input_size);
	personalization.resize(crypto_generichash_blake2b_PERSONALBYTES);

	for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
		int res = crypto_generichash_blake2b(
				h0.data(), sizeof(h0), input.data(), input.size(), nullptr, 0);
		if (res != 0) {
			throw std::runtime_error("blake2b hash failed");
		}

		for (size_t j = 0; j < m; j++) {
			res = crypto_generichash_blake2b_salt_personal(
					output.data() + (crypto_generichash_blake2b_BYTES_MAX * j),
					crypto_generichash_blake2b_BYTES_MAX,
					h0.data(),
					h0.size(),
					nullptr /* key */,
					0 /* keylen */,
					nullptr /* salt */,
					personalization.data());
			if (res != 0) {
				throw std::runtime_error("blake2b hash failed");
			}
			sodium_increment(
					personalization.data(), crypto_generichash_blake2b_PERSONALBYTES);
		}
	}
}

void benchmark_blake2xb(size_t input_size, size_t output_size, size_t n) {
	std::vector<std::byte> input;
	std::vector<std::byte> output;

	input.resize(input_size);
	output.resize(output_size);

	for (size_t i = 0; i < static_cast<size_t>(n); ++i) {
		blake2xb::Blake2xb<>::hash_single(input, output);
	}
}

TEST_CASE("Benchmark Blake2xb") {
	BENCHMARK("blake2b 100b in 64b out", n) {
		benchmark_blake2b(100, n);
	};

	BENCHMARK("blake2xb 100b in 64b out", n) {
		benchmark_blake2xb(100, 64, n);
	};

	BENCHMARK("blake2b 100b in 128b out", n) {
		benchmark_blake2b_multiple(100, 128 / 64, n);
	};

	BENCHMARK("blake2xb 100b in 128b out", n) {
		benchmark_blake2xb(100, 128, n);
	};

	BENCHMARK("blake2b 100b in 1024b out", n) {
		benchmark_blake2b_multiple(100, 1024 / 64, n);
	};

	BENCHMARK("blake2xb 100b in 1024b out", n) {
		benchmark_blake2xb(100, 1024, n);
	};

	BENCHMARK("blake2b 100b in 4096b out", n) {
		benchmark_blake2b_multiple(100, 4096 / 64, n);
	};

	BENCHMARK("blake2xb 100b in 4096b out", n) {
		benchmark_blake2xb(100, 4096, n);
	};

	BENCHMARK("blake2b 1000b in 64b out", n) {
		benchmark_blake2b(1000, n);
	};

	BENCHMARK("blake2xb 1000b in 64b out", n) {
		benchmark_blake2xb(1000, 64, n);
	};

	BENCHMARK("blake2b 1000b in 128b out", n) {
		benchmark_blake2b_multiple(1000, 128 / 64, n);
	};

	BENCHMARK("blake2xb 1000b in 128b out", n) {
		benchmark_blake2xb(1000, 128, n);
	};

	BENCHMARK("blake2b 1000b in 1024b out", n) {
		benchmark_blake2b_multiple(1000, 1024 / 64, n);
	};

	BENCHMARK("blake2xb 1000b in 1024b out", n) {
		benchmark_blake2xb(1000, 1024, n);
	};

	BENCHMARK("blake2b 1000b in 4096b out", n) {
		benchmark_blake2b_multiple(1000, 4096 / 64, n);
	};

	BENCHMARK("blake2xb 1000b in 4096b out", n) {
		benchmark_blake2xb(1000, 4096, n);
	};
}
