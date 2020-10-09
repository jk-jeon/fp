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

#include "jkj/fp/to_chars/shortest_roundtrip.h"
#include "jkj/fp/to_chars/to_chars_common.h"
#include <cstring>

namespace jkj::fp {
	namespace detail {
		char const radix_100_table[200] = {
			'0', '0', '0', '1', '0', '2', '0', '3', '0', '4',
			'0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
			'1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
			'1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
			'2', '0', '2', '1', '2', '2', '2', '3', '2', '4',
			'2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
			'3', '0', '3', '1', '3', '2', '3', '3', '3', '4',
			'3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
			'4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
			'4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
			'5', '0', '5', '1', '5', '2', '5', '3', '5', '4',
			'5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
			'6', '0', '6', '1', '6', '2', '6', '3', '6', '4',
			'6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
			'7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
			'7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
			'8', '0', '8', '1', '8', '2', '8', '3', '8', '4',
			'8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
			'9', '0', '9', '1', '9', '2', '9', '3', '9', '4',
			'9', '5', '9', '6', '9', '7', '9', '8', '9', '9'
		};

		char* print_number(char* buffer, std::uint32_t number, int length) noexcept {
			assert(length >= 0);
			auto ret = buffer + length;
			while (length > 4) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
				auto c = number - 1'0000 * (number / 1'0000);
#else
				auto c = number % 1'0000;
#endif
				number /= 1'0000;
				std::memcpy(buffer + length - 2,
					&detail::radix_100_table[(c % 100) * 2], 2);
				std::memcpy(buffer + length - 4,
					&detail::radix_100_table[(c / 100) * 2], 2);
				length -= 4;
			}
			if (length > 2) {
				auto c = number % 100;
				number /= 100;
				std::memcpy(buffer + length - 2,
					&detail::radix_100_table[c * 2], 2);
				length -= 2;
			}
			if (length > 1) {
				std::memcpy(buffer,
					&detail::radix_100_table[number * 2], 2);
			}
			else if (length > 0) {
				*buffer = char('0' + number);
			}
			return ret;
		}

		char* print_nine_digits(char* buffer, std::uint32_t number) noexcept {
			if (number == 0) {
				std::memset(buffer, '0', 9);
			}
			else {
				for (int i = 0; i < 2; ++i) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
					auto c = number - 1'0000 * (number / 1'0000);
#else
					auto c = number % 1'0000;
#endif
					number /= 1'0000;
					std::memcpy(buffer + 7 - 4 * i,
						&detail::radix_100_table[(c % 100) * 2], 2);
					std::memcpy(buffer + 5 - 4 * i,
						&detail::radix_100_table[(c / 100) * 2], 2);
				}
				*buffer = char('0' + number);
			}
			return buffer + 9;
		}

		static char* print_zero_or_nine(char* buffer, int length, char const d) noexcept {
			assert(length >= 0);
			while (length >= 4) {
				// Both msvc and clang generate unnecessary memset call
				//std::memset(buffer, d, 4);
				buffer[0] = d;
				buffer[1] = d;
				buffer[2] = d;
				buffer[3] = d;
				buffer += 4;
				length -= 4;
			}
			if (length >= 2) {
				std::memset(buffer, d, 2);
				buffer += 2;
				length -= 2;
			}
			if (length > 0) {
				std::memset(buffer, d, 1);
				++buffer;
			}
			return buffer;
		}

		char* print_zeros(char* buffer, int length) noexcept {
			return print_zero_or_nine(buffer, length, '0');
		}

		char* print_nines(char* buffer, int length) noexcept {
			return print_zero_or_nine(buffer, length, '9');
		}

		template <class Float>
		static char* to_chars_shortest_scientific_n_impl_impl(unsigned_decimal_fp<Float> v, char* buffer)
		{
			constexpr auto max_decimal_length =
				ieee754_format_info<ieee754_traits<Float>::format>::decimal_digits;
			auto const significand_length = int(decimal_length<max_decimal_length>(v.significand));

			std::uint32_t significand;
			int remaining_significand_length;
			if constexpr (ieee754_traits<Float>::format == ieee754_format::binary64)
			{
				if ((v.significand >> 32) != 0) {
					// Since v.significand is at most 10^17, the quotient is at most 10^9, so
					// it fits inside 32-bit integer
					significand = std::uint32_t(v.significand / 1'0000'0000);
					auto r = std::uint32_t(v.significand) - significand * 1'0000'0000;

					// Print 8 digits
					for (int i = 0; i < 2; ++i) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
						auto c = r - 1'0000 * (r / 1'0000);
#else
						auto c = r % 1'0000;
#endif
						r /= 1'0000;
						std::memcpy(buffer + significand_length - 4 * i - 1,
							&detail::radix_100_table[(c % 100) * 2], 2);
						std::memcpy(buffer + significand_length - 4 * i - 3,
							&detail::radix_100_table[(c / 100) * 2], 2);
					}

					remaining_significand_length = significand_length - 8;
				}
				else {
					significand = std::uint32_t(v.significand);
					remaining_significand_length = significand_length;
				}
			}
			else
			{
				significand = std::uint32_t(v.significand);
				remaining_significand_length = significand_length;
			}

			while (remaining_significand_length > 4) {
#ifdef __clang__ // https://bugs.llvm.org/show_bug.cgi?id=38217
				auto c = significand - 1'0000 * (significand / 1'0000);
#else
				auto c = significand % 1'0000;
#endif
				significand /= 1'0000;
				std::memcpy(buffer + remaining_significand_length - 1,
					&detail::radix_100_table[(c % 100) * 2], 2);
				std::memcpy(buffer + remaining_significand_length - 3,
					&detail::radix_100_table[(c / 100) * 2], 2);
				remaining_significand_length -= 4;
			}
			if (remaining_significand_length > 2) {
				auto c = significand % 100;
				significand /= 100;
				std::memcpy(buffer + remaining_significand_length - 1,
					&detail::radix_100_table[c * 2], 2);
				remaining_significand_length -= 2;
			}
			if (remaining_significand_length > 1) {
				assert(remaining_significand_length == 2);
				buffer[0] = char('0' + (significand / 10));
				buffer[1] = '.';
				buffer[2] = char('0' + (significand % 10));
				buffer += (significand_length + 1);
			}
			else {
				buffer[0] = char('0' + significand);
				if (significand_length > 1) {
					buffer[1] = '.';
					buffer += (significand_length + 1);
				}
				else {
					buffer += 1;
				}
			}

			// Print exponent and return
			*buffer = 'E';
			++buffer;
			auto exp = v.exponent + significand_length - 1;
			if (exp < 0) {
				*buffer = '-';
				++buffer;
				exp = -exp;
			}

			if constexpr (ieee754_traits<Float>::format == ieee754_format::binary64)
			{
				if (exp >= 100) {
					std::memcpy(buffer, &detail::radix_100_table[(exp / 10) * 2], 2);
					buffer[2] = (char)('0' + (exp % 10));
					buffer += 3;
				}
				else if (exp >= 10) {
					std::memcpy(buffer, &detail::radix_100_table[exp * 2], 2);
					buffer += 2;
				}
				else {
					*buffer = (char)('0' + exp);
					buffer += 1;
				}
			}
			else
			{
				if (exp >= 10) {
					std::memcpy(buffer, &detail::radix_100_table[exp * 2], 2);
					buffer += 2;
				}
				else {
					*buffer = (char)('0' + exp);
					buffer += 1;
				}
			}

			return buffer;
		}
		
		char* to_chars_shortest_scientific_n_impl(unsigned_decimal_fp<float> v, char* buffer) {
			return to_chars_shortest_scientific_n_impl_impl(v, buffer);
		}
		char* to_chars_shortest_scientific_n_impl(unsigned_decimal_fp<double> v, char* buffer) {
			return to_chars_shortest_scientific_n_impl_impl(v, buffer);
		}
	}
}
