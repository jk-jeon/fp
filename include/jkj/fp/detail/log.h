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

#ifndef JKJ_HEADER_FP_LOG
#define JKJ_HEADER_FP_LOG

#include <cassert>
#include <cstdint>
#include <cstddef>

namespace jkj::fp {
	namespace detail {
		namespace log {
			constexpr std::int32_t floor_shift(
				std::uint32_t integer_part,
				std::uint64_t fractional_digits,
				std::size_t shift_amount) noexcept
			{
				assert(shift_amount < 32);
				// Ensure no overflow
				assert(shift_amount == 0 || integer_part < (std::uint32_t(1) << (32 - shift_amount)));

				return shift_amount == 0 ? std::int32_t(integer_part) :
					std::int32_t(
						(integer_part << shift_amount) |
						(fractional_digits >> (64 - shift_amount)));
			}

			// Computes floor(e * c - s)
			template <
				std::uint32_t c_integer_part,
				std::uint64_t c_fractional_digits,
				std::size_t shift_amount,
				std::int32_t max_exponent,
				std::uint32_t s_integer_part = 0,
				std::uint64_t s_fractional_digits = 0
			>
				constexpr int compute(int e) noexcept {
				assert(e <= max_exponent && e >= -max_exponent);
				constexpr auto c = floor_shift(c_integer_part, c_fractional_digits, shift_amount);
				constexpr auto s = floor_shift(s_integer_part, s_fractional_digits, shift_amount);
				return int((std::int32_t(e) * c - s) >> shift_amount);
			}

			static constexpr std::uint64_t log10_2_fractional_digits{ 0x4d10'4d42'7de7'fbcc };
			static constexpr std::uint64_t log10_4_over_3_fractional_digits{ 0x1ffb'fc2b'bc78'0375 };
			constexpr std::size_t floor_log10_pow2_shift_amount = 22;

			static constexpr std::uint64_t log10_5_fractional_digits{ 0xb2ef'b2bd'8218'0433 };
			constexpr std::size_t floor_log10_pow5_shift_amount = 20;
			
			static constexpr std::uint64_t log2_10_fractional_digits{ 0x5269'e12f'346e'2bf9 };
			constexpr std::size_t floor_log2_pow10_shift_amount = 19;

			static constexpr std::uint64_t log5_2_fractional_digits{ 0x6e40'd1a4'143d'cb94 };
			static constexpr std::uint64_t log5_3_fractional_digits{ 0xaebf'4791'5d44'3b24 };
			constexpr std::size_t floor_log5_pow2_shift_amount = 20;

			// For constexpr computation
			// Returns -1 when n = 0
			template <class UInt>
			constexpr int floor_log2(UInt n) noexcept {
				int count = -1;
				while (n != 0) {
					++count;
					n >>= 1;
				}
				return count;
			}

			constexpr int floor_log10_pow2(int e) noexcept {
				using namespace log;
				return compute<
					0, log10_2_fractional_digits,
					floor_log10_pow2_shift_amount, 1700>(e);
			}

			constexpr int floor_log10_pow5(int e) noexcept {
				using namespace log;
				return compute<
					0, log10_5_fractional_digits,
					floor_log10_pow5_shift_amount, 2620>(e);
			}

			constexpr int floor_log2_pow5(int e) noexcept {
				using namespace log;
				return compute<
					2, log2_10_fractional_digits,
					floor_log2_pow10_shift_amount, 1764>(e);
			}

			constexpr int floor_log2_pow10(int e) noexcept {
				using namespace log;
				return compute<
					3, log2_10_fractional_digits,
					floor_log2_pow10_shift_amount, 1233>(e);
			}

			constexpr int floor_log5_pow2(int e) noexcept {
				using namespace log;
				return compute<
					0, log5_2_fractional_digits,
					floor_log5_pow2_shift_amount, 1492>(e);
			}

			constexpr int floor_log5_pow2_minus_log5_3(int e) noexcept {
				using namespace log;
				return compute<
					0, log5_2_fractional_digits,
					floor_log5_pow2_shift_amount, 2427,
					0, log5_3_fractional_digits>(e);
			}

			constexpr int floor_log10_pow2_minus_log10_4_over_3(int e) noexcept {
				using namespace log;
				return compute<
					0, log10_2_fractional_digits,
					floor_log10_pow2_shift_amount, 1700,
					0, log10_4_over_3_fractional_digits>(e);
			}
		}
	}
}

#endif