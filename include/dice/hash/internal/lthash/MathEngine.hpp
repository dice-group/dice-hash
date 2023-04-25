#ifndef DICE_HASH_MATHOPERATION_HPP
#define DICE_HASH_MATHOPERATION_HPP

namespace dice::hash::internal::lthash::detail {
#ifdef DICE_HASH_CACHE_LINE_SIZE
	inline constexpr size_t CACHE_LINE_SIZE = DICE_HASH_CACHE_LINE_SIZE;
#else
	inline constexpr size_t CACHE_LINE_SIZE = 64; // take a guess, this is a common size
#endif
} // namespace dice::hash::internal::lthash::detail

#if defined(__AVX2__)
#include "dice/hash/internal/lthash/MathEngine_AVX2.hpp"

namespace dice::hash::internal::lthash::detail {
	template<typename Bits>
	using MathEngine = MathEngine_AVX2<Bits>;
} // dice::hash::internal::lthash::detail
#elif defined(__SSE2__)
#include "dice/hash/internal/lthash/MathEngine_SSE2.hpp"

namespace dice::hash::internal::lthash::detail {
	template<typename Bits>
	using MathEngine = MathEngine_SSE2<Bits>;
} // dice::hash::internal::lthash::detail
#else
#include "dice/hash/internal/lthash/MathEngine_Simple.hpp"

namespace dice::hash::internal::lthash::detail {
	template<typename Bits>
	using MathEngine = MathEngine_Simple<Bits>;
} // dice::hash::internal::lthash::detail
#endif

#endif//DICE_HASH_MATHOPERATION_HPP
