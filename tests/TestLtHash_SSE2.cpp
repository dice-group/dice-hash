#ifndef __SSE2__
#warning "Cannot test SSE2 implementation of LtHash because your CPU does not support the SSE2 instruction set or it is disabled."
#include <cassert>
int main() {
	assert(false);
}
#else
#define DICE_HASH_TEST_LTHASH_MATH_ENGINE MathEngine_SSE2
#define DICE_HASH_TEST_LTHASH_INSTRUCTION_SET "SSE2"
#include "TestLtHash_template.hpp"
#endif
