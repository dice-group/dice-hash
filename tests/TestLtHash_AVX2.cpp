#ifndef __AVX2__
#warning "Cannot test AVX2 implementation of LtHash because your CPU does not support the AVX2 instruction set or it is disabled."
#include <cassert>
int main() {
	assert(false);
}
#else
#define DICE_HASH_TEST_LTHASH_MATH_ENGINE MathEngine_AVX2
#define DICE_HASH_TEST_LTHASH_INSTRUCTION_SET "AVX2"
#include "TestLtHash_template.hpp"
#endif
