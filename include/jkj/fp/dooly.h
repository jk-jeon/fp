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

#ifndef JKJ_HEADER_FP_DOOLY
#define JKJ_HEADER_FP_DOOLY

#include "decimal_fp.h"
#include "dragonbox.h"
#include "policy.h"
#include "detail/bits.h"
#include "detail/div.h"
#include "detail/log.h"
#include "detail/util.h"

namespace jkj::fp {
	namespace detail {
		namespace dooly {
			// Some constants
			template <ieee754_format format>
			struct impl_base : public ieee754_format_info<format>
			{
				using ieee754_format_info<format>::significand_bits;
				using ieee754_format_info<format>::min_exponent;
				using ieee754_format_info<format>::max_exponent;
				using ieee754_format_info<format>::exponent_bias;

				static constexpr int decimal_digit_limit =
					format == ieee754_format::binary32 ? 9 : 17;

				static constexpr int min_k =
					log::floor_log10_pow2(min_exponent - significand_bits)
					- decimal_digit_limit - 1;

				static constexpr int max_k =
					log::floor_log10_pow2(max_exponent + 1);
			};

			template <class Float>
			struct impl : private impl_base<ieee754_traits<Float>::format>
			{
				using carrier_uint = typename ieee754_traits<Float>::carrier_uint;
				static constexpr auto format = ieee754_traits<Float>::format;
				static constexpr auto carrier_bits = ieee754_traits<Float>::carrier_bits;

				using impl_base<format>::significand_bits;
				using impl_base<format>::exponent_bits;
				using impl_base<format>::min_exponent;
				using impl_base<format>::max_exponent;
				using impl_base<format>::exponent_bias;
				using impl_base<format>::decimal_digit_limit;
				using impl_base<format>::min_k;
				using impl_base<format>::max_k;

				static constexpr auto max_significand =
					compute_power<decimal_digit_limit>(carrier_uint(10)) - 1;

				static constexpr auto sign_bit_mask =
					carrier_uint(carrier_uint(1) << (significand_bits + exponent_bits));
				static constexpr auto infinity =
					carrier_uint(((carrier_uint(1) << exponent_bits) - 1) << significand_bits);

				static constexpr auto normal_residual_mask =
					carrier_uint((carrier_uint(1) << (carrier_bits - significand_bits - 2)) - 1);
				static constexpr auto normal_distance_to_boundary =
					carrier_uint(carrier_uint(1) << (carrier_bits - significand_bits - 3));

				static constexpr int max_power_of_factor_of_5 =
					log::floor_log5_pow2(decimal_digit_limit) + decimal_digit_limit;

				template <class IntervalTypeProvider, class SignPolicy, class CachePolicy, class InputType>
				static ieee754_bits<Float> compute(InputType decimal) noexcept
				{
					assert(decimal.significand <= max_significand);
					ieee754_bits<Float> ret_value{ carrier_uint(0) };

					// Set sign bit
					SignPolicy::decimal_to_binary(decimal, ret_value);

					// Special cases
					if (decimal.significand == 0 || decimal.exponent < min_k) {
						// Zero
						return ret_value;
					}
					else if (decimal.exponent > max_k) {
						// Infinity
						ret_value.u |= infinity;
						return ret_value;
					}

					auto tau = bits::countl_zero(decimal.significand);
					auto const& cache = CachePolicy::template get_cache<format>(decimal.exponent);
					auto gi = dragonbox::impl<Float>::compute_mul(decimal.significand << tau, cache);

					// Compute the exponent
					// e = -floor(k * log2(10)) + tau - 1
					// where f * 10^k * 2^e = g
					// Normalize g to be inside [1,2)
					int bin_exponent = carrier_bits + log::floor_log2_pow10(decimal.exponent) - tau - 1;

					// floor(log2(g)) = p-1 or p-2
					// Make it p-2
					if ((gi >> (carrier_bits - 1)) != 0) {
						gi >>= 1;
						++bin_exponent;
					}

					// Need to handle normal/subnormal dichotomy
					carrier_uint significand;
					carrier_uint residual_mask;
					carrier_uint distance_to_boundary;

					if (bin_exponent < min_exponent) {
						if constexpr (IntervalTypeProvider::tag ==
							policy::binary_rounding::tag_t::to_nearest)
						{
							// Zero
							if (bin_exponent < min_exponent - significand_bits - 1) {
								return ret_value;
							}
							// Zero or subnormal, depending on the boundary condition
							else if (bin_exponent == min_exponent - significand_bits - 1) {
								if constexpr (IntervalTypeProvider::interval_type_normal(
									ieee754_bits<Float>{ carrier_uint(0) }).include_right_endpoint())
								{
									// The middle point between 0 and the minimum nonzero number
									// should be rounded to 0
									// Hence, we need to check if there is no fractional parts
									if (gi != (sign_bit_mask >> 1)) {
										ret_value.u |= 1;
										return ret_value;
									}
									else {
										// Check if g has nonzero fractional part
										if (!is_g_integer(decimal.significand, decimal.exponent,
											carrier_bits - 2 - bin_exponent))
										{
											ret_value.u |= 1;
										}
										return ret_value;
									}
								}
								else
								{
									// The middle point should be rounded to the minimum nonzero number
									ret_value.u |= 1;
									return ret_value;
								}
							}
						}
						else if constexpr (IntervalTypeProvider::tag ==
							policy::binary_rounding::tag_t::left_closed_directed)
						{
							// Zero
							if (bin_exponent <= min_exponent - significand_bits - 1) {
								return ret_value;
							}
						}

						// Subnormal
						residual_mask = normal_residual_mask + 1;
						distance_to_boundary = normal_distance_to_boundary;

						residual_mask <<= (min_exponent - bin_exponent);
						distance_to_boundary <<= (min_exponent - bin_exponent);
						--residual_mask;

						significand = gi >>
							(int(carrier_bits - significand_bits - 2) + min_exponent - bin_exponent);

						bin_exponent = exponent_bias;
					}
					else {
						// Normal
						residual_mask = normal_residual_mask;
						distance_to_boundary = normal_distance_to_boundary;

						// Remove the implicit bit
						significand = (gi << 2) >> (carrier_bits - significand_bits);
					}

					if constexpr (IntervalTypeProvider::tag ==
						policy::binary_rounding::tag_t::to_nearest)
					{
						// Check if we need to round-up
						auto remainder = (gi & residual_mask);
						if (remainder > distance_to_boundary) {
							++significand;
						}
						else if (remainder == distance_to_boundary) {
							bool include_boundary = IntervalTypeProvider::interval_type_normal(
								ieee754_bits<Float>{ ret_value.u | significand }).include_right_endpoint();

							if (!include_boundary) {
								++significand;
							}
							else {
								// Check if g has nonzero fractional part
								if (!is_g_integer(decimal.significand, decimal.exponent,
									carrier_bits - 2 - bin_exponent))
								{
									++significand;
								}
								goto compose_bits_label;
							}
						}
						else {
							goto compose_bits_label;
						}

						// If overflow occurs
						if (significand == (carrier_uint(1) << significand_bits)) {
							// Increase the exponent
							++bin_exponent;
							significand = 0;
						}
					}
					else if constexpr (IntervalTypeProvider::tag ==
						policy::binary_rounding::tag_t::left_closed_directed)
					{
						// Always round-down (do nothing)
						goto compose_bits_label;
					}
					else
					{
						// Round-up if and only if the fractional part is nonzero
						auto remainder = (gi & residual_mask);
						if (remainder == 0) {
							if (is_g_integer(decimal.significand, decimal.exponent,
								carrier_bits - 2 - bin_exponent))
							{
								goto compose_bits_label;
							}
						}

						++significand;

						// If overflow occurs
						if (significand == (carrier_uint(1) << significand_bits)) {
							// Increase the exponent
							++bin_exponent;
							significand = 0;
						}
					}

				compose_bits_label:
					// Overflow; infinity
					if (bin_exponent > max_exponent) {
						ret_value.u |= infinity;
						return ret_value;
					}

					ret_value.u |= significand;

					// Exponent bits
					ret_value.u |= (carrier_uint(bin_exponent - exponent_bias) << significand_bits);

					return ret_value;
				}

				static bool is_g_integer(carrier_uint f, int k, int e) noexcept
				{
					if (e + k < 0) {
						return div::divisible_by_power_of_2(f, -e - k);
					}
					if (k < 0) {
						if (-k > max_power_of_factor_of_5) {
							return false;
						}
						else {
							return div::divisible_by_power_of_5<max_power_of_factor_of_5 + 1>(f, -k);
						}
					}
					else {
						return true;
					}
				}
			};
		}
	}

	template <class Float, bool is_signed>
	ieee754_bits<Float> to_binary(decimal_fp<Float, is_signed, false> decimal)
	{
		// TODO: implement policies
		return detail::dooly::impl<Float>::template compute<
			decltype(policy::binary_rounding::nearest_to_even),
			decltype(policy::sign::propagate),
			decltype(policy::cache::fast)>(decimal);
	}
}

#endif