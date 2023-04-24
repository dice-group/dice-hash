#ifndef DICE_HASH_MATHOPERATION_HPP
#define DICE_HASH_MATHOPERATION_HPP

#if defined(__AVX2__)
#include "dice/hash/internal/lthash/MathEngine_AVX2.hpp"

namespace dice::hash::internal::lthash::detail {
	using MathEngine = MathEngine_AVX2;
} // dice::hash::internal::lthash::detail
#elif defined(__SSE2__)
#include "dice/hash/internal/lthash/MathEngine_SSE2.hpp"

namespace dice::hash::internal::lthash::detail {
	using MathEngine = MathEngine_SSE2;
} // dice::hash::internal::lthash::detail
#else
#include "dice/hash/internal/lthash/MathEngine_Simple.hpp"

namespace dice::hash::internal::lthash::detail {
	using MathEngine = MathEngine_Simple;
} // dice::hash::internal::lthash::detail
#endif

#endif//DICE_HASH_MATHOPERATION_HPP
