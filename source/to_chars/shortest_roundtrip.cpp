// The contents of this file is based on contents of:
//
// https://github.com/ulfjack/ryu/blob/master/ryu/common.h,
// https://github.com/ulfjack/ryu/blob/master/ryu/d2s.c, and
// https://github.com/ulfjack/ryu/blob/master/ryu/f2s.c,
//
// which are distributed under the following terms:
//--------------------------------------------------------------------------------
// Copyright 2018 Ulf Adams
//
// The contents of this file may be used under the terms of the Apache License,
// Version 2.0.
//
//    (See accompanying file LICENSE-Apache or copy at
//     http://www.apache.org/licenses/LICENSE-2.0)
//
// Alternatively, the contents of this file may be used under the terms of
// the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE-Boost or copy at
//     https://www.boost.org/LICENSE_1_0.txt)
//
// Unless required by applicable law or agreed to in writing, this software
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.
//--------------------------------------------------------------------------------
// Modifications Copyright 2020 Junekey Jeon
//
// Following modifications were made to the original contents:
//  - Put everything inside the namespace jkj::dragonbox::to_chars_detail
//  - Combined decimalLength9 (from common.h) and decimalLength17 (from d2s.c)
//    into a single template function decimal_length
//  - Combined to_chars (from f2s.c) and to_chars (from d2s.c) into a
//    single template function fp_to_chars_impl
//  - Removed index counting statements; replaced them with pointer increments
//  - Removed usages of DIGIT_TABLE; replaced them with radix_100_table
//
//  These modifications, together with other contents of this file may be used
//  under the same terms as the original contents.

#include "jkj/fp/to_chars/shortest_roundtrip.h"
#include "jkj/fp/to_chars/to_chars_common.h"

namespace jkj::fp {
	namespace detail {
		template <class Float>
		static char* to_chars_impl(unsigned_fp_t<Float> v, char* buffer)
		{
			constexpr auto max_decimal_length =
				ieee754_format_info<ieee754_traits<Float>::format>::decimal_digits;
			auto output = v.significand;
			auto const olength = decimal_length<max_decimal_length>(output);

			// Print the decimal digits.
			// The following code is equivalent to:
			// for (uint32_t i = 0; i < olength - 1; ++i) {
			//   const uint32_t c = output % 10; output /= 10;
			//   result[index + olength - i] = (char) ('0' + c);
			// }
			// result[index] = '0' + output % 10;

			uint32_t i = 0;
			if constexpr (sizeof(Float) == 8) {
				// We prefer 32-bit operations, even on 64-bit platforms.
				// We have at most 17 digits, and uint32_t can store 9 digits.
				// If output doesn't fit into uint32_t, we cut off 8 digits,
				// so the rest will fit into uint32_t.
				if ((output >> 32) != 0) {
					// Expensive 64-bit division.
					const uint64_t q = output / 100000000;
					uint32_t output2 = ((uint32_t)output) - 100000000 * ((uint32_t)q);
					output = q;

					const uint32_t c = output2 % 10000;
					output2 /= 10000;
					const uint32_t d = output2 % 10000;
					const uint32_t c0 = (c % 100) << 1;
					const uint32_t c1 = (c / 100) << 1;
					const uint32_t d0 = (d % 100) << 1;
					const uint32_t d1 = (d / 100) << 1;
					memcpy(buffer + olength - i - 1, radix_100_table + c0, 2);
					memcpy(buffer + olength - i - 3, radix_100_table + c1, 2);
					memcpy(buffer + olength - i - 5, radix_100_table + d0, 2);
					memcpy(buffer + olength - i - 7, radix_100_table + d1, 2);
					i += 8;
				}
			}

			auto output2 = (uint32_t)output;
			while (output2 >= 10000) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
				const uint32_t c = output2 - 10000 * (output2 / 10000);
#else
				const uint32_t c = output2 % 10000;
#endif
				output2 /= 10000;
				const uint32_t c0 = (c % 100) << 1;
				const uint32_t c1 = (c / 100) << 1;
				memcpy(buffer + olength - i - 1, radix_100_table + c0, 2);
				memcpy(buffer + olength - i - 3, radix_100_table + c1, 2);
				i += 4;
			}
			if (output2 >= 100) {
				const uint32_t c = (output2 % 100) << 1;
				output2 /= 100;
				memcpy(buffer + olength - i - 1, radix_100_table + c, 2);
				i += 2;
			}
			if (output2 >= 10) {
				const uint32_t c = output2 << 1;
				// We can't use memcpy here: the decimal dot goes between these two digits.
				buffer[olength - i] = radix_100_table[c + 1];
				buffer[0] = radix_100_table[c];
			}
			else {
				buffer[0] = (char)('0' + output2);
			}

			// Print decimal point if needed.
			if (olength > 1) {
				buffer[1] = '.';
				buffer += olength + 1;
			}
			else {
				++buffer;
			}

			// Print the exponent.
			*buffer = 'E';
			++buffer;
			int32_t exp = v.exponent + (int32_t)olength - 1;
			if (exp < 0) {
				*buffer = '-';
				++buffer;
				exp = -exp;
			}
			if constexpr (sizeof(Float) == 8) {
				if (exp >= 100) {
					const int32_t c = exp % 10;
					memcpy(buffer, radix_100_table + 2 * (exp / 10), 2);
					buffer[2] = (char)('0' + c);
					buffer += 3;
				}
				else if (exp >= 10) {
					memcpy(buffer, radix_100_table + 2 * exp, 2);
					buffer += 2;
				}
				else {
					*buffer = (char)('0' + exp);
					++buffer;
				}
			}
			else {
				if (exp >= 10) {
					memcpy(buffer, radix_100_table + 2 * exp, 2);
					buffer += 2;
				}
				else {
					*buffer = (char)('0' + exp);
					++buffer;
				}
			}

			return buffer;
		}
		
		char* to_chars(unsigned_fp_t<float> v, char* buffer) {
			return to_chars_impl(v, buffer);
		}
		char* to_chars(unsigned_fp_t<double> v, char* buffer) {
			return to_chars_impl(v, buffer);
		}
	}
}
