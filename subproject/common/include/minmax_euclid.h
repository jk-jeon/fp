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

#ifndef JKJ_HEADER_FP_MINMAX_EUCLID
#define JKJ_HEADER_FP_MINMAX_EUCLID

////////////////////////////////////////////////////////////////////////////////////////
// This file is only used for cache generation, and need not be included for real use
////////////////////////////////////////////////////////////////////////////////////////

#include "bigint.h"
#include <optional>

namespace jkj::fp {
	namespace detail {
		// Improved min-max Euclid algorithm
		// Precondition: a, b, N are positive integers
		template <std::size_t array_size>
		struct minmax_euclid_return {
			bigint_impl<array_size> min;
			bigint_impl<array_size> max;
			bigint_impl<array_size> argmin;
			bigint_impl<array_size> argmax;
		};

		template <std::size_t array_size>
		minmax_euclid_return<array_size> minmax_euclid(
			bigint_impl<array_size> const& a,
			bigint_impl<array_size> const& b,
			bigint_impl<array_size> const& N)
		{
			assert(!a.is_zero());
			assert(!b.is_zero());
			assert(!N.is_zero());

			using bigint_t = bigint_impl<array_size>;

			minmax_euclid_return<array_size> ret;
			ret.max = b;

			bigint_t ai = a;
			bigint_t bi = b;
			bigint_t si = 1;
			bigint_t ui = 0;

			while (true) {
				// Update ui and bi
				auto new_b = bi;
				auto qi = new_b.long_division(ai);
				if (new_b == 0) {
					assert(qi > 0);
					--qi;
					new_b = ai;
				}
				auto new_u = qi * si + ui;

				if (new_u > N) {
					// Find 0 < k < qi such that ui + k*si <= N < ui + (k+1)*si
					auto k = (N - ui).long_division(si);

					// si <= N < new_u
					ret.min = ai;
					ret.argmin = si;
					ret.max -= bi;
					ret.max += k * ai;
					ret.argmax = ui + k * si;

					break;
				}

				// Update si and ai
				auto new_a = ai;
				auto pi = new_a.long_division(new_b);
				if (new_a == 0) {
					assert(pi > 0);
					--pi;
					new_a = new_b;
				}
				auto new_s = pi * new_u + si;

				if (new_s > N) {
					// Find 0 < k < pi such that si + k*u(i+1) <= N < si + (k+1)*u(i+1)
					auto k = (N - si).long_division(new_u);

					// new_u <= N < new_s
					ret.min = ai;
					ret.min -= k * new_b;
					ret.argmin = si + k * new_u;
					ret.max -= new_b;
					ret.argmax = new_u;

					break;
				}

				if (new_b == bi && new_a == ai) {
					// Reached to the gcd
					assert(ui == new_u);
					assert(si == new_s);

					ret.max -= new_b;
					ret.argmax = new_u;

					auto sum_idx = new_s + new_u;
					if (sum_idx > N) {
						ret.min = new_a;
						ret.argmin = new_s;
					}
					else {
						ret.min = 0;
						ret.argmin = sum_idx;
					}

					break;
				}

				bi = new_b;
				ui = new_u;
				ai = new_a;
				si = new_s;
			}

			return ret;
		}

		template <std::size_t array_size>
		struct bit_reduction_return {
			bigint_impl<array_size>	resulting_number;
			std::uint8_t			round_direction;	// 0 means floor, 1 means ceiling
		};

		// Check if we have either
		// floor(f * g / 2^b) = floor(f * floor(g/2^l) / 2^(b-l)) or
		// floor(f * g / 2^b) = floor(f * (floor(g/2^l)+1) / 2^(b-l))
		// for all f = 0, ... , N
		// Precondition: array_size should be large enough
		template <std::size_t array_size>
		std::optional<bit_reduction_return<array_size>> multiplier_right_shift(
			bigint_impl<array_size> const& g, int b, int l,
			bigint_impl<array_size> const& N)
		{
			assert(N >= 1);
			using bigint_t = bigint_impl<array_size>;

			bit_reduction_return<array_size> ret;

			if (l <= 0) {
				ret.resulting_number = (g << std::size_t(-l));
				ret.round_direction = 0;
				return ret;
			}
			else if (g.is_zero() || l <= int(g.count_factor_of_2())) {
				ret.resulting_number = (g >> std::size_t(l));
				ret.round_direction = 0;
				return ret;
			}
			else if (b < 0) {
				// Fail
				return{};
			}

			auto divisor = bigint_t::power_of_2(std::size_t(b));
			auto minmax_euclid_result = minmax_euclid(g, divisor, N);

			auto lower_bits_of_g = lower_bits_of(g, std::size_t(l));

			// Try floor
			if (minmax_euclid_result.max + lower_bits_of_g * N < divisor)
			{
				ret.resulting_number = (g >> std::size_t(l));
				ret.round_direction = 0;
				return ret;
			}
			// Try ceiling
			else if (minmax_euclid_result.min >=
				(bigint_t::power_of_2(std::size_t(l)) - lower_bits_of_g) * N)
			{
				ret.resulting_number = (g >> std::size_t(l)) + 1;
				ret.round_direction = 1;
				return ret;
			}
			
			// Fail
			return{};
		}

		// Calculate an upper bound for the minimum number of required bits for the above routine
		constexpr int required_bits_for_multiplier_right_shift(
			int max_g_bits, int max_b, int min_l, int max_l,
			int max_N_bits) noexcept
		{
			assert(max_g_bits > 0);
			assert(max_N_bits > 0);
			assert(min_l <= max_l);
			int ret = max_g_bits;

			// ret.resulting_number = (g << std::size_t(-l));
			if (min_l < 0) {
				ret = max_g_bits - min_l;
			}
			// auto divisor = bigint_t::power_of_2(std::size_t(b));
			if (max_b > 0) {
				if (max_b + 1 > ret) {
					ret = max_b + 1;
				}
			}
			// if (minmax_euclid_result.max + lower_bits_of_g * N < divisor)
			if (max_g_bits + max_N_bits - 1 > ret) {
				ret = max_g_bits + max_N_bits - 1;
			}
			// else if (minmax_euclid_result.min >=
			//     (bigint_t::power_of_2(std::size_t(l)) - lower_bits_of_g) * N)
			if (max_l + 1 + max_N_bits - 1 > ret) {
				ret = max_l + 1 + max_N_bits - 1;
			}
			// ret.resulting_number = (g >> std::size_t(l)) + 1;
			if (max_g_bits + 1 > ret) {
				ret = max_g_bits + 1;
			}
			return ret;
		}

		// Check if we have either
		// floor(f * 2^b / g) = floor(f * floor(2^u / g) / 2^(u-b)) or
		// floor(f * 2^b / g) = floor(f * (floor(2^u / g)+1) / 2^(u-b))
		// for all f = 0, ... , N
		// Precondition: array_size should be large enough
		template <std::size_t array_size>
		std::optional<bit_reduction_return<array_size>> reciprocal_left_shift(
			bigint_impl<array_size> const& g, int b, int u,
			bigint_impl<array_size> const& N)
		{
			assert(N >= 1);
			assert(!g.is_zero());
			using bigint_t = bigint_impl<array_size>;

			bit_reduction_return<array_size> ret;

			auto gp = g;
			if (b < 0) {
				gp <<= std::size_t(-b);
				u -= b;
				b = 0;
			}

			if (u < 0) {
				// Try floor
				if (bigint_t::power_of_2(std::size_t(b)) * N < g) {
					ret.resulting_number = 0;
					ret.round_direction = 0;
					return ret;
				}
				else {
					// Ceiling is impossible;
					// we shouldl have floor(f * 2^(b-u)) = floor(f * 2^b / g),
					// so in particular when f = 1, 2^(b-u) = floor(2^b / g),
					// but LHS is strictly bigger than 2^b
					return{};
				}
			}

			auto minmax_euclid_result = minmax_euclid(bigint_t::power_of_2(std::size_t(b)), gp, N);

			auto pow2_mod_g = bigint_t::power_of_2(std::size_t(u));
			auto pow2_over_g = pow2_mod_g.long_division(gp);

			if (u <= b) {
				// Try floor
				if ((minmax_euclid_result.min >> std::size_t(b - u)) >= pow2_mod_g * N)
				{
					ret.resulting_number = pow2_over_g;
					ret.round_direction = 0;
					return ret;
				}
				// Try ceiling
				else {
					auto dividend = gp - minmax_euclid_result.max;
					auto threshold = (gp - pow2_mod_g) * N;
					auto test_number = (dividend >> std::size_t(b - u));
					if (lower_bits_of(dividend, std::size_t(b - u)).is_zero()) {
						++threshold;
					}
					if (test_number >= threshold) {
						ret.resulting_number = pow2_over_g + 1;
						ret.round_direction = 1;
						return ret;
					}
				}
			}
			else {
				// Try floor
				auto dividend = pow2_mod_g * N;
				auto threshold = minmax_euclid_result.min;
				auto test_number = ((pow2_mod_g * N) >> std::size_t(u - b));
				if (!lower_bits_of(dividend, std::size_t(u - b)).is_zero()) {
					++test_number;
				}
				if (test_number <= threshold)
				{
					ret.resulting_number = pow2_over_g;
					ret.round_direction = 0;
					return ret;
				}
				// Try ceiling
				else {
					if ((((gp - pow2_mod_g) * N) >> std::size_t(u - b)) < gp - minmax_euclid_result.max) {
						ret.resulting_number = pow2_over_g + 1;
						ret.round_direction = 1;
						return ret;
					}
				}
			}

			// Fail
			return{};
		}

		// Calculate an upper bound for the number of required bits for the above routine
		constexpr int required_bits_for_reciprocal_left_shift(
			int max_g_bits, int min_b, int max_b, int max_u,
			int max_N_bits) noexcept
		{
			assert(max_g_bits > 0);
			assert(max_N_bits > 0);
			assert(min_b <= max_b);
			int ret = max_g_bits;

			// gp <<= std::size_t(-b);
			if (min_b < 0) {
				ret = max_g_bits - min_b;
				max_u -= min_b;
				max_g_bits -= min_b;
			}
			// if (bigint_t::power_of_2(std::size_t(b)) * N < g)
			if (max_b + 1 + max_N_bits - 1 > ret) {
				ret = max_b + 1 + max_N_bits - 1;
			}
			// auto minmax_euclid_result = minmax_euclid(bigint_t::power_of_2(std::size_t(b)), gp, N)
			// Redundant (above)
			// auto pow2_mod_g = bigint_t::power_of_2(std::size_t(u))
			// Redundant (below)
			// if ((minmax_euclid_result.min >> std::size_t(b - u)) >= pow2_mod_g * N)
			if (max_u + 1 + max_N_bits - 1 > ret) {
				ret = max_u + 1 + max_N_bits - 1;
			}
			// auto threshold = (gp - pow2_mod_g) * N;
			// ++threshold
			if (max_g_bits + max_N_bits > ret) {
				ret = max_g_bits + max_N_bits;
			}
			// Others are redundant
			return ret;
		}
	}
}

#endif