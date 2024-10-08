#ifndef DICE_HASH_VERSION_HPP
#define DICE_HASH_VERSION_HPP

#include <array>

namespace dice::hash {
	inline constexpr char name[] = "dice-hash";
	inline constexpr char version[] = "0.4.8";
	inline constexpr std::array<int, 3> version_tuple = {0, 4, 8};
	inline constexpr int pobr_version = 1; ///< persisted object binary representation version
} // namespace dice::hash

#endif // DICE_HASH_VERSION_HPP
