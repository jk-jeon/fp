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
#include "../detail/macros.h"
#include <cassert>
#include <cstdint>
#include <cstring>	// std::memcpy, std::memset

namespace jkj::fp {
	namespace detail {
		// Some common routines
		JKJ_FORCEINLINE char* print_number(char* buffer, std::uint32_t number, int length) noexcept {
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
		JKJ_FORCEINLINE char* print_nine_digits(char* buffer, std::uint32_t number) noexcept {
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

		JKJ_FORCEINLINE char* print_zero_or_nine(char* buffer, int length, char const d) noexcept {
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

		JKJ_FORCEINLINE char* print_zeros(char* buffer, int length) noexcept {
			return print_zero_or_nine(buffer, length, '0');
		}

		JKJ_FORCEINLINE char* print_nines(char* buffer, int length) noexcept {
			return print_zero_or_nine(buffer, length, '9');
		}
	}

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
						current_digits = rp.compute_next_segment();
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
							remainder = rp.compute_next_segment();
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

						buffer = detail::print_number(buffer, current_digits, current_digits_length);
					} // precision <= current_digits_length
					// If there are more digits to be generated
					else {
						int number_of_trailing_9;
						precision -= current_digits_length;
						auto next_digits = rp.compute_next_segment();

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
										remainder = rp.compute_next_segment();
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
									next_digits = rp.compute_next_segment();
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
										remainder = rp.compute_next_segment();
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
									next_digits = rp.compute_next_segment();
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
						next_digits = rp.compute_next_segment();
						precision -= 9;

						while (precision > 9) {
							if (next_digits == 9'9999'9999) {
								number_of_trailing_9 += 9;
							}
							else {
								buffer = detail::print_nine_digits(buffer, current_digits);
								buffer = detail::print_nines(buffer, number_of_trailing_9);
								number_of_trailing_9 = 0;
								current_digits = next_digits;
							}
							precision -= 9;
							next_digits = rp.compute_next_segment();
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
							remainder = rp.compute_next_segment();
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
					*buffer = 'e';
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
					return detail::print_zeros(buffer, precision);
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

#include "../detail/undef_macros.h"
#endif