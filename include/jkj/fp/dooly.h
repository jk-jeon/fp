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
#include "ryu_printf.h"
#include "policy.h"
#include "detail/bits.h"
#include "detail/div.h"
#include "detail/log.h"
#include "detail/util.h"
#include "detail/macros.h"
#include <cassert>
#include <cstdint>

namespace jkj::fp {
	template <ieee754_format format>
	static constexpr int to_binary_limited_precision_digit_limit =
		format == ieee754_format::binary32 ? 9 : 17;

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
					to_binary_limited_precision_digit_limit<format>;

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
	ieee754_bits<Float> to_binary_limited_precision(decimal_fp<Float, is_signed, false> decimal)
	{
		// TODO: implement policies
		return detail::dooly::impl<Float>::template compute<
			decltype(policy::binary_rounding::nearest_to_even),
			decltype(policy::sign::propagate),
			decltype(policy::cache::fast)>(decimal);
	}

	// Does the same thing as ryu_printf, but specialized for the usage in Dooly.
	// Suppose the input decimal number is of the form
	// (g + alpha) * 10^k, where g is an integer and alpha is in [0,1).
	// Write k = q * eta + r for integers q, r with 0 <= r < eta, where eta is the segment length.
	// What this class does is to generate digits of 2^e for given e, from the
	// segment index given as a construction parameter, which should be -q.
	// This class will tell if the given initial segment index is the first nonzero segment or not.
	// If there are preceding nonzero segments, then that means 2^e is strictly greater than
	// alpha * 10^k. If not, then compare floor(alpha * 10^r) with the initial segment
	// generated from this class. If they are equal, then compare
	// floor(alpha * 10^(r + eta)) with the next segment generated, and if they are still equal,
	// then compare floor(alpha * 10^(r + 2 * eta)) with the next segment generated,
	// and so on. In this way, the user can compare alpha * 10^k and 2^e.
	template <class Float>
	class dooly_generator : private detail::ryu_printf::impl_base<ieee754_traits<Float>::format>
	{
	public:
		static constexpr auto format = ieee754_traits<Float>::format;

	private:
		using impl_base = detail::ryu_printf::impl_base<format>;
		using impl_base::significand_bits;
		using impl_base::min_exponent;
		using impl_base::exponent_bias;
		using impl_base::segment_bit_size;
		using impl_base::compression_factor;
		using fast_cache_holder = detail::ryu_printf::fast_cache_holder<format>;
		using cache_entry_type = typename fast_cache_holder::cache_entry_type;

		using carrier_uint = typename ieee754_traits<Float>::carrier_uint;
		static constexpr auto carrier_bits = ieee754_traits<Float>::carrier_bits;

		int exponent_;
		std::uint32_t segment_;
		int segment_index_;		// n
		int exponent_index_;	// k
		int remainder_;			// r
		int min_segment_index_;
		int max_segment_index_;

	public:
		using impl_base::segment_size;
		using impl_base::segment_divisor;

		static constexpr auto max_nonzero_decimal_digits =
			detail::log::floor_log10_pow5(significand_bits - min_exponent) +
			detail::log::floor_log10_pow2(significand_bits) + 2;

		// Computes the first segmnet on construction.
		JKJ_FORCEINLINE dooly_generator(int exponent, int initial_segment_index) noexcept
			: exponent_{ exponent }, segment_index_{ initial_segment_index }
		{
			// 2^e * 10^(n * eta) >= 1
			// <=>
			// n >= ceil(-e * log10(2) / eta) = -floor(e * log10(2) / eta)
			if (exponent_ >= 0) {
				// Avoid signed division.
				min_segment_index_ =
					-int(unsigned(detail::log::floor_log10_pow2(exponent_)) / unsigned(segment_size));
				max_segment_index_ = 0;
			}
			else {
				// Avoid signed division.
				min_segment_index_ =
					int(unsigned(detail::log::floor_log10_pow2(-exponent_) / unsigned(segment_size))) + 1;
				max_segment_index_ = int(unsigned(-exponent_ + segment_size - 1) / unsigned(segment_size));
			}

			// We will compute the first segment.

			// Avoid signed division.
			int pow2_exponent = exponent_ + segment_index_ * segment_size;
			if (pow2_exponent >= 0) {
				exponent_index_ = int(unsigned(pow2_exponent) / unsigned(compression_factor));
				remainder_ = int(unsigned(pow2_exponent) % unsigned(compression_factor));
			}
			else {
				exponent_index_ = -int(unsigned(-pow2_exponent) / unsigned(compression_factor));
				remainder_ = int(unsigned(-pow2_exponent) % unsigned(compression_factor));

				if (remainder_ != 0) {
					--exponent_index_;
					remainder_ = compression_factor - remainder_;
				}
			}

			// Get the first nonzero segment.
			if (segment_index_ >= min_segment_index_ && segment_index_ <= max_segment_index_) {
				segment_ = compute_segment();
			}
			else {
				segment_ = 0;
			}
		}

		std::uint32_t current_segment() const noexcept {
			return segment_;
		}

		int current_segment_index() const noexcept {
			return segment_index_;
		}

		bool has_further_nonzero_segments() const noexcept {
			if (segment_index_ >= max_segment_index_) {
				return false;
			}
			else {
				// Check if there are reamining nonzero digits,
				// which is equivalent to that f * 2^e * 10^(n * eta) is not an integer.

				auto minus_pow5_exponent = -segment_index_ * segment_size;
				auto minus_pow2_exponent = -exponent_ + minus_pow5_exponent;

				return minus_pow2_exponent > 0 || minus_pow5_exponent > 0;
			}
		}

		// Returns true if there might be nonzero segments remaining,
		// and returns false if all following segments are zero.
		JKJ_FORCEINLINE bool compute_next_segment() noexcept {
			++segment_index_;
			if (segment_index_ <= max_segment_index_) {
				on_increase_segment_index();
				return true;
			}
			else {
				segment_ = 0;
				return false;
			}
		}

	private:
		JKJ_FORCEINLINE std::uint32_t compute_segment() const noexcept {
			auto const& cache = fast_cache_holder::cache[exponent_index_ +
				fast_cache_holder::get_starting_index_minus_min_k(segment_index_)];
			return multiply_shift_mod(cache, segment_bit_size + remainder_);
		}

		JKJ_FORCEINLINE void on_increase_segment_index() noexcept {
			assert(segment_index_ <= max_segment_index_);
			remainder_ += segment_size;
			static_assert(segment_size < compression_factor);
			if (remainder_ >= compression_factor) {
				++exponent_index_;
				remainder_ -= compression_factor;
			}
			if (segment_index_ >= min_segment_index_) {
				segment_ = compute_segment();
			}
		}

		JKJ_SAFEBUFFERS JKJ_FORCEINLINE static std::uint32_t multiply_shift_mod(
			cache_entry_type const& y, int shift_amount) noexcept
		{
			using namespace detail;

			if constexpr (format == ieee754_format::binary32) {
				static_assert(value_bits<carrier_uint> <= 32);
				assert(shift_amount > 0 && shift_amount <= 64);
				auto shift_result = ((std::uint64_t(y[0]) << 32) | y[1]) >> (64 - shift_amount);

				return std::uint32_t(shift_result % segment_divisor);
			}
			else {
				static_assert(format == ieee754_format::binary64);
				static_assert(value_bits<carrier_uint> <= 64);
				assert(shift_amount > 0 && shift_amount <= 128);
				auto shift_result = shift_amount <= 64 ?
					wuint::uint128{ 0, y[0] >> (64 - shift_amount) } :
					wuint::uint128{ y[0], y[1] } >> (128 - shift_amount);

				// Granlund-Montgomery style division by 10^9
				static_assert(compression_factor + segment_size - 1 <= 75);
				// Note that shift_result is of shift_amount bits.
				// For 75-bit numbers, it is sufficient to use 76-bits, but to
				// make computation simpler, we will use 99-bits.
				// The result of multiplication fits inside 192-bit,
				// and the corresponding shift amount is 128 bits so we just need to
				// take the upper 64-bits.
				auto const c = wuint::uint128{ 0x4'4b82'fa09, 0xb5a5'2cb9'8bc9'c4a7 };

				auto q = wuint::umul256_upper_middle64(shift_result, c);
				return std::uint32_t(shift_result.low()) - segment_divisor * std::uint32_t(q);
			}
		}
	};
}

#include "detail/undef_macros.h"
#endif