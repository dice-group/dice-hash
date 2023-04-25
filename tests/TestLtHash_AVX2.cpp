#ifndef __AVX2__
#warning "Cannot test AVX2 implementation of LtHash because your CPU does not support the AVX2 instruction set or it is disabled."
#include <cassert>
int main() {
	assert(false);
}
#else
#undef __SSE2__
#include "TestLtHash_template.hpp"
#endif
