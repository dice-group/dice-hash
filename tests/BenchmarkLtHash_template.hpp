#include <catch2/catch_all.hpp>
#include <dice/hash/lthash/LtHash.hpp>

/**
 * @note Benchmarks adapted from https://github.com/facebook/folly/blob/main/folly/experimental/crypto/test/LtHashBenchmark.cpp
 */

std::vector<std::byte> make_random_data(size_t length) {
	std::vector<std::byte> data;
	data.resize(length);

	std::default_random_engine rng{std::random_device{}()};
	std::uniform_int_distribution<unsigned char> dist{0, std::numeric_limits<unsigned char>::max()};

	for (size_t ix = 0; ix < length; ++ix) {
		data[ix] = static_cast<std::byte>(dist(rng));
	}

	return data;
}

std::vector<std::vector<std::byte>> objects = []() {
	std::vector<std::vector<std::byte>> ret;
	for (size_t i = 0; i < 1000; ++i) {
		ret.push_back(make_random_data(i));
	}
	return ret;
}();

using namespace dice::hash::lthash;
using namespace dice::hash::blake3;

template<size_t B, size_t N>
using H = LtHash<B, N, Blake3, DICE_HASH_BENCHMARK_LTHASH_MATH_ENGINE>;

template<size_t B, size_t N>
void run_benchmark(size_t n) {
	H<B, N> lt;
	for (size_t i = 0; i < n; ++i) {
		auto const &obj = objects[i % objects.size()];
		lt.add(obj);
	}
}

TEST_CASE("Benchmark LtHash using " DICE_HASH_BENCHMARK_LTHASH_INSTRUCTION_SET, "[DiceHash]") {
	BENCHMARK("LtHash<16, 1024>", n) {
		run_benchmark<16, 1024>(n);
	};

	BENCHMARK("LtHash<20, 1008>", n) {
		run_benchmark<20, 1008>(n);
	};

	BENCHMARK("LtHash<32, 1024>", n) {
		run_benchmark<32, 1024>(n);
	};

	BENCHMARK("LtHash<32, 2048>", n) {
		run_benchmark<32, 2048>(n);
	};

	BENCHMARK("LtHash<20, 1008> add 100k elems") {
		LtHash<20, 1008> lt;
		for (auto i = 0; i < 100'000; ++i) {
			auto const &obj = objects[i % objects.size()];
			lt.add(obj);
		}
	};

	BENCHMARK("LtHash<16, 1024> add 100k elems") {
		LtHash<16, 1024> lt;
		for (auto i = 0; i < 100'000; ++i) {
			auto const &obj = objects[i % objects.size()];
			lt.add(obj);
		}
	};

	BENCHMARK("LtHash<32, 1024> add 100k elems") {
		LtHash<32, 1024> lt;
		for (auto i = 0; i < 100'000; ++i) {
			auto const &obj = objects[i % objects.size()];
			lt.add(obj);
		}
	};

	BENCHMARK("LtHash<20, 1008> remove 100k elems") {
		LtHash<20, 1008> lt;
		for (auto i = 0; i < 100'000; ++i) {
			auto const &obj = objects[i % objects.size()];
			lt.remove(obj);
		}
	};

	BENCHMARK("LtHash<16, 1024> remove 100k elems") {
		LtHash<16, 1024> lt;
		for (auto i = 0; i < 100'000; ++i) {
			auto const &obj = objects[i % objects.size()];
			lt.remove(obj);
		}
	};

	BENCHMARK("LtHash<32, 1024> remove 100k elems") {
		LtHash<32, 1024> lt;
		for (auto i = 0; i < 100'000; ++i) {
			auto const &obj = objects[i % objects.size()];
			lt.remove(obj);
		}
	};

	BENCHMARK_ADVANCED("LtHash<16, 1024> add 150B object")(Catch::Benchmark::Chronometer meter) {
		LtHash<16, 1024> lt;
		auto obj = make_random_data(150);

		meter.measure([&]() {
			lt.add(obj);
		});
	};

	BENCHMARK_ADVANCED("LtHash<20, 1008> add 150B object")(Catch::Benchmark::Chronometer meter) {
		LtHash<20, 1008> lt;
		auto obj = make_random_data(150);

		meter.measure([&]() {
			lt.add(obj);
		});
	};

	BENCHMARK_ADVANCED("LtHash<32, 1024> add 150B object")(Catch::Benchmark::Chronometer meter) {
		LtHash<32, 1024> lt;
		auto obj = make_random_data(150);

		meter.measure([&]() {
			lt.add(obj);
		});
	};
}
