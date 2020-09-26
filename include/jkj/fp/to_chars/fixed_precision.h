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

#ifndef JKJ_HEADER_FP_TO_CHARS_FIXED_PRECISION
#define JKJ_HEADER_FP_TO_CHARS_FIXED_PRECISION

#include "../ryu_printf.h"
#include "to_chars_common.h"
#include <cassert>
#include <cstdint>
#ifndef NDEBUG
#include <cmath>	// std::pow
#endif

namespace jkj::fp {
	// Fixed-precision formatting in fixed-point form
	// precision means the number of digits after the decimal point.
	template <class Float>
	void to_chars_fixed_precision_fixed_point(Float x, char* buffer, int precision) noexcept {
		assert(precision >= 0);
	}

	// Fixed-precision formatting in scientific form
	// precision means the number of significand digits excluding the first digit.
	template <class Float>
	char* to_chars_fixed_precision_scientific(Float x, char* buffer, int precision) noexcept {
		assert(precision >= 0);

		using ieee754_format_info = ieee754_format_info<ieee754_traits<Float>::format>;

		// Some common routines
		auto print_zeros = [&](int number_of_zeros) {
			while (number_of_zeros >= 8) {
				std::memcpy(buffer, "00000000", 8);
				buffer += 8;
				number_of_zeros -= 8;
			}
			if (number_of_zeros >= 4) {
				std::memcpy(buffer, "0000", 4);
				buffer += 4;
				number_of_zeros -= 4;
			}
			if (number_of_zeros >= 2) {
				std::memcpy(buffer, "00", 2);
				buffer += 2;
				number_of_zeros -= 2;
			}
			if (number_of_zeros != 0) {
				*buffer = '0';
				++buffer;
			}
		};
		auto print_number = [&](std::uint32_t number, int length) {
			auto total_increment = length;
			while (length > 4) {
				auto c = number % 1'0000;
				number /= 1'0000;
				std::memcpy(buffer + length - 4,
					&detail::radix_100_table[(c / 100) * 2], 2);
				std::memcpy(buffer + length - 2,
					&detail::radix_100_table[(c % 100) * 2], 2);
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
			buffer += total_increment;
		};

		// Take care of special cases.
		auto br = ieee754_bits(x);
		if (br.is_finite()) {
			if (br.is_negative()) {
				*buffer = '-';
				++buffer;
			}
			if (br.is_nonzero()) {
				// The main case
				static_assert(ryu_printf<Float>::segment_size == 9);
				ryu_printf<Float> rp{ br };
				int exponent;

				if (precision == 0) {
					// We only need the first digit and rounding information.
					char first_digit;
					std::uint32_t next_digits_normalized;
					if (rp.current_segment() >= 1'0000'0000) {
						first_digit = char(rp.current_segment() / 1'0000'0000);
						next_digits_normalized = (rp.current_segment() % 1'0000'0000) * 10;
						exponent = 8 - rp.current_segment_index() * 9;
					}
					else if (rp.current_segment() >= 1000'0000) {
						first_digit = char(rp.current_segment() / 1000'0000);
						next_digits_normalized = (rp.current_segment() % 1000'0000) * 100;
						exponent = 7 - rp.current_segment_index() * 9;
					}
					else if (rp.current_segment() >= 100'0000) {
						first_digit = char(rp.current_segment() / 100'0000);
						next_digits_normalized = (rp.current_segment() % 100'0000) * 1000;
						exponent = 6 - rp.current_segment_index() * 9;
					}
					else if (rp.current_segment() >= 10'0000) {
						first_digit = char(rp.current_segment() / 10'0000);
						next_digits_normalized = (rp.current_segment() % 10'0000) * 1'0000;
						exponent = 5 - rp.current_segment_index() * 9;
					}
					else if (rp.current_segment() >= 1'0000) {
						first_digit = char(rp.current_segment() / 1'0000);
						next_digits_normalized = (rp.current_segment() % 1'0000) * 10'0000;
						exponent = 4 - rp.current_segment_index() * 9;
					}
					else if (rp.current_segment() >= 1000) {
						first_digit = char(rp.current_segment() / 1000);
						next_digits_normalized = (rp.current_segment() % 1000) * 100'0000;
						exponent = 3 - rp.current_segment_index() * 9;
					}
					else if (rp.current_segment() >= 100) {
						first_digit = char(rp.current_segment() / 100);
						next_digits_normalized = (rp.current_segment() % 100) * 1000'0000;
						exponent = 2 - rp.current_segment_index() * 9;
					}
					else if (rp.current_segment() >= 10) {
						first_digit = char(rp.current_segment() / 10);
						next_digits_normalized = (rp.current_segment() % 10) * 1'0000'0000;
						exponent = 1 - rp.current_segment_index() * 9;
					}
					else {
						first_digit = char(rp.current_segment());
						next_digits_normalized = rp.compute_next_segment();
						exponent = 9 - rp.current_segment_index() * 9;
					}

					// Determine rounding.
					if (next_digits_normalized > 5'0000'0000 ||
						(next_digits_normalized == 5'0000'0000 &&
							(first_digit % 2 != 0 || rp.has_further_nonzero_segments())))
					{
						if (++first_digit == 10) {
							*buffer = '1';
							++buffer;
							++exponent;
							goto print_exponent_and_return_label;
						}
					}
					*buffer = char('0' + first_digit);
					++buffer;
				}
				else {
					char first_digit;
					bool first_digit_printed;
					std::uint32_t last_non_9_digits, normalizer;
					int last_non_9_digits_length;

					if (rp.current_segment() >= 1'0000'0000) {
						first_digit = char(rp.current_segment() / 1'0000'0000);
						last_non_9_digits = rp.current_segment() % 1'0000'0000;
						normalizer = 10;
						last_non_9_digits_length = 8;
					}
					else if (rp.current_segment() >= 1000'0000) {
						first_digit = char(rp.current_segment() / 1000'0000);
						last_non_9_digits = rp.current_segment() % 1000'0000;
						normalizer = 100;
						last_non_9_digits_length = 7;
					}
					else if (rp.current_segment() >= 100'0000) {
						first_digit = char(rp.current_segment() / 100'0000);
						last_non_9_digits = rp.current_segment() % 100'0000;
						normalizer = 1000;
						last_non_9_digits_length = 6;
					}
					else if (rp.current_segment() >= 10'0000) {
						first_digit = char(rp.current_segment() / 10'0000);
						last_non_9_digits = rp.current_segment() % 10'0000;
						normalizer = 1'0000;
						last_non_9_digits_length = 5;
					}
					else if (rp.current_segment() >= 1'0000) {
						first_digit = char(rp.current_segment() / 1'0000);
						last_non_9_digits = rp.current_segment() % 1'0000;
						normalizer = 10'0000;
						last_non_9_digits_length = 4;
					}
					else if (rp.current_segment() >= 1000) {
						first_digit = char(rp.current_segment() / 1000);
						last_non_9_digits = rp.current_segment() % 1000;
						normalizer = 100'0000;
						last_non_9_digits_length = 3;
					}
					else if (rp.current_segment() >= 100) {
						first_digit = char(rp.current_segment() / 100);
						last_non_9_digits = rp.current_segment() % 100;
						normalizer = 1000'0000;
						last_non_9_digits_length = 2;
					}
					else if (rp.current_segment() >= 10) {
						first_digit = char(rp.current_segment() / 10);
						last_non_9_digits = rp.current_segment() % 10;
						normalizer = 1'0000'0000;
						last_non_9_digits_length = 1;
					}
					else {
						first_digit = char(rp.current_segment());
						last_non_9_digits = rp.compute_next_segment();
						normalizer = 1;
						last_non_9_digits_length = 9;
					}
					exponent = last_non_9_digits_length - rp.current_segment_index() * 9;

					// Count trailing 9's.
					int number_of_trailing_9 = 0;
					while (last_non_9_digits % 10 == 9) {
						last_non_9_digits /= 10;
						++number_of_trailing_9;
						normalizer *= 10;
					}

					// Print the first digit if it cannot be altered by rounding.
					if (number_of_trailing_9 < last_non_9_digits_length) {
						buffer[0] = char('0' + first_digit);
						buffer[1] = '.';
						buffer += 2;
						first_digit_printed = true;
					}
					else {
						first_digit_printed = false;
					}
					last_non_9_digits_length -= number_of_trailing_9;

					while (precision > last_non_9_digits_length + number_of_trailing_9) {
						auto next_segment = rp.compute_next_segment();

						if (next_segment == 9'9999'9999) {
							number_of_trailing_9 += 9;
						}
						else {
							// Previous digits can be now safely printed.
							precision -= (last_non_9_digits_length + number_of_trailing_9);

							// The first digit
							if (!first_digit_printed) {
								buffer[0] = char('0' + first_digit);
								buffer[1] = '.';
								buffer += 2;
								first_digit_printed = true;
							}

							// Non-9 digits
							print_number(last_non_9_digits, last_non_9_digits_length);

							// Trailing 9's
							while (number_of_trailing_9 >= 8) {
								std::memcpy(buffer, "99999999", 8);
								buffer += 8;
								number_of_trailing_9 -= 8;
							}
							if (number_of_trailing_9 >= 4) {
								std::memcpy(buffer, "9999", 4);
								buffer += 4;
								number_of_trailing_9 -= 4;
							}
							if (number_of_trailing_9 >= 2) {
								std::memcpy(buffer, "99", 4);
								buffer += 2;
								number_of_trailing_9 -= 2;
							}
							if (number_of_trailing_9 != 0) {
								*buffer = '9';
								++buffer;
							}

							// Reset variables.
							last_non_9_digits = next_segment;
							number_of_trailing_9 = 0;
							normalizer = 1;
							while (last_non_9_digits % 10 == 9) {
								last_non_9_digits /= 10;
								++number_of_trailing_9;
								normalizer *= 10;
							}
							last_non_9_digits_length = 9 - number_of_trailing_9;
						}
					}

					if (precision == last_non_9_digits_length + number_of_trailing_9) {
						auto remainder = rp.compute_next_segment();

						if (number_of_trailing_9 != 0) {
							// Determine rounding.
							if (remainder >= 5'0000'0000) {
								// Perform round-up.

								// If digits are all 9's,
								if (last_non_9_digits_length == 0) {
									if (!first_digit_printed) {
										if (++first_digit == 10) {
											std::memcpy(buffer, "1.", 2);
											++exponent;
										}
										else {
											buffer[0] = char('0' + first_digit);
											buffer[1] = '.';
										}
										buffer += 2;
									}
								}
								// Otherwise,
								else {
									++last_non_9_digits;
									assert(last_non_9_digits < std::pow(10, last_non_9_digits_length));
									assert(first_digit_printed);
									print_number(last_non_9_digits, last_non_9_digits_length);
								}
								print_zeros(number_of_trailing_9);
							}
							else {
								// Non-9 digits
								print_number(last_non_9_digits, last_non_9_digits_length);

								// Trailing 9's
								while (number_of_trailing_9 >= 8) {
									std::memcpy(buffer, "99999999", 8);
									buffer += 8;
									number_of_trailing_9 -= 8;
								}
								if (number_of_trailing_9 >= 4) {
									std::memcpy(buffer, "9999", 4);
									buffer += 4;
									number_of_trailing_9 -= 4;
								}
								if (number_of_trailing_9 >= 2) {
									std::memcpy(buffer, "99", 4);
									buffer += 2;
									number_of_trailing_9 -= 2;
								}
								if (number_of_trailing_9 != 0) {
									*buffer = '9';
									++buffer;
								}
							}
						}
						else {
							assert(first_digit_printed);

							// Determine rounding.
							if (remainder > 5'0000'0000 ||
								(remainder == 5'0000'0000 &&
									(last_non_9_digits % 2 != 0 || rp.has_further_nonzero_segments())))
							{
								++last_non_9_digits;
							}
							print_number(last_non_9_digits, last_non_9_digits_length);
						}
					}
					else if (precision >= last_non_9_digits_length) {
						// The next digit after the last digit is 9;
						// thus, perform round-up.

						// This condition means every digit after the first digit were 9.
						if (!first_digit_printed) {
							assert(last_non_9_digits_length == 0);
							if (++first_digit == 10) {
								std::memcpy(buffer, "1.", 2);
								++exponent;
							}
							else {
								buffer[0] = char('0' + first_digit);
								buffer[1] = '.';
							}
							buffer += 2;
						}
						// Otherwise, print non-9 digits.
						else {
							++last_non_9_digits;
							assert(last_non_9_digits * normalizer <= 9'9999'9999);
							precision -= last_non_9_digits_length;
							print_number(last_non_9_digits, last_non_9_digits_length);
						}

						// Print trailing 0's.
						print_zeros(precision);
					}
					else {
						assert(precision < last_non_9_digits_length);
						last_non_9_digits *= normalizer;
						assert(last_non_9_digits <= 9'9999'9999);

						std::uint32_t remainder;
						switch (precision) {
						case 1:
							remainder = (last_non_9_digits % 1'0000'0000) * 10;
							last_non_9_digits /= 1'0000'0000;
							break;

						case 2:
							remainder = (last_non_9_digits % 1000'0000) * 100;
							last_non_9_digits /= 1000'0000;
							break;

						case 3:
							remainder = (last_non_9_digits % 100'0000) * 1000;
							last_non_9_digits /= 100'0000;
							break;

						case 4:
							remainder = (last_non_9_digits % 10'0000) * 1'0000;
							last_non_9_digits /= 10'0000;
							break;

						case 5:
							remainder = (last_non_9_digits % 1'0000) * 10'0000;
							last_non_9_digits /= 1'0000;
							break;

						case 6:
							remainder = (last_non_9_digits % 1000) * 100'0000;
							last_non_9_digits /= 1000;
							break;

						case 7:
							remainder = (last_non_9_digits % 100) * 1000'0000;
							last_non_9_digits /= 100;
							break;

						default:
							assert(precision == 8);
							remainder = (last_non_9_digits % 10) * 1'0000'0000;
							last_non_9_digits /= 10;
						}
						last_non_9_digits_length = precision;

						// Determine rounding.
						if (remainder > 5'0000'0000 ||
							(remainder == 5'0000'0000 &&
								(last_non_9_digits % 2 != 0 || rp.has_further_nonzero_segments())))
						{
							++last_non_9_digits;
							assert(last_non_9_digits < std::pow(10, last_non_9_digits_length));
						}

						assert(first_digit_printed);
						print_number(last_non_9_digits, last_non_9_digits_length);
					}
				}

				// Print the exponent and return.
			print_exponent_and_return_label:
				if (exponent < 0) {
					exponent = -exponent;
					std::memcpy(buffer, "E-", 2);
					buffer += 2;
				}
				else {
					*buffer = 'E';
					++buffer;
				}
				if constexpr (ieee754_format_info::format == ieee754_format::binary64) {
					auto uexp = unsigned(exponent);
					assert(uexp < 1000);
					if (uexp >= 100) {
						std::memcpy(buffer, &detail::radix_100_table[(uexp / 10) * 2], 2);
						buffer += 2;
						*buffer = char('0' + (uexp % 10));
						++buffer;
					}
					else if (uexp >= 10) {
						std::memcpy(buffer, &detail::radix_100_table[uexp * 2], 2);
						buffer += 2;
					}
					else {
						*buffer = char('0' + uexp);
						++buffer;
					}
					return buffer;
				}
				else {
					static_assert(ieee754_format_info::format == ieee754_format::binary32);
					assert(exponent < 100);
					if (exponent >= 10) {
						std::memcpy(buffer, &detail::radix_100_table[exponent * 2], 2);
						buffer += 2;
					}
					else {
						*buffer = char('0' + exponent);
						++buffer;
					}
					return buffer;
				}
			}
			else {
				if (precision == 0) {
					std::memcpy(buffer, "0", 1);
					return buffer + 1;
				}
				else {
					std::memcpy(buffer, "0.", 1);
					buffer += 2;
					
					while (precision >= 8) {
						std::memcpy(buffer, "00000000", 8);
						buffer += 8;
						precision -= 4;
					}
					if (precision >= 4) {
						std::memcpy(buffer, "0000", 4);
						buffer += 4;
						precision -= 4;
					}
					if (precision >= 2) {
						std::memcpy(buffer, "00", 2);
						buffer += 2;
						precision -= 2;
					}
					if (precision != 1) {
						*buffer = '0';
						++buffer;
					}
					return buffer;
				}
			}
		}
		else {
			if ((br.u << (ieee754_format_info::exponent_bits + 1)) != 0)
			{
				std::memcpy(buffer, "NaN", 3);
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
}

#endif