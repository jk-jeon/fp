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

#ifndef JKJ_HEADER_FP_WUINT
#define JKJ_HEADER_FP_WUINT

#include <cassert>
#include <cstdint>
#include "macros.h"

#if (defined(__GNUC__) || defined(__clang__)) && defined(__x86_64__)
#include <immintrin.h>
#elif defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>	// this includes immintrin.h as well
#endif

namespace jkj::fp {
	namespace detail {
		namespace wuint {
			struct uint128 {
				uint128() = default;

#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && defined(__x86_64__)
				unsigned __int128	internal_;

				constexpr uint128(std::uint64_t high, std::uint64_t low) noexcept :
					internal_{ ((unsigned __int128)low) | (((unsigned __int128)high) << 64) } {}

				constexpr uint128(unsigned __int128 u) : internal_{ u } {}

				constexpr std::uint64_t high() const noexcept {
					return std::uint64_t(internal_ >> 64);
				}
				constexpr std::uint64_t low() const noexcept {
					return std::uint64_t(internal_);
				}

				uint128& operator+=(std::uint64_t n) & noexcept {
					internal_ += n;
					return *this;
				}

				uint128 operator>>(int sh) const noexcept {
					assert(sh >= 0 && sh < 64);
					return{ internal_ >> sh };
				}
#else
				std::uint64_t	high_;
				std::uint64_t	low_;

				constexpr uint128(std::uint64_t high, std::uint64_t low) noexcept :
					high_{ high }, low_{ low } {}

				constexpr std::uint64_t high() const noexcept {
					return high_;
				}
				constexpr std::uint64_t low() const noexcept {
					return low_;
				}

				uint128& operator+=(std::uint64_t n) & noexcept {
#if defined(_MSC_VER) && defined(_M_X64)
					auto carry = _addcarry_u64(0, low_, n, &low_);
					_addcarry_u64(carry, high_, 0, &high_);
					return *this;
#else
					auto sum = low_ + n;
					high_ += (sum < low_ ? 1 : 0);
					low_ = sum;
					return *this;
#endif
				}

				uint128 operator>>(int sh) const noexcept {
					assert(sh >= 0 && sh < 64);
#if defined(_MSC_VER) && defined(_M_X64)
					return{ high_ >> sh, __shiftright128(low_, high_, (unsigned char)sh) };
#else
					return{ high_ >> sh,
						sh == 0 ? low_ : ((high_ << (64 - sh)) | (low_ >> sh)) };
#endif
				}
#endif
			};

			struct uint96 {
				uint96() = default;

				std::uint32_t		high_;
				std::uint32_t		middle_;
				std::uint32_t		low_;

				constexpr uint96(std::uint32_t high, std::uint32_t middle, std::uint32_t low) noexcept :
					high_{ high }, middle_{ middle }, low_{ low } {}

				constexpr std::uint32_t high() const noexcept {
					return high_;
				}
				constexpr std::uint32_t middle() const noexcept {
					return middle_;
				}
				constexpr std::uint32_t low() const noexcept {
					return low_;
				}
			};

			struct uint192 {
				uint192() = default;

				std::uint64_t	high_;
				std::uint64_t	middle_;
				std::uint64_t	low_;

				constexpr uint192(std::uint64_t high, std::uint64_t middle, std::uint64_t low) noexcept :
					high_{ high }, middle_{ middle }, low_{ low } {}

				constexpr std::uint64_t high() const noexcept {
					return high_;
				}
				constexpr std::uint64_t middle() const noexcept {
					return middle_;
				}
				constexpr std::uint64_t low() const noexcept {
					return low_;
				}
			};

			struct uint256 {
				uint256() = default;
				uint128		high_;
				uint128		low_;

				constexpr uint256(std::uint64_t high, std::uint64_t middle_high,
					std::uint64_t middle_low, std::uint64_t low) noexcept :
					high_{ high, middle_high }, low_{ middle_low, low } {}

				constexpr std::uint64_t high() const noexcept {
					return high_.high();
				}
				constexpr std::uint64_t middle_high() const noexcept {
					return high_.low();;
				}
				constexpr std::uint64_t middle_low() const noexcept {
					return low_.high();
				}
				constexpr std::uint64_t low() const noexcept {
					return low_.low();
				}
			};

			// Computes 128-bit result of multiplication of two 64-bit unsigned integers.
			JKJ_SAFEBUFFERS inline uint128 umul128(std::uint64_t x, std::uint64_t y) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && defined(__x86_64__)
				return (unsigned __int128)(x) * (unsigned __int128)(y);
#elif defined(_MSC_VER) && defined(_M_X64)
				uint128 result;
				result.low_ = _umul128(x, y, &result.high_);
				return result;
#else
				constexpr auto mask = (std::uint64_t(1) << 32) - std::uint64_t(1);

				auto a = x >> 32;
				auto b = x & mask;
				auto c = y >> 32;
				auto d = y & mask;

				auto ac = a * c;
				auto bc = b * c;
				auto ad = a * d;
				auto bd = b * d;

				auto intermediate = (bd >> 32) + (ad & mask) + (bc & mask);

				return{ ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32),
					(intermediate << 32) + (bd & mask)
				};
#endif
			}

			// Computes upper 64 bits of multiplication of two 64-bit unsigned integers.
			JKJ_SAFEBUFFERS inline std::uint64_t umul128_upper64(std::uint64_t x, std::uint64_t y) noexcept {
#if (defined(__GNUC__) || defined(__clang__)) && defined(__SIZEOF_INT128__) && defined(__x86_64__)
				auto p = (unsigned __int128)(x) * (unsigned __int128)(y);
				return std::uint64_t(p >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
				return __umulh(x, y);
#else
				constexpr auto mask = (std::uint64_t(1) << 32) - std::uint64_t(1);

				auto a = x >> 32;
				auto b = x & mask;
				auto c = y >> 32;
				auto d = y & mask;

				auto ac = a * c;
				auto bc = b * c;
				auto ad = a * d;
				auto bd = b * d;

				auto intermediate = (bd >> 32) + (ad & mask) + (bc & mask);

				return ac + (intermediate >> 32) + (ad >> 32) + (bc >> 32);
#endif
			}

			// Computes upper 64-bits of multiplication of a 64-bit unsigned integer
			// and a 128-bit unsigned integer.
			JKJ_SAFEBUFFERS inline std::uint64_t umul192_upper64(std::uint64_t x, uint128 y) noexcept {
				auto g0 = umul128(x, y.high());
				g0 += umul128_upper64(x, y.low());
				return g0.high();
			}

			// Computes upper 32-bits of multiplication of a 32-bit unsigned integer
			// and a 64-bit unsigned integer.
			inline std::uint32_t umul96_upper32(std::uint32_t x, std::uint64_t y) noexcept {
				return std::uint32_t(umul128_upper64(x, y));
			}

			// Computes upper 128 bits of multiplication of a 64-bit unsigned integer.
			// and a 192-bit unsigned integer.
			JKJ_SAFEBUFFERS inline uint128 umul256_upper128(std::uint64_t x, uint192 y) noexcept {
				auto g0 = umul128(x, y.high());
				auto g1 = umul128(x, y.middle());
				g1 += umul128_upper64(x, y.low());
				g0 += g1.high();
				return g0;
			}

			// Computes upper 64 bits of multiplication of a 32-bit unsigned integer
			// and a 96-bit unsigned integer.
			JKJ_SAFEBUFFERS inline std::uint64_t umul128_upper64(std::uint32_t x, uint96 y) noexcept {
				auto g0 = std::uint64_t(x) * std::uint64_t(y.high());
				auto g1 = umul128_upper64(std::uint64_t(x),
					(std::uint64_t(y.middle()) << 32) | y.low());
				return g0 + g1;
			}

			// Computes middle 64-bits of multiplication of a 64-bit unsigned integer
			// and a 128-bit unsigned integer.
			JKJ_SAFEBUFFERS inline std::uint64_t umul192_middle64(std::uint64_t x, uint128 y) noexcept {
				auto g01 = x * y.high();
				auto g10 = umul128_upper64(x, y.low());
				return g01 + g10;
			}

			// Computes middle 32-bits of multiplication of a 32-bit unsigned integer
			// and a 64-bit unsigned integer.
			inline std::uint64_t umul96_lower64(std::uint32_t x, std::uint64_t y) noexcept {
				return x * y;
			}

			// Computes the second 64-bit block of
			// 256-bit multiplication of two 128-bit unsigned integers.
			JKJ_SAFEBUFFERS inline std::uint64_t umul256_upper_middle64(uint128 x, uint128 y) noexcept {
				auto g11 = umul128_upper64(x.low(), y.low());
				auto g12 = umul128(x.low(), y.high());
				auto g21 = umul128(x.high(), y.low());
				auto g22 = x.high() * y.high();

				g12 += g11;
				g21 += g12.low();
				g22 += g12.high();
				g22 += g21.high();

				return g22;
			}
		}
	}
}

#include "undef_macros.h"
#endif