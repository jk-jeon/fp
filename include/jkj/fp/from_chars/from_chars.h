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

#ifndef JKJ_HEADER_FP_FROM_CHARS_FROM_CHARS
#define JKJ_HEADER_FP_FROM_CHARS_FROM_CHARS

#include "../dooly.h"
#include "../ryu_printf.h"
#include "../to_chars/to_chars_common.h"
#include <cassert>

namespace jkj::fp {
	// This function is VERY primitive; it does not offer any proper parse error checking,
	// and it might even accept some malformed inputs.
	template <class Float>
	ieee754_bits<Float> from_chars_limited(char const* begin, char const* end)
	{
		using carrier_uint = typename ieee754_traits<Float>::carrier_uint;
		[[maybe_unused]] constexpr auto digit_limit =
			jkj::fp::to_binary_limited_precision_digit_limit<jkj::fp::ieee754_traits<Float>::format>;

		assert(begin != end);

		jkj::fp::signed_decimal_fp<Float> decimal;
		decimal.exponent = 0;
		int digits = 0;
		char const* next_to_decimal_dot_pos;
		if (*begin == '-') {
			decimal.is_negative = true;
			++begin;
			assert(begin != end);
		}
		else {
			decimal.is_negative = false;
			if (*begin == '+') {
				++begin;
				assert(begin != end);
			}
		}

		if (*begin == '.') {
			next_to_decimal_dot_pos = ++begin;
			assert(begin != end);
			decimal.significand = 0;
			goto after_decimal_point_label;
		}
		else if (*begin == '0') {
			++begin;
			decimal.significand = 0;
			if (begin == end) {
				goto convert_to_binary_label;
			}
			else if (*begin == '.') {
				next_to_decimal_dot_pos = ++begin;
				goto after_decimal_point_label;
			}
			else if (*begin == 'e' || *begin == 'E') {
				++begin;
				goto after_e_label;
			}
			else {
				// Should not reach here.
				assert(false);
			}
		}
		else {
			assert(*begin >= '1' && *begin <= '9');
			digits = 1;
			decimal.significand = carrier_uint(*begin - '0');
			++begin;
		}

		for (; begin != end; ++begin) {
			if (*begin == '.') {
				next_to_decimal_dot_pos = ++begin;
				goto after_decimal_point_label;
			}
			else if (*begin == 'e' || *begin == 'E') {
				++begin;
				goto after_e_label;
			}
			else {
				assert(*begin >= '0' && *begin <= '9');
				++digits;
				assert(digits <= digit_limit);
				decimal.significand *= 10;
				decimal.significand += carrier_uint(*begin - '0');
			}
		}
		goto convert_to_binary_label;

	after_decimal_point_label:
		for (; begin != end; ++begin) {
			if (*begin == 'e' || *begin == 'E') {
				decimal.exponent -= int(begin - next_to_decimal_dot_pos);
				++begin;
				goto after_e_label;
			}
			else {
				assert(*begin >= '0' && *begin <= '9');
				++digits;
				assert(digits <= digit_limit);
				decimal.significand *= 10;
				decimal.significand += carrier_uint(*begin - '0');
			}
		}
		decimal.exponent -= int(begin - next_to_decimal_dot_pos);
		goto convert_to_binary_label;

	after_e_label:
		{
			assert(begin != end);
			bool has_negative_exponent = false;
			if (*begin == '-') {
				has_negative_exponent = true;
				++begin;
				assert(begin != end);
			}
			else if (*begin == '+') {
				++begin;
				assert(begin != end);
			}

			int exp = 0;
			for (; begin != end; ++begin) {
				assert(*begin >= '0' && *begin <= '9');
				exp *= 10;
				exp += int(*begin - '0');
			}
			if (has_negative_exponent) {
				decimal.exponent -= exp;
			}
			else {
				decimal.exponent += exp;
			}
		}

	convert_to_binary_label:
		return to_binary_limited_precision(decimal);
	}

	// This function is VERY primitive; it does not offer any proper parse error checking,
	// and it might even accept some malformed inputs.
	template <class Float>
	ieee754_bits<Float> from_chars_unlimited(char const* begin, char const* end)
	{
		char const* decimal_dot_pos = end;
		char const* significand_end_pos = end;
		bool is_negative;
		int exponent = 0;

		char const* ptr = begin;
		assert(ptr != end);
		if (*ptr == '-') {
			is_negative = true;
			++ptr;
			++begin;
			assert(ptr != end);
		}
		else if (*ptr == '+') {
			is_negative = false;
			++ptr;
			++begin;
			assert(ptr != end);
		}
		else {
			is_negative = false;
		}

		// Scan significand digits.
		bool first_nonzero_digit_found = false;
		while (ptr != end) {
			if (*ptr == '.') {
				assert(decimal_dot_pos == end);
				decimal_dot_pos = ptr;
			}
			else if (*ptr == 'e' || *ptr == 'E') {
				significand_end_pos = ptr;
				if (++ptr == end) {
					break;
				}
				goto compute_exponent_label;
			}
			else {
				assert(*ptr >= '0' && *ptr <= '9');
				if (!first_nonzero_digit_found) {
					if (*ptr != '0') {
						first_nonzero_digit_found = true;
					}
					else {
						++begin;
					}
				}
			}
			++ptr;
		}
		goto start_parsing_label;

		// Compute exponent.
	compute_exponent_label:
		{
			bool negative_exponent;
			if (*ptr == '-') {
				negative_exponent = true;
				++ptr;
			}
			else if (*ptr == '+') {
				negative_exponent = false;
				++ptr;
			}
			else {
				negative_exponent = false;
			}
			while (ptr != end) {
				assert(*ptr >= '0' && *ptr <= '9');
				exponent *= 10;
				exponent += (*ptr - '0');
				++ptr;

				if (exponent >= 1000) {
					if (negative_exponent) {
						if (is_negative) {
							return ieee754_bits<Float>{ ieee754_traits<Float>::negative_zero() };
						}
						else {
							return ieee754_bits<Float>{ ieee754_traits<Float>::positive_zero() };
						}
					}
					else {
						if (is_negative) {
							return ieee754_bits<Float>{ ieee754_traits<Float>::negative_infinity() };
						}
						else {
							return ieee754_bits<Float>{ ieee754_traits<Float>::positive_infinity() };
						}
					}
				}
			}

			if (negative_exponent) {
				exponent = -exponent;
			}
		}

	start_parsing_label:
		constexpr auto digit_limit = 
			to_binary_limited_precision_digit_limit<ieee754_traits<Float>::format>;

		// Normalize
		if (decimal_dot_pos != end) {
			exponent += int((decimal_dot_pos - begin) - digit_limit);
		}
		else {
			exponent += int((significand_end_pos - begin) - digit_limit);
		}
		if (begin >= decimal_dot_pos) {
			++begin;
		}

		// Read initial segments.
		ptr = begin;
		using carrier_uint = typename ieee754_traits<Float>::carrier_uint;
		carrier_uint significand = 0;
		if constexpr (ieee754_format_info<ieee754_traits<Float>::format>::format == ieee754_format::binary32)
		{
			for (int i = 0; i < digit_limit; ++i) {
				significand *= 10;

				if (*ptr == '.') {
					++ptr;
				}
				if (ptr != significand_end_pos) {
					assert(*ptr >= '0' && *ptr <= '9');
					significand += (*ptr - '0');
					++ptr;
				}
			}
		}
		else {
			// Avoid 64-bit multiplications.
			static_assert(digit_limit <= 18);

			std::uint32_t significand32 = 0;
			for (int i = 0; i < digit_limit - 9; ++i) {
				significand32 *= 10;

				if (*ptr == '.') {
					++ptr;
				}
				if (ptr != significand_end_pos) {
					assert(*ptr >= '0' && *ptr <= '9');
					significand32 += (*ptr - '0');
					++ptr;
				}
			}
			significand = carrier_uint(10'0000'0000) * significand32;
			significand32 = 0;
			for (int i = 0; i < 9; ++i) {
				significand32 *= 10;

				if (*ptr == '.') {
					++ptr;
				}
				if (ptr != significand_end_pos) {
					assert(*ptr >= '0' && *ptr <= '9');
					significand32 += (*ptr - '0');
					++ptr;
				}
			}
			significand += significand32;
		}		

		// Compute the initial guess.
		auto f = to_binary_limited_precision(unsigned_decimal_fp<Float>{ significand, exponent });
		if (is_negative) {
			f.u |= ieee754_traits<Float>::negative_zero();
		}

		if (ptr == significand_end_pos) {
			return f;
		}

		// Generate digits of the middle point.
		ryu_printf<Float> digit_gen{ f, std::bool_constant<true>{} };

		// Compare g (significand) times 10^(k + n * eta) to s (the boundary point),
		// where k is the decimal exponent and n is the segment index.
		// Let d be the number of digits of g, then:
		//  - if k + n * eta + d - 1 >= eta, then g * 10^(k + n * eta) > s.
		//  - if k + n * eta + d <= 0, then g * 10^(k + n * eta) < s.
		//  - the above two conditions are not met if and only if
		//    0 <= k + n * eta + d - 1 < eta.
		auto initial_comparison_digits = exponent + digit_limit
			+ digit_gen.current_segment_index() * ryu_printf<Float>::segment_size;

		if (initial_comparison_digits <= 0) {
			// Boundary point is strictly greater.
			return f;
		}
		else if (initial_comparison_digits > ryu_printf<Float>::segment_size) {
			// Boundary point is strictly smaller.
			++f.u;
			return f;
		}
		else {
			static_assert(digit_limit >= ryu_printf<Float>::segment_size);

			ptr = begin;
			std::uint32_t significand32 = 0;
			for (int i = 0; i < initial_comparison_digits; ++i) {
				significand32 *= 10;

				if (*ptr == '.') {
					++ptr;
				}
				if (ptr != significand_end_pos) {
					assert(*ptr >= '0' && *ptr <= '9');
					significand32 += (*ptr - '0');
					++ptr;
				}
			}

			if (significand32 > digit_gen.current_segment()) {
				// Boundary point is strictly smaller.
				++f.u;
				return f;
			}
			else if (significand32 < digit_gen.current_segment()) {
				// Boundary point is strictly greater.
				return f;
			}
		}

		while (ptr != significand_end_pos) {
			digit_gen.compute_next_segment();

			std::uint32_t significand32 = 0;
			for (int i = 0; i < ryu_printf<Float>::segment_size; ++i) {
				significand32 *= 10;

				if (*ptr == '.') {
					++ptr;
				}
				if (ptr != significand_end_pos) {
					assert(*ptr >= '0' && *ptr <= '9');
					significand32 += (*ptr - '0');
					++ptr;
				}
			}

			if (significand32 > digit_gen.current_segment()) {
				// Boundaryle point is strictly smaller.
				++f.u;
				return f;
			}
			else if (significand32 < digit_gen.current_segment()) {
				// Boundary point is strictly bigger.
				return f;
			}
		}

		if (digit_gen.has_further_nonzero_segments()) {
			// Boundary point is strictly bigger.
			return f;
		}

		// Tie!
		if (f.u % 2 != 0) {
			++f.u;
		}

		return f;
	}
}

#endif