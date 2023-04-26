#ifndef __SSE2__
#warning "Cannot benchmark SSE2 implementation of LtHash because your CPU does not support the SSE2 instruction set or it is disabled."
#include <cassert>
int main() {
	assert(false);
}
#else
#undef __AVX2__
#include "BenchmarkLtHash_template.hpp"
#endif