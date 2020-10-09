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

#ifndef JKJ_HEADER_FP_TO_CHARS_SHORTEST_PRECISE
#define JKJ_HEADER_FP_TO_CHARS_SHORTEST_PRECISE

#include "../ryu_printf.h"
#include "../detail/bits.h"
#include "to_chars_common.h"

namespace jkj::fp {
	// Returns the next-to-end position.
	template <class Float>
	char* to_chars_precise_scientific_n(Float x, char* buffer)
	{
		using ieee754_format_info = ieee754_format_info<ieee754_traits<Float>::format>;

		auto br = ieee754_bits(x);
		if (br.is_finite()) {
			if (br.is_negative()) {
				*buffer = '-';
				++buffer;
			}
			if (br.is_nonzero()) {
				// Print the exact value using Ryu-printf
				ryu_printf<Float> digit_gen{ x };
				static_assert(ryu_printf<Float>::segment_size == 9);
				int exponent =
					-digit_gen.current_segment_index() * ryu_printf<Float>::segment_size;
				std::uint32_t current_segment;
				bool has_more_segments;

				auto case_handler = [&](auto holder) {
					constexpr auto length = decltype(holder)::value;
					constexpr auto divisor = detail::compute_power<length - 1>(std::uint32_t(10));
					buffer[0] = char('0' + digit_gen.current_segment() / divisor);
					buffer[1] = '.';
					buffer += 2;

					current_segment = digit_gen.current_segment() % divisor;
					has_more_segments = digit_gen.compute_next_segment();

					if (has_more_segments) {
						buffer = detail::print_number(buffer, current_segment, length - 1);
						current_segment = digit_gen.current_segment();
						has_more_segments = digit_gen.compute_next_segment();
					}
					else {
						// Normalize.
						constexpr auto normalizer = detail::compute_power<
							ryu_printf<Float>::segment_size - length + 1>(std::uint32_t(10));
						current_segment *= normalizer;
						assert(current_segment <= 9'9999'9999);
					}
					exponent += length - 1;
				};
				if (digit_gen.current_segment() >= 1'0000'0000) {
					case_handler(std::integral_constant<int, 9>{});
				}
				else if (digit_gen.current_segment() >= 1000'0000) {
					case_handler(std::integral_constant<int, 8>{});
				}
				else if (digit_gen.current_segment() >= 100'0000) {
					case_handler(std::integral_constant<int, 7>{});
				}
				else if (digit_gen.current_segment() >= 10'0000) {
					case_handler(std::integral_constant<int, 6>{});
				}
				else if (digit_gen.current_segment() >= 1'0000) {
					case_handler(std::integral_constant<int, 5>{});
				}
				else if (digit_gen.current_segment() >= 1000) {
					case_handler(std::integral_constant<int, 4>{});
				}
				else if (digit_gen.current_segment() >= 100) {
					case_handler(std::integral_constant<int, 3>{});
				}
				else if (digit_gen.current_segment() >= 10) {
					case_handler(std::integral_constant<int, 2>{});
				}
				else {
					buffer[0] = char('0' + digit_gen.current_segment());
					
					has_more_segments = digit_gen.compute_next_segment();
					if (has_more_segments) {
						buffer[1] = '.';
						buffer += 2;
						current_segment = digit_gen.current_segment();
						has_more_segments = digit_gen.compute_next_segment();
					}
					else {
						return buffer + 1;
					}					
				}

				while (has_more_segments) {
					buffer = detail::print_nine_digits(buffer, current_segment);
					current_segment = digit_gen.current_segment();
					has_more_segments = digit_gen.compute_next_segment();
				}

				// Remove trailing zeros
				auto t = detail::bits::countr_zero(current_segment);
				if (t > ryu_printf<Float>::segment_size) {
					t = ryu_printf<Float>::segment_size;
				}

				constexpr auto const& divtable =
					detail::div::table_holder<std::uint32_t, 5, ryu_printf<Float>::segment_size>::table;

				int s = 0;
				for (; s < t - 1; s += 2) {
					if (current_segment * divtable[2].mod_inv > divtable[2].max_quotient) {
						break;
					}
					current_segment *= divtable[2].mod_inv;
				}
				if (s < t && current_segment * divtable[1].mod_inv <= divtable[1].max_quotient)
				{
					current_segment *= divtable[1].mod_inv;
					++s;
				}
				current_segment >>= s;
				
				// Print the last segment
				buffer = detail::print_number(buffer, current_segment, ryu_printf<Float>::segment_size - s);

				// Print exponent
				if (exponent < 0) {
					std::memcpy(buffer, "e-", 2);
					exponent = -exponent;
				}
				else {
					std::memcpy(buffer, "e+", 2);
				}
				buffer += 2;
				buffer = detail::print_number(buffer, std::uint32_t(exponent),
					ieee754_traits<Float>::format == ieee754_format::binary32 ? 2 : 3);

				return buffer;
			}
			else {
				std::memcpy(buffer, "0e0", 3);
				return buffer + 3;
			}
		}
		else {
			if ((br.u << (ieee754_format_info::exponent_bits + 1)) != 0)
			{
				std::memcpy(buffer, "nan", 3);
				return buffer + 3;
			}
			else {
				if (br.is_negative()) {
					*buffer = '-';
					++buffer;
				}
				std::memcpy(buffer, "Infinity", 8);
				return buffer + 8;
			}
		}
	}

	// Null-terminates and bypass the return value of fp_to_chars_n.
	template <class Float>
	char* to_chars_precise_scientific(Float x, char* buffer)
	{
		auto ptr = to_chars_precise_scientific_n(x, buffer);
		*ptr = '\0';
		return ptr;
	}
}

#include "../detail/undef_macros.h"
#endif

