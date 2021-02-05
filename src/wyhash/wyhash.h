// Copyright 2020 The Abseil Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// This file provides the Google-internal implementation of the Wyhash
// algorithm.
//
// Wyhash is a fast hash function for hash tables, the fastest we've currently
// (late 2020) found that passes the SMHasher tests. The algorithm relies on
// intrinsic 128-bit multiplication for speed. This is not meant to be secure -
// just fast.

#ifndef ABSL_HASH_INTERNAL_WYHASH_H_
#define ABSL_HASH_INTERNAL_WYHASH_H_

#include <cstdint>
#include <cstdlib>
#include <utility>


namespace Dice::hash::wyhash {
	inline constexpr uint64_t kSeed = 0xe17a1465UL;
	inline constexpr uint64_t kWyhashSalt[5] = {
			uint64_t{0x243F6A8885A308D3},
			uint64_t{0x13198A2E03707344},
			uint64_t{0xA4093822299F31D0},
			uint64_t{0x082EFA98EC4E6C89},
			uint64_t{0x452821E638D01377},
	};

	// Hash function for a byte array. A 64-bit seed and a set of five 64-bit
	// integers are hashed into the result.
	//
	// To allow all hashable types (including string_view and Span) to depend on
	// this algoritm, we keep the API low-level, with as few dependencies as
	// possible.
	uint64_t Wyhash(const void *data, size_t len, uint64_t seed,
					const uint64_t salt[5]);

	inline uint64_t Hash64(const void *data, size_t len) {
		return Wyhash(data, len, kSeed, kWyhashSalt);
	}

	uint64_t Mix(uint64_t state, uint64_t v);

	uint64_t CombineContiguousImpl(
			uint64_t state, const unsigned char *first, size_t len);

	inline uint64_t combine_contiguous(uint64_t state, const unsigned char *first, std::size_t size) noexcept {
		return CombineContiguousImpl(state, first, size);
	}
	template<typename H, typename T>
	inline uint64_t hash_bytes(H hash_state, const T &value) {
		const unsigned char *start = reinterpret_cast<const unsigned char *>(&value);
		return H::combine_contiguous(std::move(hash_state), start, sizeof(value));
	}

	inline uint64_t combine(uint64_t state) noexcept {
		return state;
	}
	template<typename T, typename... Args>
	inline uint64_t combine(uint64_t state, T first, Args... args) noexcept {
		return combine(combine(state, first), args...);
	}
}// namespace Dice::hash::wyhash

#endif