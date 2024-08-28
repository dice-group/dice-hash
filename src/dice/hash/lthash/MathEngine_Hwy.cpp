#include "MathEngine_Hwy.hpp"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "dice/hash/lthash/MathEngine_Hwy.cpp"
#include <hwy/foreach_target.h>

#include <hwy/highway.h>
#include <hwy/contrib/algo/transform-inl.h>


HWY_BEFORE_NAMESPACE();  // at file scope
namespace dice::hash::lthash::detail::HWY_NAMESPACE {
	static void add_with_padding_impl(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t data_mask) {
		using namespace hwy::HWY_NAMESPACE;

		using D = ScalableTag<uint64_t>;
		using V = Vec<D>;

		V mask = Set(D{}, data_mask);

		Transform1(D{}, dst.data(), dst.size(), src.data(), [&](D, V d, V s) HWY_ATTR {
			return And(Add(d, s), mask);
		});
	}
	static void add_no_padding_impl(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t mask_group1, uint64_t mask_group2) {
		using namespace hwy::HWY_NAMESPACE;

		using D = ScalableTag<uint64_t>;
		using V = Vec<D>;

		V mask1 = Set(D{}, mask_group1);
		V mask2 = Set(D{}, mask_group2);

		Transform1(D{}, dst.data(), dst.size(), src.data(), [&](D, V d, V s) HWY_ATTR {
			V a1 = And(d, mask1);
			V a2 = And(d, mask2);
			V b1 = And(s, mask1);
			V b2 = And(s, mask2);
			return Or(And(Add(a1, b1), mask1), And(Add(a2, b2), mask2));
		});
	}
	static void sub_with_padding_impl(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t data_mask) {
		using namespace hwy::HWY_NAMESPACE;

		using D = ScalableTag<uint64_t>;
		using V = Vec<D>;

		V mask = Set(D{}, data_mask);

		Transform1(D{}, dst.data(), dst.size(), src.data(), [&](D, V d, V s) HWY_ATTR {
			V n = And(Sub(Not(mask), s), mask);
			return And(Add(d, n), mask);
		});
	}
	static void sub_no_padding_impl(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t mask_group1, uint64_t mask_group2) {
		using namespace hwy::HWY_NAMESPACE;

		using D = ScalableTag<uint64_t>;
		using V = Vec<D>;

		V mask1 = Set(D{}, mask_group1);
		V mask2 = Set(D{}, mask_group2);

		Transform1(D{}, dst.data(), dst.size(), src.data(), [&](D, V d, V s) HWY_ATTR {
			V a1 = And(d, mask1);
			V a2 = And(d, mask2);
			V b1 = And(s, mask1);
			V b2 = And(s, mask2);

			V oi1 = And(Add(a1, Sub(mask2, b1)), mask1);
			V oi2 = And(Add(a2, Sub(mask1, b2)), mask2);

			return Or(oi1, oi2);
		});
	}
}
HWY_AFTER_NAMESPACE();

#if HWY_ONCE
namespace dice::hash::lthash::detail {
	HWY_EXPORT(add_with_padding_impl);
	HWY_EXPORT(add_no_padding_impl);
	HWY_EXPORT(sub_with_padding_impl);
	HWY_EXPORT(sub_no_padding_impl);

	void add_with_padding(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t data_mask) {
		HWY_DYNAMIC_DISPATCH(add_with_padding_impl)(dst, src, data_mask);
	}
	void add_no_padding(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t mask_group1, uint64_t mask_group2) {
		HWY_DYNAMIC_DISPATCH(add_no_padding_impl)(dst, src, mask_group1, mask_group2);
	}
	void sub_with_padding(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t data_mask) {
		HWY_DYNAMIC_DISPATCH(sub_with_padding_impl)(dst, src, data_mask);
	}
	void sub_no_padding(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t mask_group1, uint64_t mask_group2) {
		HWY_DYNAMIC_DISPATCH(sub_no_padding_impl)(dst, src, mask_group1, mask_group2);
	}
}
#endif
