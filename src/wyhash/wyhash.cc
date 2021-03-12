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

#include "Dice/hash/wyhash.h"

#include "internal/int128.h"
#include "internal/unaligned_access.h"

namespace Dice::hash::wyhash {


	static uint64_t WyhashMix(uint64_t v0, uint64_t v1) {
		Dice::hash::wyhash::uint128 p = v0;
		p *= v1;
		return Dice::hash::wyhash::Uint128Low64(p) ^ Dice::hash::wyhash::Uint128High64(p);
	}

	uint64_t Wyhash(const void *data, size_t len, uint64_t seed,
					const uint64_t salt[]) {
		const auto *ptr = static_cast<const uint8_t *>(data);
		auto starting_length = static_cast<uint64_t>(len);
		uint64_t current_state = seed ^ salt[0];

		if (len > 64) {
			// If we have more than 64 bytes, we're going to handle chunks of 64
			// bytes at a time. We're going to build up two separate hash states
			// which we will then hash together.
			uint64_t duplicated_state = current_state;

			do {
				uint64_t a = Dice::hash::wyhash::UnalignedLoad64(ptr);
				uint64_t b = Dice::hash::wyhash::UnalignedLoad64(ptr + 8);
				uint64_t c = Dice::hash::wyhash::UnalignedLoad64(ptr + 16);
				uint64_t d = Dice::hash::wyhash::UnalignedLoad64(ptr + 24);
				uint64_t e = Dice::hash::wyhash::UnalignedLoad64(ptr + 32);
				uint64_t f = Dice::hash::wyhash::UnalignedLoad64(ptr + 40);
				uint64_t g = Dice::hash::wyhash::UnalignedLoad64(ptr + 48);
				uint64_t h = Dice::hash::wyhash::UnalignedLoad64(ptr + 56);

				uint64_t cs0 = WyhashMix(a ^ salt[1], b ^ current_state);
				uint64_t cs1 = WyhashMix(c ^ salt[2], d ^ current_state);
				current_state = (cs0 ^ cs1);

				uint64_t ds0 = WyhashMix(e ^ salt[3], f ^ duplicated_state);
				uint64_t ds1 = WyhashMix(g ^ salt[4], h ^ duplicated_state);
				duplicated_state = (ds0 ^ ds1);

				ptr += 64;
				len -= 64;
			} while (len > 64);

			current_state = current_state ^ duplicated_state;
		}

		// We now have a data `ptr` with at most 64 bytes and the current state
		// of the hashing state machine stored in current_state.
		while (len > 16) {
			uint64_t a = Dice::hash::wyhash::UnalignedLoad64(ptr);
			uint64_t b = Dice::hash::wyhash::UnalignedLoad64(ptr + 8);

			current_state = WyhashMix(a ^ salt[1], b ^ current_state);

			ptr += 16;
			len -= 16;
		}

		// We now have a data `ptr` with at most 16 bytes.
		uint64_t a = 0;
		uint64_t b = 0;
		if (len > 8) {
			// When we have at least 9 and at most 16 bytes, set A to the first 64
			// bits of the input and B to the last 64 bits of the input. Yes, they will
			// overlap in the middle if we are working with less than the full 16
			// bytes.
			a = Dice::hash::wyhash::UnalignedLoad64(ptr);
			b = Dice::hash::wyhash::UnalignedLoad64(ptr + len - 8);
		} else if (len > 3) {
			// If we have at least 4 and at most 8 bytes, set A to the first 32
			// bits and B to the last 32 bits.
			a = Dice::hash::wyhash::UnalignedLoad32(ptr);
			b = Dice::hash::wyhash::UnalignedLoad32(ptr + len - 4);
		} else if (len > 0) {
			// If we have at least 1 and at most 3 bytes, read all of the provided
			// bits into A, with some adjustments.
			a = ((ptr[0] << 16) | (ptr[len >> 1] << 8) | ptr[len - 1]);
			b = 0;
		} else {
			a = 0;
			b = 0;
		}

		uint64_t w = WyhashMix(a ^ salt[1], b ^ current_state);
		uint64_t z = salt[1] ^ starting_length;
		return WyhashMix(w, z);
	}


	uint64_t Mix(uint64_t state, uint64_t v) {
		static constexpr uint64_t kMul = uint64_t{0x9ddfea08eb382d69};
		using MultType =
				std::conditional_t<sizeof(size_t) == 4, uint64_t, __uint128_t>;
		// We do the addition in 64-bit space to make sure the 128-bit
		// multiplication is fast. If we were to do it as MultType the compiler has
		// to assume that the high word is non-zero and needs to perform 2
		// multiplications instead of one.
		MultType m = state + v;
		m *= kMul;
		return static_cast<uint64_t>(m ^ (m >> (sizeof(m) * 8 / 2)));
	}
}// namespace Dice::hash::wyhash
