#ifndef __AVX2__
#warning "Cannot benchmark AVX2 implementation of LtHash because your CPU does not support the AVX2 instruction set or it is disabled."
#include <cassert>
int main() {
	assert(false);
}
#else
#define DICE_HASH_BENCHMARK_LTHASH_MATH_ENGINE MathEngine_AVX2
#define DICE_HASH_BENCHMARK_LTHASH_INSTRUCTION_SET "AVX2"
#include "BenchmarkLtHash_template.hpp"
#endif