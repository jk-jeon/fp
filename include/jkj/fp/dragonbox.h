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


#ifndef JKJ_HEADER_FP_DRAGONBOX
#define JKJ_HEADER_FP_DRAGONBOX

#include "decimal_fp.h"
#include "policy.h"
#include "detail/bits.h"
#include "detail/div.h"
#include "detail/log.h"
#include "detail/util.h"
#include "detail/wuint.h"
#include "detail/macros.h"
#include <cassert>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace jkj::fp {
	namespace detail {
		////////////////////////////////////////////////////////////////////////////////////////
		// Utilities for fast divisibility test
		////////////////////////////////////////////////////////////////////////////////////////

		namespace div {
			// Replace n by floor(n / 5^N)
			// Returns true if and only if n is divisible by 5^N
			// Precondition: n <= 2 * 5^(N+1)
			template <int N>
			struct check_divisibility_and_divide_by_pow5_info;

			template <>
			struct check_divisibility_and_divide_by_pow5_info<1> {
				static constexpr std::uint32_t magic_number = 0xcccd;
				static constexpr int bits_for_comparison = 16;
				static constexpr std::uint32_t threshold = 0x3333;
				static constexpr int shift_amount = 18;
			};

			template <>
			struct check_divisibility_and_divide_by_pow5_info<2> {
				static constexpr std::uint32_t magic_number = 0xa429;
				static constexpr int bits_for_comparison = 8;
				static constexpr std::uint32_t threshold = 0x0a;
				static constexpr int shift_amount = 20;
			};

			template <int N>
			constexpr bool check_divisibility_and_divide_by_pow5(std::uint32_t& n) noexcept
			{
				// Make sure the computation for max_n does not overflow
				static_assert(N + 1 <= log::floor_log5_pow2(31));
				assert(n <= compute_power<N + 1>(std::uint32_t(5)) * 2);

				using info = check_divisibility_and_divide_by_pow5_info<N>;
				n *= info::magic_number;
				constexpr std::uint32_t comparison_mask =
					info::bits_for_comparison >= 32 ? std::numeric_limits<std::uint32_t>::max() :
					std::uint32_t((std::uint32_t(1) << info::bits_for_comparison) - 1);

				if ((n & comparison_mask) <= info::threshold) {
					n >>= info::shift_amount;
					return true;
				}
				else {
					n >>= info::shift_amount;
					return false;
				}
			}

			// Compute floor(n / 10^N) for small n and N
			// Precondition: n <= 10^(N+1)
			template <int N>
			struct small_division_by_pow10_info;

			template <>
			struct small_division_by_pow10_info<1> {
				static constexpr std::uint32_t magic_number = 0xcccd;
				static constexpr int shift_amount = 19;
			};

			template <>
			struct small_division_by_pow10_info<2> {
				static constexpr std::uint32_t magic_number = 0xa3d8;
				static constexpr int shift_amount = 22;
			};

			template <int N>
			constexpr std::uint32_t small_division_by_pow10(std::uint32_t n) noexcept
			{
				assert(n <= compute_power<N + 1>(std::uint32_t(10)));
				return (n * small_division_by_pow10_info<N>::magic_number)
					>> small_division_by_pow10_info<N>::shift_amount;
			}

			// Compute floor(n / 10^N) for small N
			// Precondition: n <= 2^a * 5^b (a = max_pow2, b = max_pow5)
			template <int N, int max_pow2, int max_pow5, class UInt>
			constexpr UInt divide_by_pow10(UInt n) noexcept
			{
				static_assert(N >= 0);

				// Ensure no overflow
				static_assert(max_pow2 + (log::floor_log2_pow10(max_pow5) - max_pow5) < value_bits<UInt>);

				// Specialize for 64bit division by 1000
				// Ensure that the correctness condition is met
				if constexpr (std::is_same_v<UInt, std::uint64_t> && N == 3 &&
					max_pow2 + (log::floor_log2_pow10(N + max_pow5) - (N + max_pow5)) < 70)
				{
					return wuint::umul128_upper64(n, 0x8312'6e97'8d4f'df3c) >> 9;
				}
				else {
					constexpr auto divisor = compute_power<N>(UInt(10));
					return n / divisor;
				}
			}
		}
	}

	namespace detail {
		namespace dragonbox {
			////////////////////////////////////////////////////////////////////////////////////////
			// The main algorithm
			////////////////////////////////////////////////////////////////////////////////////////

			// Some constants
			template <ieee754_format format>
			struct impl_base : private ieee754_format_info<format>
			{
				using ieee754_format_info<format>::significand_bits;
				using ieee754_format_info<format>::min_exponent;
				using ieee754_format_info<format>::max_exponent;
				using ieee754_format_info<format>::exponent_bias;
				using ieee754_format_info<format>::decimal_digits;

				static constexpr int kappa = format == ieee754_format::binary32 ? 1 : 2;
				static_assert(kappa >= 1);

				static constexpr int min_k = [] {
					constexpr auto a = -log::floor_log10_pow2_minus_log10_4_over_3(
						int(max_exponent - significand_bits));
					constexpr auto b = -log::floor_log10_pow2(
						int(max_exponent - significand_bits)) + kappa;
					return a < b ? a : b;
				}();
				static_assert(min_k >= cache_holder<format>::min_k);

				static constexpr int max_k = [] {
					constexpr auto a = -log::floor_log10_pow2_minus_log10_4_over_3(
						int(min_exponent - significand_bits + 1));
					constexpr auto b = -log::floor_log10_pow2(
						int(min_exponent - significand_bits)) + kappa;
					return a > b ? a : b;
				}();
				static_assert(max_k <= cache_holder<format>::max_k);
			};

			// Get sign/decimal significand/decimal exponent from
			// the bit representation of a floating-point number
			template <class Float>
			struct impl : private ieee754_traits<Float>,
				private impl_base<ieee754_traits<Float>::format>
			{
				using carrier_uint = typename ieee754_traits<Float>::carrier_uint;
				using ieee754_traits<Float>::format;
				using ieee754_traits<Float>::carrier_bits;

				using impl_base<format>::significand_bits;
				using impl_base<format>::min_exponent;
				using impl_base<format>::max_exponent;
				using impl_base<format>::exponent_bias;
				using impl_base<format>::decimal_digits;
				using impl_base<format>::kappa;
				using impl_base<format>::min_k;
				using impl_base<format>::max_k;
				
				static_assert(carrier_bits >= significand_bits + 2 + log::floor_log2_pow10(kappa + 1));

				using cache_entry_type = typename cache_holder<format>::cache_entry_type;
				static constexpr auto cache_bits = cache_holder<format>::cache_bits;

				static constexpr int max_power_of_factor_of_5 = log::floor_log5_pow2(int(significand_bits + 2));
				static constexpr int divisibility_check_by_5_threshold =
					log::floor_log2_pow10(max_power_of_factor_of_5 + kappa + 1);

				static constexpr int case_fc_pm_half_lower_threshold = -kappa - log::floor_log5_pow2(kappa);
				static constexpr int case_fc_pm_half_upper_threshold = log::floor_log2_pow10(kappa + 1);

				static constexpr int case_fc_lower_threshold = -kappa - 1 - log::floor_log5_pow2(kappa + 1);
				static constexpr int case_fc_upper_threshold = log::floor_log2_pow10(kappa + 1);

				static constexpr int case_shorter_interval_left_endpoint_lower_threshold = 2;
				static constexpr int case_shorter_interval_left_endpoint_upper_threshold = 2 +
					log::floor_log2(compute_power<
						count_factors<5>((carrier_uint(1) << (significand_bits + 2)) - 1) + 1
					>(10) / 3);

				static constexpr int case_shorter_interval_right_endpoint_lower_threshold = 0;
				static constexpr int case_shorter_interval_right_endpoint_upper_threshold = 2 +
					log::floor_log2(compute_power<
						count_factors<5>((carrier_uint(1) << (significand_bits + 1)) + 1) + 1
					>(10) / 3);

				static constexpr int shorter_interval_tie_lower_threshold =
					-log::floor_log5_pow2_minus_log5_3(significand_bits + 4) - 2 - significand_bits;
				static constexpr int shorter_interval_tie_upper_threshold =
					-log::floor_log5_pow2(significand_bits + 2) - 2 - significand_bits;

				//// The main algorithm assumes the input is a normal/subnormal finite number

				template <class ReturnType, class IntervalTypeProvider, class SignPolicy,
					class TrailingZeroPolicy, class DecimalRoundingPolicy, class CachePolicy>
				JKJ_SAFEBUFFERS static ReturnType compute_nearest(ieee754_bits<Float> const br) noexcept
				{
					//////////////////////////////////////////////////////////////////////
					// Step 1: integer promotion & Schubfach multiplier calculation
					//////////////////////////////////////////////////////////////////////

					ReturnType ret_value;

					SignPolicy::binary_to_decimal(br, ret_value);

					auto significand = br.extract_significand_bits();
					auto exponent = int(br.extract_exponent_bits());

					// Deal with normal/subnormal dichotomy
					if (exponent != 0) {
						exponent += exponent_bias - significand_bits;

						// Shorter interval case; proceed like Schubfach
						if (significand == 0) {
							shorter_interval_case<TrailingZeroPolicy, DecimalRoundingPolicy, CachePolicy>(
								ret_value, exponent,
								IntervalTypeProvider::interval_type_shorter(br));
							return ret_value;
						}

						significand |= (carrier_uint(1) << significand_bits);
					}
					// Subnormal case; interval is always regular
					else {
						exponent = min_exponent - significand_bits;
					}

					auto const interval_type = IntervalTypeProvider::interval_type_normal(br);

					// Compute k and beta
					int const minus_k = log::floor_log10_pow2(exponent) - kappa;
					auto const cache = CachePolicy::template get_cache<format>(-minus_k);
					int const beta_minus_1 = exponent + log::floor_log2_pow10(-minus_k);

					// Compute zi and deltai
					// 10^kappa <= deltai < 10^(kappa + 1)
					auto const deltai = compute_delta(cache, beta_minus_1);
					carrier_uint const two_fc = significand << 1;
					carrier_uint const two_fr = two_fc | 1;
					carrier_uint const zi = compute_mul(two_fr << beta_minus_1, cache);


					//////////////////////////////////////////////////////////////////////
					// Step 2: Try larger divisor; remove trailing zeros if necessary
					//////////////////////////////////////////////////////////////////////

					constexpr auto big_divisor = compute_power<kappa + 1>(std::uint32_t(10));
					constexpr auto small_divisor = compute_power<kappa>(std::uint32_t(10));

					// Using an upper bound on zi, we might be able to optimize the division
					// better than the compiler; we are computing zi / big_divisor here
					ret_value.significand = div::divide_by_pow10<kappa + 1,
						significand_bits + kappa + 2, kappa + 1>(zi);
					auto r = std::uint32_t(zi - big_divisor * ret_value.significand);

					if (r > deltai) {
						goto small_divisor_case_label;
					}
					else if (r < deltai) {
						// Exclude the right endpoint if necessary
						if (r == 0 && !interval_type.include_right_endpoint() &&
							is_product_integer<integer_check_case_id::fc_pm_half>(two_fr, exponent, minus_k))
						{
							if constexpr (DecimalRoundingPolicy::tag ==
								policy::decimal_rounding::tag_t::do_not_care)
							{
								ret_value.significand *= 10;
								ret_value.exponent = minus_k + kappa;
								--ret_value.significand;
								return ret_value;
							}
							else {
								--ret_value.significand;
								r = big_divisor;
								goto small_divisor_case_label;
							}
						}
					}
					else {
						// r == deltai; compare fractional parts
						// Check conditions in the order different from the paper
						// to take advantage of short-circuiting
						auto const two_fl = two_fc - 1;
						if ((!interval_type.include_left_endpoint() ||
							!is_product_integer<integer_check_case_id::fc_pm_half>(
								two_fl, exponent, minus_k)) &&
							!compute_mul_parity(two_fl, cache, beta_minus_1))
						{
							goto small_divisor_case_label;
						}
					}
					ret_value.exponent = minus_k + kappa + 1;

					// We may need to remove trailing zeros
					TrailingZeroPolicy::on_trailing_zeros(ret_value);
					return ret_value;


					//////////////////////////////////////////////////////////////////////
					// Step 3: Find the significand with the smaller divisor
					//////////////////////////////////////////////////////////////////////

				small_divisor_case_label:
					TrailingZeroPolicy::no_trailing_zeros(ret_value);
					ret_value.significand *= 10;
					ret_value.exponent = minus_k + kappa;

					constexpr auto mask = (std::uint32_t(1) << kappa) - 1;

					if constexpr (DecimalRoundingPolicy::tag ==
						policy::decimal_rounding::tag_t::do_not_care)
					{
						// Normally, we want to compute
						// ret_value.significand += r / small_divisor
						// and return, but we need to take care of the case that the resulting
						// value is exactly the right endpoint, while that is not included in the interval
						if (!interval_type.include_right_endpoint()) {
							// Is r divisible by 2^kappa?
							if ((r & mask) == 0) {
								r >>= kappa;

								// Is r divisible by 5^kappa?
								if (div::check_divisibility_and_divide_by_pow5<kappa>(r) &&
									is_product_integer<integer_check_case_id::fc_pm_half>(two_fr, exponent, minus_k))
								{
									// This should be in the interval
									ret_value.significand += r - 1;
								}
								else {
									ret_value.significand += r;
								}
							}
							else {
								ret_value.significand += div::small_division_by_pow10<kappa>(r);
							}
						}
						else {
							ret_value.significand += div::small_division_by_pow10<kappa>(r);
						}
					}
					else
					{
						auto dist = r - (deltai / 2) + (small_divisor / 2);

						// Is dist divisible by 2^kappa?
						if ((dist & mask) == 0) {
							bool const approx_y_parity = ((dist ^ (small_divisor / 2)) & 1) != 0;
							dist >>= kappa;

							// Is dist divisible by 5^kappa?
							if (div::check_divisibility_and_divide_by_pow5<kappa>(dist)) {
								ret_value.significand += dist;

								// Check z^(f) >= epsilon^(f)
								// We have either yi == zi - epsiloni or yi == (zi - epsiloni) - 1,
								// where yi == zi - epsiloni if and only if z^(f) >= epsilon^(f)
								// Since there are only 2 possibilities, we only need to care about the parity
								// Also, zi and r should have the same parity since the divisor
								// is an even number
								if (compute_mul_parity(two_fc, cache, beta_minus_1) != approx_y_parity) {
									--ret_value.significand;
								}
								else {
									// If z^(f) >= epsilon^(f), we might have a tie
									// when z^(f) == epsilon^(f), or equivalently, when y is an integer
									// For tie-to-up case, we can just choose the upper one
									if constexpr (DecimalRoundingPolicy::tag !=
										policy::decimal_rounding::tag_t::away_from_zero)
									{
										if (is_product_integer<integer_check_case_id::fc>(
											two_fc, exponent, minus_k))
										{
											DecimalRoundingPolicy::break_rounding_tie(ret_value);
										}
									}
								}
							}
							// Is dist not divisible by 5^kappa?
							else {
								ret_value.significand += dist;
							}
						}
						// Is dist not divisible by 2^kappa?
						else {
							// Since we know dist is small, we might be able to optimize the division
							// better than the compiler; we are computing dist / small_divisor here
							ret_value.significand += div::small_division_by_pow10<kappa>(dist);
						}
					}
					return ret_value;
				}

				template <class TrailingZeroPolicy, class DecimalRoundingPolicy,
					class CachePolicy, class ReturnType, class IntervalType>
					JKJ_FORCEINLINE JKJ_SAFEBUFFERS static void shorter_interval_case(
						ReturnType& ret_value, int const exponent, IntervalType const interval_type) noexcept
				{
					// Compute k and beta
					int const minus_k = log::floor_log10_pow2_minus_log10_4_over_3(exponent);
					int const beta_minus_1 = exponent + log::floor_log2_pow10(-minus_k);

					// Compute xi and zi
					auto const cache = CachePolicy::template get_cache<format>(-minus_k);

					auto xi = compute_left_endpoint_for_shorter_interval_case(cache, beta_minus_1);
					auto zi = compute_right_endpoint_for_shorter_interval_case(cache, beta_minus_1);

					// If we don't accept the right endpoint and
					// if the right endpoint is an integer, decrease it
					if (!interval_type.include_right_endpoint() &&
						is_right_endpoint_integer_shorter_interval(exponent))
					{
						--zi;
					}
					// If we don't accept the left endpoint or
					// if the left endpoint is not an integer, increase it
					if (!interval_type.include_left_endpoint() ||
						!is_left_endpoint_integer_shorter_interval(exponent))
					{
						++xi;
					}

					// Try bigger divisor
					ret_value.significand = zi / 10;

					// If succeed, remove trailing zeros if necessary and return
					if (ret_value.significand * 10 >= xi) {
						ret_value.exponent = minus_k + 1;
						TrailingZeroPolicy::on_trailing_zeros(ret_value);
						return;
					}

					// Otherwise, compute the round-up of y
					TrailingZeroPolicy::no_trailing_zeros(ret_value);
					ret_value.significand = compute_round_up_for_shorter_interval_case(cache, beta_minus_1);
					ret_value.exponent = minus_k;

					// When tie occurs, choose one of them according to the rule
					if constexpr (DecimalRoundingPolicy::tag !=
						policy::decimal_rounding::tag_t::do_not_care &&
						DecimalRoundingPolicy::tag !=
						policy::decimal_rounding::tag_t::away_from_zero)
					{
						if (exponent >= shorter_interval_tie_lower_threshold &&
							exponent <= shorter_interval_tie_upper_threshold)
						{
							DecimalRoundingPolicy::break_rounding_tie(ret_value);
						}
						else if (ret_value.significand < xi) {
							++ret_value.significand;
						}
					}
					else
					{
						if (ret_value.significand < xi) {
							++ret_value.significand;
						}
					}
				}

				template <class ReturnType, class SignPolicy, class TrailingZeroPolicy, class CachePolicy>
				JKJ_SAFEBUFFERS static ReturnType
					compute_left_closed_directed(ieee754_bits<Float> const br) noexcept
				{
					//////////////////////////////////////////////////////////////////////
					// Step 1: integer promotion & Schubfach multiplier calculation
					//////////////////////////////////////////////////////////////////////

					ReturnType ret_value;

					SignPolicy::handle_sign(br, ret_value);

					auto significand = br.extract_significand_bits();
					auto exponent = int(br.extract_exponent_bits());

					// Deal with normal/subnormal dichotomy
					if (exponent != 0) {
						exponent += exponent_bias - significand_bits;
						significand |= (carrier_uint(1) << significand_bits);
					}
					// Subnormal case; interval is always regular
					else {
						exponent = min_exponent - significand_bits;
					}

					// Compute k and beta
					int const minus_k = log::floor_log10_pow2(exponent) - kappa;
					auto const cache = CachePolicy::template get_cache<format>(-minus_k);
					int const beta = exponent + log::floor_log2_pow10(-minus_k) + 1;

					// Compute xi and deltai
					// 10^kappa <= deltai < 10^(kappa + 1)
					auto const deltai = compute_delta(cache, beta - 1);
					carrier_uint xi = compute_mul(significand << beta, cache);

					if (!is_product_integer<integer_check_case_id::fc>(significand, exponent + 1, minus_k)) {
						++xi;
					}

					//////////////////////////////////////////////////////////////////////
					// Step 2: Try larger divisor; remove trailing zeros if necessary
					//////////////////////////////////////////////////////////////////////

					constexpr auto big_divisor = compute_power<kappa + 1>(std::uint32_t(10));
					constexpr auto small_divisor = compute_power<kappa>(std::uint32_t(10));

					// Using an upper bound on xi, we might be able to optimize the division
					// better than the compiler; we are computing xi / big_divisor here
					ret_value.significand = div::divide_by_pow10<kappa + 1,
						significand_bits + kappa + 2, kappa + 1>(xi);
					auto r = std::uint32_t(xi - big_divisor * ret_value.significand);

					if (r != 0) {
						++ret_value.significand;
						r = big_divisor - r;
					}

					if (r > deltai) {
						goto small_divisor_case_label;
					}
					else if (r == deltai) {
						// Compare the fractional parts
						if (compute_mul_parity(significand + 1, cache, beta) ||
							is_product_integer<integer_check_case_id::fc>(significand + 1, exponent + 1, minus_k))
						{
							goto small_divisor_case_label;
						}

					}

					// The ceiling is inside, so we are done
					ret_value.exponent = minus_k + kappa + 1;
					TrailingZeroPolicy::on_trailing_zeros(ret_value);
					return ret_value;


					//////////////////////////////////////////////////////////////////////
					// Step 3: Find the significand with the smaller divisor
					//////////////////////////////////////////////////////////////////////

				small_divisor_case_label:
					ret_value.significand *= 10;
					ret_value.significand -= div::small_division_by_pow10<kappa>(r);
					ret_value.exponent = minus_k + kappa;
					TrailingZeroPolicy::no_trailing_zeros(ret_value);
					return ret_value;
				}

				template <class ReturnType, class SignPolicy, class TrailingZeroPolicy, class CachePolicy>
				JKJ_SAFEBUFFERS static ReturnType
					compute_right_closed_directed(ieee754_bits<Float> const br) noexcept
				{
					//////////////////////////////////////////////////////////////////////
					// Step 1: integer promotion & Schubfach multiplier calculation
					//////////////////////////////////////////////////////////////////////

					ReturnType ret_value;

					SignPolicy::handle_sign(br, ret_value);

					auto significand = br.extract_significand_bits();
					auto exponent = int(br.extract_exponent_bits());

					// Deal with normal/subnormal dichotomy
					bool closer_boundary = false;
					if (exponent != 0) {
						exponent += exponent_bias - significand_bits;
						if (significand == 0) {
							closer_boundary = true;
						}
						significand |= (carrier_uint(1) << significand_bits);
					}
					// Subnormal case; interval is always regular
					else {
						exponent = min_exponent - significand_bits;
					}

					// Compute k and beta
					int const minus_k = log::floor_log10_pow2(exponent - (closer_boundary ? 1 : 0)) - kappa;
					auto const cache = CachePolicy::template get_cache<format>(-minus_k);
					int const beta = exponent + log::floor_log2_pow10(-minus_k) + 1;

					// Compute zi and deltai
					// 10^kappa <= deltai < 10^(kappa + 1)
					auto const deltai = closer_boundary ?
						compute_delta(cache, beta - 2) :
						compute_delta(cache, beta - 1);
					carrier_uint const zi = compute_mul(significand << beta, cache);


					//////////////////////////////////////////////////////////////////////
					// Step 2: Try larger divisor; remove trailing zeros if necessary
					//////////////////////////////////////////////////////////////////////

					constexpr auto big_divisor = compute_power<kappa + 1>(std::uint32_t(10));
					constexpr auto small_divisor = compute_power<kappa>(std::uint32_t(10));

					// Using an upper bound on zi, we might be able to optimize the division
					// better than the compiler; we are computing zi / big_divisor here
					ret_value.significand = div::divide_by_pow10<kappa + 1,
						significand_bits + kappa + 2, kappa + 1>(zi);
					auto const r = std::uint32_t(zi - big_divisor * ret_value.significand);

					if (r > deltai) {
						goto small_divisor_case_label;
					}
					else if (r == deltai) {
						// Compare the fractional parts
						if (closer_boundary) {
							if (!compute_mul_parity((significand * 2) - 1, cache, beta - 1))
							{
								goto small_divisor_case_label;
							}
						}
						else {
							if (!compute_mul_parity(significand - 1, cache, beta))
							{
								goto small_divisor_case_label;
							}
						}
					}

					// The floor is inside, so we are done
					ret_value.exponent = minus_k + kappa + 1;
					TrailingZeroPolicy::on_trailing_zeros(ret_value);
					return ret_value;


					//////////////////////////////////////////////////////////////////////
					// Step 3: Find the significand with the small divisor
					//////////////////////////////////////////////////////////////////////

				small_divisor_case_label:
					ret_value.significand *= 10;
					ret_value.significand += div::small_division_by_pow10<kappa>(r);
					ret_value.exponent = minus_k + kappa;
					TrailingZeroPolicy::no_trailing_zeros(ret_value);
					return ret_value;
				}

				// Remove trailing zeros from n and return the number of zeros removed
				JKJ_FORCEINLINE static int remove_trailing_zeros(carrier_uint& n) noexcept {
					constexpr auto max_power = [] {
						auto max_possible_significand =
							std::numeric_limits<carrier_uint>::max() /
							compute_power<kappa + 1>(std::uint32_t(10));

						int k = 0;
						carrier_uint p = 1;
						while (p < max_possible_significand / 10) {
							p *= 10;
							++k;
						}
						return k;
					}();

					auto t = bits::countr_zero(n);
					if (t > max_power) {
						t = max_power;
					}

					if constexpr (format == ieee754_format::binary32) {
						constexpr auto const& divtable =
							div::table_holder<carrier_uint, 5, decimal_digits>::table;

						int s = 0;
						for (; s < t - 1; s += 2) {
							if (n * divtable[2].mod_inv > divtable[2].max_quotient) {
								break;
							}
							n *= divtable[2].mod_inv;
						}
						if (s < t && n * divtable[1].mod_inv <= divtable[1].max_quotient)
						{
							n *= divtable[1].mod_inv;
							++s;
						}
						n >>= s;
						return s;
					}
					else {
						static_assert(format == ieee754_format::binary64);
						static_assert(kappa >= 2);

						// Divide by 10^8 and reduce to 32-bits
						// Since ret_value.significand <= (2^64 - 1) / 1000 < 10^17,
						// both of the quotient and the r should fit in 32-bits

						constexpr auto const& divtable =
							div::table_holder<carrier_uint, 5, decimal_digits>::table;

						// If the number is divisible by 1'0000'0000, work with the quotient
						if (t >= 8) {
							auto quotient_candidate = n * divtable[8].mod_inv;

							if (quotient_candidate <= divtable[8].max_quotient) {
								auto quotient = std::uint32_t(quotient_candidate >> 8);

								constexpr auto mod_inverse = std::uint32_t(divtable[1].mod_inv);
								constexpr auto max_quotient =
									std::numeric_limits<std::uint32_t>::max() / 5;

								int s = 8;
								for (; s < t; ++s) {
									if (quotient * mod_inverse > max_quotient) {
										break;
									}
									quotient *= mod_inverse;
								}
								quotient >>= (s - 8);
								n = quotient;
								return s;
							}
						}

						// Otherwise, work with the remainder
						auto quotient = std::uint32_t(div::divide_by_pow10<8, 54, 0>(n));
						auto remainder = std::uint32_t(n - 1'0000'0000 * quotient);

						constexpr auto mod_inverse = std::uint32_t(divtable[1].mod_inv);
						constexpr auto max_quotient =
							std::numeric_limits<std::uint32_t>::max() / 5;

						if (t == 0 || remainder * mod_inverse > max_quotient) {
							return 0;
						}
						remainder *= mod_inverse;

						if (t == 1 || remainder * mod_inverse > max_quotient) {
							n = (remainder >> 1)
								+ quotient * carrier_uint(1000'0000);
							return 1;
						}
						remainder *= mod_inverse;

						if (t == 2 || remainder * mod_inverse > max_quotient) {
							n = (remainder >> 2)
								+ quotient * carrier_uint(100'0000);
							return 2;
						}
						remainder *= mod_inverse;

						if (t == 3 || remainder * mod_inverse > max_quotient) {
							n = (remainder >> 3)
								+ quotient * carrier_uint(10'0000);
							return 3;
						}
						remainder *= mod_inverse;

						if (t == 4 || remainder * mod_inverse > max_quotient) {
							n = (remainder >> 4)
								+ quotient * carrier_uint(1'0000);
							return 4;
						}
						remainder *= mod_inverse;

						if (t == 5 || remainder * mod_inverse > max_quotient) {
							n = (remainder >> 5)
								+ quotient * carrier_uint(1000);
							return 5;
						}
						remainder *= mod_inverse;

						if (t == 6 || remainder * mod_inverse > max_quotient) {
							n = (remainder >> 6)
								+ quotient * carrier_uint(100);
							return 6;
						}
						remainder *= mod_inverse;

						n = (remainder >> 7)
							+ quotient * carrier_uint(10);
						return 7;
					}
				}

				static carrier_uint compute_mul(carrier_uint u, cache_entry_type const& cache) noexcept
				{
					if constexpr (format == ieee754_format::binary32) {
						return wuint::umul96_upper32(u, cache);
					}
					else {
						return wuint::umul192_upper64(u, cache);
					}
				}

				static std::uint32_t compute_delta(cache_entry_type const& cache, int beta_minus_1) noexcept
				{
					if constexpr (format == ieee754_format::binary32) {
						return std::uint32_t(cache >> (cache_bits - 1 - beta_minus_1));
					}
					else {
						return std::uint32_t(cache.high() >> (carrier_bits - 1 - beta_minus_1));
					}
				}

				static bool compute_mul_parity(carrier_uint two_f, cache_entry_type const& cache, int beta_minus_1) noexcept
				{
					assert(beta_minus_1 >= 1);
					assert(beta_minus_1 < 64);

					if constexpr (format == ieee754_format::binary32) {
						return ((wuint::umul96_lower64(two_f, cache) >>
							(64 - beta_minus_1)) & 1) != 0;
					}
					else {
						return ((wuint::umul192_middle64(two_f, cache) >>
							(64 - beta_minus_1)) & 1) != 0;
					}
				}

				static carrier_uint compute_left_endpoint_for_shorter_interval_case(
					cache_entry_type const& cache, int beta_minus_1) noexcept
				{
					if constexpr (format == ieee754_format::binary32) {
						return carrier_uint(
							(cache - (cache >> (significand_bits + 2))) >>
							(cache_bits - significand_bits - 1 - beta_minus_1));
					}
					else {
						return (cache.high() - (cache.high() >> (significand_bits + 2))) >>
							(carrier_bits - significand_bits - 1 - beta_minus_1);
					}
				}

				static carrier_uint compute_right_endpoint_for_shorter_interval_case(
					cache_entry_type const& cache, int beta_minus_1) noexcept
				{
					if constexpr (format == ieee754_format::binary32) {
						return carrier_uint(
							(cache + (cache >> (significand_bits + 1))) >>
							(cache_bits - significand_bits - 1 - beta_minus_1));
					}
					else {
						return (cache.high() + (cache.high() >> (significand_bits + 1))) >>
							(carrier_bits - significand_bits - 1 - beta_minus_1);
					}
				}

				static carrier_uint compute_round_up_for_shorter_interval_case(
					cache_entry_type const& cache, int beta_minus_1) noexcept
				{
					if constexpr (format == ieee754_format::binary32) {
						return (carrier_uint(cache >> (cache_bits - significand_bits - 2 - beta_minus_1)) + 1) / 2;
					}
					else {
						return ((cache.high() >> (carrier_bits - significand_bits - 2 - beta_minus_1)) + 1) / 2;
					}
				}

				static bool is_right_endpoint_integer_shorter_interval(int exponent) noexcept {
					return exponent >= case_shorter_interval_right_endpoint_lower_threshold &&
						exponent <= case_shorter_interval_right_endpoint_upper_threshold;
				}

				static bool is_left_endpoint_integer_shorter_interval(int exponent) noexcept {
					return exponent >= case_shorter_interval_left_endpoint_lower_threshold &&
						exponent <= case_shorter_interval_left_endpoint_upper_threshold;
				}

				enum class integer_check_case_id {
					fc_pm_half,
					fc
				};
				template <integer_check_case_id case_id>
				static bool is_product_integer(carrier_uint two_f, int exponent, int minus_k) noexcept
				{
					// Case I: f = fc +- 1/2
					if constexpr (case_id == integer_check_case_id::fc_pm_half)
					{
						if (exponent < case_fc_pm_half_lower_threshold) {
							return false;
						}
						// For k >= 0
						else if (exponent <= case_fc_pm_half_upper_threshold) {
							return true;
						}
						// For k < 0
						else if (exponent > divisibility_check_by_5_threshold) {
							return false;
						}
						else {
							return div::divisible_by_power_of_5<max_power_of_factor_of_5 + 1>(two_f, minus_k);
						}
					}
					// Case II: f = fc + 1
					// Case III: f = fc
					else
					{
						// Exponent for 5 is negative
						if (exponent > divisibility_check_by_5_threshold) {
							return false;
						}
						else if (exponent > case_fc_upper_threshold) {
							return div::divisible_by_power_of_5<max_power_of_factor_of_5 + 1>(two_f, minus_k);
						}
						// Both exponents are nonnegative
						else if (exponent >= case_fc_lower_threshold) {
							return true;
						}
						// Exponent for 2 is negative
						else {
							return div::divisible_by_power_of_2(two_f, minus_k - exponent + 1);
						}
					}
				}
			};
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////
	// The interface function
	////////////////////////////////////////////////////////////////////////////////////////

	template <class Float, class... Policies>
	JKJ_SAFEBUFFERS JKJ_FORCEINLINE auto to_shortest_decimal(Float x, Policies&&... policies)
	{
		// Build policy holder type
		using namespace policy;
		using detail::policy::make_policy_holder;
		using detail::policy::make_default_list;
		using detail::policy::make_default;
		auto policy_holder = make_policy_holder(
			make_default_list(
				make_default<policy_kind::sign>(sign::propagate),
				make_default<policy_kind::trailing_zero>(trailing_zero::remove),
				make_default<policy_kind::binary_rounding>(binary_rounding::nearest_to_even),
				make_default<policy_kind::decimal_rounding>(decimal_rounding::to_even),
				make_default<policy_kind::cache>(cache::fast),
				make_default<policy_kind::input_validation>(input_validation::assert_finite)),
			std::forward<Policies>(policies)...);

		using policy_holder_t = decltype(policy_holder);

		using return_type = decimal_fp<Float,
			decltype(policy_holder)::return_has_sign,
			decltype(policy_holder)::report_trailing_zeros>;

		auto br = ieee754_bits(x);
		policy_holder.validate_input(br);

		return policy_holder.delegate(br,
			[br](auto interval_type_provider) {
				using detail::policy::binary_rounding::tag_t;
				constexpr tag_t tag = decltype(interval_type_provider)::tag;

				if constexpr (tag == tag_t::to_nearest) {
					return detail::dragonbox::impl<Float>::template
						compute_nearest<return_type, decltype(interval_type_provider),
							typename policy_holder_t::sign_policy,
							typename policy_holder_t::trailing_zero_policy,
							typename policy_holder_t::decimal_rounding_policy,
							typename policy_holder_t::cache_policy
						>(br);
				}
				else if constexpr (tag == tag_t::left_closed_directed) {
					return detail::dragonbox::impl<Float>::template
						compute_left_closed_directed<return_type,
							typename policy_holder_t::sign_policy,
							typename policy_holder_t::trailing_zero_policy,
							typename policy_holder_t::cache_policy
						>(br);
				}
				else {
					return detail::dragonbox::impl<Float>::template
						compute_right_closed_directed<return_type,
							typename policy_holder_t::sign_policy,
							typename policy_holder_t::trailing_zero_policy,
							typename policy_holder_t::cache_policy
						>(br);
				}
			});
	}
}

#include "detail/undef_macros.h"
#endif
