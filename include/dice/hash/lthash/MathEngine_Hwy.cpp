#include "MathEngine_Hwy.hpp"

#undef HWY_TARGET_INCLUDE
#define HWY_TARGET_INCLUDE "dice/hash/lthash/MathEngine_Hwy.cpp"
#include <hwy/foreach_target.h>

#include <hwy/highway.h>
#include <hwy/contrib/algo/transform-inl.h>


HWY_BEFORE_NAMESPACE();  // at file scope
namespace dice::hash::lthash::detail::HWY_NAMESPACE {
	static hwy::HWY_NAMESPACE::Vec<hwy::HWY_NAMESPACE::ScalableTag<uint64_t>> HWY_INLINE little_endian(hwy::HWY_NAMESPACE::Vec<hwy::HWY_NAMESPACE::ScalableTag<uint64_t>> data) {
		using namespace hwy::HWY_NAMESPACE;

		if constexpr (std::endian::native == std::endian::little) {
			return data;
		}
		else {
			return ReverseLaneBytes(data);
		}
	}
	static void add_with_padding_impl(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t data_mask) {
		using namespace hwy::HWY_NAMESPACE;

		using D = ScalableTag<uint64_t>;
		using V = Vec<D>;

		V mask = Set(D{}, data_mask);

		Transform1(D{}, dst.data(), dst.size(), src.data(), [&](D, V d, V s) HWY_ATTR {
			V a = little_endian(d);
			V b = little_endian(s);
			return little_endian(And(Add(a, b), mask));
		});
	}
	static void add_no_padding_impl(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t mask_group1, uint64_t mask_group2) {
		using namespace hwy::HWY_NAMESPACE;

		using D = ScalableTag<uint64_t>;
		using V = Vec<D>;

		V mask1 = Set(D{}, mask_group1);
		V mask2 = Set(D{}, mask_group2);

		Transform1(D{}, dst.data(), dst.size(), src.data(), [&](D, V d, V s) HWY_ATTR {
			V a = little_endian(d);
			V b = little_endian(s);

			V a1 = And(a, mask1);
			V a2 = And(a, mask2);
			V b1 = And(b, mask1);
			V b2 = And(b, mask2);

			return little_endian(Or(And(Add(a1, b1), mask1), And(Add(a2, b2), mask2)));
		});
	}
	static void sub_with_padding_impl(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t data_mask) {
		using namespace hwy::HWY_NAMESPACE;

		using D = ScalableTag<uint64_t>;
		using V = Vec<D>;

		V mask = Set(D{}, data_mask);

		Transform1(D{}, dst.data(), dst.size(), src.data(), [&](D, V d, V s) HWY_ATTR {
			V a = little_endian(d);
			V b = little_endian(s);

			V n = And(Sub(Not(mask), b), mask);
			return little_endian(And(Add(a, n), mask));
		});
	}
	static void sub_no_padding_impl(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t mask_group1, uint64_t mask_group2) {
		using namespace hwy::HWY_NAMESPACE;

		using D = ScalableTag<uint64_t>;
		using V = Vec<D>;

		V mask1 = Set(D{}, mask_group1);
		V mask2 = Set(D{}, mask_group2);

		Transform1(D{}, dst.data(), dst.size(), src.data(), [&](D, V d, V s) HWY_ATTR {
			V a = little_endian(d);
			V b = little_endian(s);

			V a1 = And(a, mask1);
			V a2 = And(a, mask2);
			V b1 = And(b, mask1);
			V b2 = And(b, mask2);

			V oi1 = And(Add(a1, Sub(mask2, b1)), mask1);
			V oi2 = And(Add(a2, Sub(mask1, b2)), mask2);

			return little_endian(Or(oi1, oi2));
		});
	}
	static bool check_padding_bits_impl(std::span<uint64_t const> data, uint64_t data_mask) {
		using namespace hwy::HWY_NAMESPACE;

		using D = ScalableTag<uint64_t>;
		using V = Vec<D>;

		V mask = Set(D{}, data_mask);
		V not_mask = Not(mask);
		V zero = Zero(D{});

		bool r = true;

		Foreach(D{}, data.data(), data.size(), mask, [&](D, V d) {
			d = And(little_endian(d), not_mask);
			auto m = Eq(d, zero);
			if (!AllTrue(D{}, m)) {
				r = false;
			}
		});

		return r;
	}
	static void clear_padding_bits_impl(std::span<uint64_t> data, uint64_t data_mask) {
		using namespace hwy::HWY_NAMESPACE;

		using D = ScalableTag<uint64_t>;
		using V = Vec<D>;

		V mask = Set(D{}, data_mask);

		Transform(D{}, data.data(), data.size(), [&](D, V d) {
			return little_endian(And(little_endian(d), mask));
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
	HWY_EXPORT(check_padding_bits_impl);
	HWY_EXPORT(clear_padding_bits_impl);

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
	bool check_padding_bits(std::span<uint64_t const> data, uint64_t data_mask) {
		return HWY_DYNAMIC_DISPATCH(check_padding_bits_impl)(data, data_mask);
	}
	void clear_padding_bits(std::span<uint64_t> data, uint64_t data_mask) {
		HWY_DYNAMIC_DISPATCH(clear_padding_bits_impl)(data, data_mask);
	}
}
#endif
