// Copyright 2020 Junekey Jeon
//
// The contents of this file may be used under the terms of
// the Apache License v2.0 with LLVM Exceptions.
//
//    (See accompanying file LICENSE-Apache or copy at
//     https://llvm.org/foundation/relicensing/LICENSE.txt)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.

////////////////////////////////////////////////////////////
//////////// !!!! This header leaks macros !!!! ////////////
////////////////////////////////////////////////////////////

#ifndef JKJ_HEADER_FP_BITS
#define JKJ_HEADER_FP_BITS

#include "util.h"
#include <cassert>
#include <type_traits>

#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
#include <immintrin.h>
#elif defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>	// this includes immintrin.h as well
#endif

namespace jkj::fp {
	namespace detail {
		namespace bits {
			template <class UInt>
			int countl_zero(UInt n) noexcept {
				static_assert(std::is_unsigned_v<UInt> && sizeof(UInt) <= 8);
				assert(n != 0);
#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
				if constexpr (std::is_same_v<UInt, unsigned long>) {
					return __builtin_clzl(n);
				}
				else if constexpr (std::is_same_v<UInt, unsigned long long>) {
					return __builtin_clzll(n);
				}
				else {
					static_assert(sizeof(UInt) <= sizeof(unsigned int));
					return __builtin_clz((unsigned int)n)
						- (value_bits<unsigned int> - value_bits<UInt>);
				}
#elif defined(_MSC_VER) && defined(_M_X64)
				if constexpr (std::is_same_v<UInt, unsigned short>) {
					return int(__lzcnt16(n));
				}
				else if constexpr (std::is_same_v<UInt, unsigned __int64>) {
					return int(__lzcnt64(n));
				}
				else {
					static_assert(sizeof(UInt) <= sizeof(unsigned int));
					return int(__lzcnt((unsigned int)n)
						- (value_bits<unsigned int> - value_bits<UInt>));
				}
#else
				int count = int(value_bits<UInt>);

				std::uint32_t n32;
				if constexpr (value_bits<UInt> > 32) {
					if ((n >> 32) != 0) {
						count -= 33;
						n32 = std::uint32_t(n >> 32);
					}
					else {
						n32 = std::uint32_t(n);
						if (n32 != 0) {
							count -= 1;
						}
					}
				}
				else {
					n32 = std::uint32_t(n);
				}
				if constexpr (value_bits<UInt> > 16) {
					if ((n32 & 0xffff0000) != 0) count -= 16;
				}
				if constexpr (value_bits<UInt> > 8) {
					if ((n32 & 0xff00ff00) != 0) count -= 8;
				}
				if ((n32 & 0xf0f0f0f0) != 0) count -= 4;
				if ((n32 & 0xcccccccc) != 0) count -= 2;
				if ((n32 & 0xaaaaaaaa) != 0) count -= 1;

				return count;
#endif
			}

			template <class UInt>
			inline int countr_zero(UInt n) noexcept {
				static_assert(std::is_unsigned_v<UInt> && value_bits<UInt> <= 64);
#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
#define JKJ_HAS_COUNTR_ZERO_INTRINSIC 1
				if constexpr (std::is_same_v<UInt, unsigned long>) {
					return __builtin_ctzl(n);
				}
				else if constexpr (std::is_same_v<UInt, unsigned long long>) {
					return __builtin_ctzll(n);
				}
				else {
					static_assert(sizeof(UInt) <= sizeof(unsigned int));
					return __builtin_ctz((unsigned int)n);
				}
#elif defined(_MSC_VER) && defined(_M_X64)
#define JKJ_HAS_COUNTR_ZERO_INTRINSIC 1
				if constexpr (std::is_same_v<UInt, unsigned __int64>) {
					return int(_tzcnt_u64(n));
				}
				else {
					static_assert(sizeof(UInt) <= sizeof(unsigned int));
					return int(_tzcnt_u32((unsigned int)n));
				}
#else
#define JKJ_HAS_COUNTR_ZERO_INTRINSIC 0
				int count = int(value_bits<UInt>);

				auto n32 = std::uint32_t(n);
				if constexpr (value_bits<UInt> > 32) {
					if (n32 != 0) {
						count = 31;
					}
					else {
						n32 = std::uint32_t(n >> 32);
						if (n32 != 0) {
							count -= 1;
						}
					}
				}
				if constexpr (value_bits<UInt> > 16) {
					if ((n32 & 0x0000ffff) != 0) count -= 16;
				}
				if constexpr (value_bits<UInt> > 8) {
					if ((n32 & 0x00ff00ff) != 0) count -= 8;
				}
				if ((n32 & 0x0f0f0f0f) != 0) count -= 4;
				if ((n32 & 0x33333333) != 0) count -= 2;
				if ((n32 & 0x55555555) != 0) count -= 1;

				return count;
#endif
			}
		}
	}
}

#endif