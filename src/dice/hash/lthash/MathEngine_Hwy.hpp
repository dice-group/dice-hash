#ifndef DICE_HASH_MATHENGINE_HWY_HPP
#define DICE_HASH_MATHENGINE_HWY_HPP

#include <algorithm>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>

namespace dice::hash::lthash {
	namespace detail {
		void add_with_padding(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t data_mask);
		void add_no_padding(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t mask_group1, uint64_t mask_group2);
		void sub_with_padding(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t data_mask);
		void sub_no_padding(std::span<uint64_t> dst, std::span<uint64_t const> src, uint64_t mask_group1, uint64_t mask_group2);
		bool check_padding_bits(std::span<uint64_t const> data, uint64_t data_mask);
		void clear_padding_bits(std::span<uint64_t> data, uint64_t data_mask);
	}

	template<typename Bits>
	struct MathEngine_Hwy {
		static constexpr size_t min_buffer_align = alignof(uint64_t);

		template<size_t DstExtent, size_t SrcExtent>
		static void add(std::span<std::byte, DstExtent> dst, std::span<std::byte const, SrcExtent> src) noexcept {
			assert(dst.size() == src.size());
			assert(dst.size() % sizeof(uint64_t) == 0);
			assert(reinterpret_cast<uintptr_t>(dst.data()) % alignof(uint64_t) == 0);
			assert(reinterpret_cast<uintptr_t>(src.data()) % alignof(uint64_t) == 0);

			std::span<uint64_t> dst64{reinterpret_cast<uint64_t *>(dst.data()), dst.size() / sizeof(uint64_t)};
			std::span<uint64_t const> src64{reinterpret_cast<uint64_t const *>(src.data()), src.size() / sizeof(uint64_t)};

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				static constexpr uint64_t mask_group_1 = Bits::bits_per_element == 16
																 ? 0xffff0000ffff0000ull
																 : 0xffffffff00000000ull;
				static constexpr uint64_t mask_group_2 = ~mask_group_1;

				detail::add_no_padding(dst64, src64, mask_group_1, mask_group_2);
			} else {
				detail::add_with_padding(dst64, src64, Bits::data_mask);
			}
		}

		template<size_t DstExtent, size_t SrcExtent>
		static void sub(std::span<std::byte, DstExtent> dst, std::span<std::byte const, SrcExtent> src) noexcept {
			assert(dst.size() == src.size());
			assert(dst.size() % sizeof(uint64_t) == 0);
			assert(reinterpret_cast<uintptr_t>(dst.data()) % alignof(uint64_t) == 0);
			assert(reinterpret_cast<uintptr_t>(src.data()) % alignof(uint64_t) == 0);

			std::span<uint64_t> dst64{reinterpret_cast<uint64_t *>(dst.data()), dst.size() / sizeof(uint64_t)};
			std::span<uint64_t const> src64{reinterpret_cast<uint64_t const *>(src.data()), src.size() / sizeof(uint64_t)};

			if constexpr (!Bits::needs_padding) {
				static_assert(Bits::bits_per_element == 16 || Bits::bits_per_element == 32,
							  "Only 16 and 32 bit elements are implemented for non-padded data");

				static constexpr uint64_t mask_group_1 = Bits::bits_per_element == 16
																 ? 0xffff0000ffff0000ull
																 : 0xffffffff00000000ull;
				static constexpr uint64_t mask_group_2 = ~mask_group_1;

				detail::sub_no_padding(dst64, src64, mask_group_1, mask_group_2);
			} else {
				detail::sub_with_padding(dst64, src64, Bits::data_mask);
			}
		}

		template<size_t Extent>
			requires(Bits::needs_padding)
		static bool check_padding_bits(std::span<std::byte const, Extent> data) noexcept {
			assert(data.size() % sizeof(uint64_t) == 0);
			assert(reinterpret_cast<uintptr_t>(data.data()) % alignof(uint64_t) == 0);

			std::span<uint64_t const> data64{reinterpret_cast<uint64_t const *>(data.data()), data.size() / sizeof(uint64_t)};
			return detail::check_padding_bits(data64, Bits::data_mask);
		}

		template<size_t Extent>
			requires(Bits::needs_padding)
		static void clear_padding_bits(std::span<std::byte, Extent> data) noexcept {
			assert(data.size() % sizeof(uint64_t) == 0);
			assert(reinterpret_cast<uintptr_t>(data.data()) % alignof(uint64_t) == 0);

			std::span<uint64_t> data64{reinterpret_cast<uint64_t *>(data.data()), data.size() / sizeof(uint64_t)};
			detail::clear_padding_bits(data64, Bits::data_mask);
		}
	};

} // namespace dice::hash::lthash

#endif//DICE_HASH_MATHENGINE_HWY_HPP
