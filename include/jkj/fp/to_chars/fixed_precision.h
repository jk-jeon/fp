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
#include <cstring>	// std::memcpy, std::memset

namespace jkj::fp {
	namespace detail {
		
	}

	// Fixed-precision formatting in fixed-point form
	// precision means the number of digits after the decimal point.
	template <class Float>
	void to_chars_fixed_precision_fixed_point(Float x, char* buffer, int precision) noexcept {
		assert(precision >= 0);
	}

	// Fixed-precision formatting in scientific form
	// precision means the number of significand digits excluding the first digit.
	// This function does not null-terminate the buffer.
	//
	// NOTE: It should be easy enough to modify this function to be strictly single-pass.
	// It is in fact "almost" single-pass already.
	// Hence, we can in fact make buffer to be any forward iterator.
	// However, it seems that today's compilers are not smart enough to optimize well
	// the case when buffer is of type char* if we do that.
	// So I leave this function to take char* rather than a general iterator.
	template <class Float>
	char* to_chars_fixed_precision_scientific_n(Float x, char* buffer, int precision) noexcept {
		assert(precision >= 0);

		using ieee754_format_info = ieee754_format_info<ieee754_traits<Float>::format>;

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
						rp.compute_next_segment();
						next_digits_normalized = rp.current_segment();
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
				} // precision == 0
				else {
					char first_digit;
					std::uint32_t current_digits, normalizer;
					int current_digits_length;

					if (rp.current_segment() >= 1'0000'0000) {
						first_digit = char(rp.current_segment() / 1'0000'0000);
						current_digits = rp.current_segment() % 1'0000'0000;
						normalizer = 10;
						current_digits_length = 8;
					}
					else if (rp.current_segment() >= 1000'0000) {
						first_digit = char(rp.current_segment() / 1000'0000);
						current_digits = rp.current_segment() % 1000'0000;
						normalizer = 100;
						current_digits_length = 7;
					}
					else if (rp.current_segment() >= 100'0000) {
						first_digit = char(rp.current_segment() / 100'0000);
						current_digits = rp.current_segment() % 100'0000;
						normalizer = 1000;
						current_digits_length = 6;
					}
					else if (rp.current_segment() >= 10'0000) {
						first_digit = char(rp.current_segment() / 10'0000);
						current_digits = rp.current_segment() % 10'0000;
						normalizer = 1'0000;
						current_digits_length = 5;
					}
					else if (rp.current_segment() >= 1'0000) {
						first_digit = char(rp.current_segment() / 1'0000);
						current_digits = rp.current_segment() % 1'0000;
						normalizer = 10'0000;
						current_digits_length = 4;
					}
					else if (rp.current_segment() >= 1000) {
						first_digit = char(rp.current_segment() / 1000);
						current_digits = rp.current_segment() % 1000;
						normalizer = 100'0000;
						current_digits_length = 3;
					}
					else if (rp.current_segment() >= 100) {
						first_digit = char(rp.current_segment() / 100);
						current_digits = rp.current_segment() % 100;
						normalizer = 1000'0000;
						current_digits_length = 2;
					}
					else if (rp.current_segment() >= 10) {
						first_digit = char(rp.current_segment() / 10);
						current_digits = rp.current_segment() % 10;
						normalizer = 1'0000'0000;
						current_digits_length = 1;
					}
					else {
						first_digit = char(rp.current_segment());
						rp.compute_next_segment();
						current_digits = rp.current_segment();
						normalizer = 1;
						current_digits_length = 9;
					}
					exponent = current_digits_length - rp.current_segment_index() * 9;

					// If all the required digits were generated
					if (precision <= current_digits_length) {
						std::uint32_t remainder;

						if (precision < current_digits_length) {
							current_digits *= normalizer;

							auto case_handler = [&](auto const_holder) {
								constexpr auto e = decltype(const_holder)::value;
								constexpr auto divisor =
									detail::compute_power<9 - e>(std::uint32_t(10));
								constexpr auto remainder_normalizer =
									detail::compute_power<e>(std::uint32_t(10));

								remainder = (current_digits % divisor) * remainder_normalizer;
								current_digits /= divisor;
								normalizer = divisor;
							};

							switch (precision) {
							case 1:
								case_handler(std::integral_constant<int, 1>{});
								break;

							case 2:
								case_handler(std::integral_constant<int, 2>{});
								break;

							case 3:
								case_handler(std::integral_constant<int, 3>{});
								break;

							case 4:
								case_handler(std::integral_constant<int, 4>{});
								break;

							case 5:
								case_handler(std::integral_constant<int, 5>{});
								break;

							case 6:
								case_handler(std::integral_constant<int, 6>{});
								break;

							case 7:
								case_handler(std::integral_constant<int, 7>{});
								break;

							default:
								assert(precision == 8);
								case_handler(std::integral_constant<int, 8>{});
							}
						} // precision < current_digits_length
						else {
							rp.compute_next_segment();
							remainder = rp.current_segment();
						}

						// Determine rounding.
						if (remainder > 5'0000'0000 ||
							(remainder == 5'0000'0000 &&
								(current_digits % 2 != 0 || rp.has_further_nonzero_segments())))
						{
							if (normalizer * ++current_digits == 10'0000'0000) {
								if (++first_digit == 10) {
									++exponent;
									*buffer = '1';
									++buffer;
								}
								else {
									*buffer = char('0' + first_digit);
									++buffer;
								}
								*buffer = '.';
								++buffer;

								buffer = detail::print_zeros(buffer, precision);
								goto print_exponent_and_return_label;
							}
						}

						*buffer = char('0' + first_digit);
						++buffer;
						*buffer = '.';
						++buffer;

						buffer = detail::print_number(buffer, current_digits, precision);
					} // precision <= current_digits_length
					// If there are more digits to be generated
					else {
						int number_of_trailing_9;
						precision -= current_digits_length;
						rp.compute_next_segment();
						auto next_digits = rp.current_segment();

						// If the current digits are all 9's
						if ((current_digits + 1) * normalizer == 10'0000'0000) {
							number_of_trailing_9 = current_digits_length;

							// Scan until a digit other than 9 is found
							while (true) {
								assert(precision > 0);

								if (precision <= 9) {
									std::uint32_t remainder;

									auto case_handler = [&](auto const_holder) {
										constexpr auto e = decltype(const_holder)::value;
										constexpr auto divisor =
											detail::compute_power<9 - e>(std::uint32_t(10));
										constexpr auto remainder_normalizer =
											detail::compute_power<e>(std::uint32_t(10));

										remainder = (next_digits % divisor) * remainder_normalizer;
										next_digits /= divisor;
										normalizer = divisor;
									};

									switch (precision) {
									case 1:
										case_handler(std::integral_constant<int, 1>{});
										break;

									case 2:
										case_handler(std::integral_constant<int, 2>{});
										break;

									case 3:
										case_handler(std::integral_constant<int, 3>{});
										break;

									case 4:
										case_handler(std::integral_constant<int, 4>{});
										break;

									case 5:
										case_handler(std::integral_constant<int, 5>{});
										break;

									case 6:
										case_handler(std::integral_constant<int, 6>{});
										break;

									case 7:
										case_handler(std::integral_constant<int, 7>{});
										break;

									case 8:
										case_handler(std::integral_constant<int, 8>{});
										break;

									default:
										assert(precision == 9);
										rp.compute_next_segment();
										remainder = rp.current_segment();
										normalizer = 1;
									}

									// Determine rounding
									if (remainder > 5'0000'0000 ||
										(remainder == 5'0000'0000 &&
											(next_digits % 2 != 0 || rp.has_further_nonzero_segments())))
									{
										if (normalizer * ++next_digits == 10'0000'0000) {
											if (++first_digit == 10) {
												++exponent;
												*buffer = '1';
												++buffer;
											}
											else {
												*buffer = char('0' + first_digit);
												++buffer;
											}
											*buffer = '.';
											++buffer;

											buffer = detail::print_zeros(buffer, number_of_trailing_9 + precision);
											goto print_exponent_and_return_label;
										}
									}

									// Print digits
									*buffer = char('0' + first_digit);
									++buffer;
									*buffer = '.';
									++buffer;
									buffer = detail::print_nines(buffer, number_of_trailing_9);
									buffer = detail::print_number(buffer, next_digits, precision);
									goto print_exponent_and_return_label;
								}

								if (next_digits == 9'9999'9999) {
									number_of_trailing_9 += 9;
									precision -= 9;
									rp.compute_next_segment();
									next_digits = rp.current_segment();
								}
								else {
									break;
								}
							} // Scan until a digit other than 9 is found

							// Digits until next_digits can be safely printed
							*buffer = char('0' + first_digit);
							++buffer;
							*buffer = '.';
							++buffer;
							buffer = detail::print_nines(buffer, number_of_trailing_9);
						} // (current_digits + 1) * normalizer == 10'0000'0000
						// If the current digits are not all 9's
						else {
							// Print the first digit and the decimal dot
							*buffer = char('0' + first_digit);
							++buffer;
							*buffer = '.';
							++buffer;

							number_of_trailing_9 = 0;

							// Scan until a digit other than '9' is found
							while (true) {
								assert(precision > 0);

								if (precision <= 9) {
									std::uint32_t remainder;

									auto case_handler = [&](auto const_holder) {
										constexpr auto e = decltype(const_holder)::value;
										constexpr auto divisor =
											detail::compute_power<9 - e>(std::uint32_t(10));
										constexpr auto remainder_normalizer =
											detail::compute_power<e>(std::uint32_t(10));

										remainder = (next_digits % divisor) * remainder_normalizer;
										next_digits /= divisor;
										normalizer = divisor;
									};

									switch (precision) {
									case 1:
										case_handler(std::integral_constant<int, 1>{});
										break;

									case 2:
										case_handler(std::integral_constant<int, 2>{});
										break;

									case 3:
										case_handler(std::integral_constant<int, 3>{});
										break;

									case 4:
										case_handler(std::integral_constant<int, 4>{});
										break;

									case 5:
										case_handler(std::integral_constant<int, 5>{});
										break;

									case 6:
										case_handler(std::integral_constant<int, 6>{});
										break;

									case 7:
										case_handler(std::integral_constant<int, 7>{});
										break;

									case 8:
										case_handler(std::integral_constant<int, 8>{});
										break;

									default:
										assert(precision == 9);
										rp.compute_next_segment();
										remainder = rp.current_segment();
										normalizer = 1;
									}

									// Determine rounding
									if (remainder > 5'0000'0000 ||
										(remainder == 5'0000'0000 &&
											(next_digits % 2 != 0 || rp.has_further_nonzero_segments())))
									{
										if (normalizer * ++next_digits == 10'0000'0000) {
											++current_digits;
											buffer = detail::print_number(buffer, current_digits,
												current_digits_length);
											buffer = detail::print_zeros(buffer, number_of_trailing_9 + precision);
											goto print_exponent_and_return_label;
										}
									}

									// Print digits
									buffer = detail::print_number(buffer,
										current_digits, current_digits_length);
									buffer = detail::print_nines(buffer, number_of_trailing_9);
									buffer = detail::print_number(buffer, next_digits, precision);
									goto print_exponent_and_return_label;
								}

								if (next_digits == 9'9999'9999) {
									number_of_trailing_9 += 9;
									precision -= 9;
									rp.compute_next_segment();
									next_digits = rp.current_segment();
								}
								else {
									break;
								}
							}

							// Digits until next_digits can be safely printed
							buffer = detail::print_number(buffer, current_digits,
								current_digits_length);
							buffer = detail::print_nines(buffer, number_of_trailing_9);
						} // (current_digits + 1) * normalizer != 10'0000'0000

						assert(precision > 9);
						number_of_trailing_9 = 0;
						current_digits = next_digits;

						while (true) {
							precision -= 9;
							// If all nonzero segments are exhausted,
							if (!rp.compute_next_segment()) {
								// Print trailing 9's and 0's
								buffer = detail::print_nine_digits(buffer, current_digits);
								buffer = detail::print_nines(buffer, number_of_trailing_9);
								buffer = detail::print_zeros(buffer, precision);
								goto print_exponent_and_return_label;
							}

							next_digits = rp.current_segment();
							if (precision <= 9) {
								break;
							}

							if (next_digits == 9'9999'9999) {
								number_of_trailing_9 += 9;
							}
							else {
								buffer = detail::print_nine_digits(buffer, current_digits);
								buffer = detail::print_nines(buffer, number_of_trailing_9);
								number_of_trailing_9 = 0;
								current_digits = next_digits;
							}
						}

						// Print the last segment
						std::uint32_t remainder;

						auto case_handler = [&](auto const_holder) {
							constexpr auto e = decltype(const_holder)::value;
							constexpr auto divisor =
								detail::compute_power<9 - e>(std::uint32_t(10));
							constexpr auto remainder_normalizer =
								detail::compute_power<e>(std::uint32_t(10));

							remainder = (next_digits % divisor) * remainder_normalizer;
							next_digits /= divisor;
							normalizer = divisor;
						};

						switch (precision) {
						case 1:
							case_handler(std::integral_constant<int, 1>{});
							break;

						case 2:
							case_handler(std::integral_constant<int, 2>{});
							break;

						case 3:
							case_handler(std::integral_constant<int, 3>{});
							break;

						case 4:
							case_handler(std::integral_constant<int, 4>{});
							break;

						case 5:
							case_handler(std::integral_constant<int, 5>{});
							break;

						case 6:
							case_handler(std::integral_constant<int, 6>{});
							break;

						case 7:
							case_handler(std::integral_constant<int, 7>{});
							break;

						case 8:
							case_handler(std::integral_constant<int, 8>{});
							break;

						default:
							assert(precision == 9);
							rp.compute_next_segment();
							remainder = rp.current_segment();
							normalizer = 1;
						}

						// Determine rounding
						if (remainder > 5'0000'0000 ||
							(remainder == 5'0000'0000 &&
								(next_digits % 2 != 0 || rp.has_further_nonzero_segments())))
						{
							if (normalizer * ++next_digits == 10'0000'0000) {
								++current_digits;
								assert(current_digits < 10'0000'0000);
								buffer = detail::print_nine_digits(buffer, current_digits);
								buffer = detail::print_zeros(buffer, number_of_trailing_9 + precision);
								goto print_exponent_and_return_label;
							}
						}

						// Print digits
						buffer = detail::print_nine_digits(buffer, current_digits);
						buffer = detail::print_nines(buffer, number_of_trailing_9);
						buffer = detail::print_number(buffer, next_digits, precision);
					} // precision > current_digits_length
				} // precision != 0

				// Print the exponent and return.
			print_exponent_and_return_label:
				if (exponent < 0) {
					exponent = -exponent;
					std::memcpy(buffer, "e-", 2);
					buffer += 2;
				}
				else {
					std::memcpy(buffer, "e+", 2);
					buffer += 2;
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
					else {
						std::memcpy(buffer, &detail::radix_100_table[uexp * 2], 2);
						buffer += 2;
					}
					return buffer;
				}
				else {
					static_assert(ieee754_format_info::format == ieee754_format::binary32);
					assert(exponent < 100);
					std::memcpy(buffer, &detail::radix_100_table[exponent * 2], 2);
					buffer += 2;
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
					return detail::print_zeros(buffer, precision);
				}
			}
		}
		else {
			if (br.is_negative()) {
				*buffer = '-';
				++buffer;
			}

			if ((br.u << (ieee754_format_info::exponent_bits + 1)) != 0)
			{
				std::memcpy(buffer, "nan", 3);
				return buffer + 3;
			}
			else {				
				std::memcpy(buffer, "Infinity", 8);
				return buffer + 8;
			}
		}
	}

	// Same as to_chars_fixed_precision_scientific, but null-terminates the buffer.
	// Returns the pointer to the added null character.
	template <class Float>
	char* to_chars_fixed_precision_scientific(Float x, char* buffer, int precision) noexcept {
		auto ptr = to_chars_fixed_precision_scientific_n(x, buffer, precision);
		*ptr = '\0';
		return ptr;
	}
}

#endif