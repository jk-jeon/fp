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

#ifndef JKJ_HEADER_FP_RYU_PRINTF
#define JKJ_HEADER_FP_RYU_PRINTF

#include "ieee754_format.h"
#include "detail/div.h"
#include "detail/log.h"
#include "detail/ryu_printf_cache.h"
#include "detail/util.h"
#include "detail/macros.h"
#include <cassert>
#include <cstdint>

namespace jkj::fp {
	template <ieee754_format format>
	static constexpr int ryu_printf_segment_size = 9;

	namespace detail {
		namespace ryu_printf {
			template <ieee754_format format_>
			struct impl_base : public ieee754_format_info<format_>
			{
				static constexpr auto format = format_;

				static constexpr int segment_size = ryu_printf_segment_size<format>;
				static constexpr int segment_bit_size = log::floor_log2_pow10(segment_size) + 1;
				static_assert(segment_bit_size <= 32);
				static constexpr auto segment_divisor =
					compute_power<segment_size>(std::uint32_t(10));

				// It is possible that taking a non-maximum value can result in a shorter table.
				static constexpr int compression_factor =
					format == ieee754_format::binary32 ? 11 : 46;

				static constexpr int max_compression_factor =
					cache_holder<format>::multiply_and_reduce_result_bits -
					ieee754_format_info<format>::significand_bits - segment_bit_size;

				static_assert(compression_factor <= max_compression_factor);
			};
		}
	}

	// The core of Ryu-printf algorithm.
	// Iterates over decimal digits segments of fixed size, from left to right,
	// starting from the first nonzero segment.
	// The initial segment is in [1,10^eta) where eta is the segment size thus is of variable length,
	// while other segments are in [10^(eta-1), 10^eta) thus are of fixed length.
	// The interface is pull-oriented rather than push-oriented;
	// it is the user who controls the flow, so there is no callback mechanism.
	// The user can request the object to obtain the next segment.
	template <class Float>
	class ryu_printf : private detail::ryu_printf::impl_base<ieee754_traits<Float>::format>
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
		using cache_holder = detail::ryu_printf::cache_holder<format>;
		using cache_entry_type = typename cache_holder::cache_entry_type;

		using carrier_uint = typename ieee754_traits<Float>::carrier_uint;
		static constexpr auto carrier_bits = ieee754_traits<Float>::carrier_bits;

		static constexpr int max_power_of_factor_of_5 =
			detail::log::floor_log5_pow2(int(significand_bits + 2));

		carrier_uint significand_;
		int exponent_;
		std::uint32_t segment_;
		int segment_index_;		// n
		int exponent_index_;	// k
		int remainder_;			// r
		int max_segment_index_;

	public:
		using impl_base::segment_size;
		using impl_base::segment_divisor;

		// Computes the first segmnet on construction.
		ryu_printf(Float x) noexcept : ryu_printf(ieee754_bits{ x }) {}

		JKJ_FORCEINLINE ryu_printf(ieee754_bits<Float> br) noexcept {
			using namespace detail;

			significand_ = br.extract_significand_bits();
			exponent_ = int(br.extract_exponent_bits());

			if (exponent_ != 0) {
				// If the input is normal
				exponent_ += exponent_bias - significand_bits;
				significand_ |= (carrier_uint(1) << significand_bits);

				// n0 = floor((-e-p-1)log10(2) / eta) + 1
				// Avoid signed division.
				auto const dividend = log::floor_log10_pow2(-exponent_ - significand_bits - 1);
				if (exponent_ <= -significand_bits - 1) {
					assert(dividend >= 0);
					segment_index_ = int(unsigned(dividend) / unsigned(segment_size) + 1);
					max_segment_index_ = int(unsigned(-exponent_ + segment_size - 1) / unsigned(segment_size));
				}
				else {
					assert(dividend < 0);
					segment_index_ = -int(unsigned(-dividend) / unsigned(segment_size));
					if (exponent_ < 0) {
						max_segment_index_ = int(unsigned(-exponent_ + segment_size - 1) / unsigned(segment_size));
					}
					else {
						max_segment_index_ = 0;
					}
				}
			}
			else {
				// If the input is subnormal
				exponent_ = min_exponent - significand_bits;

				// n0 = floor((-e-p-1)log10(2) / eta) + 1
				// Avoid signed division.
				constexpr auto dividend = log::floor_log10_pow2(-min_exponent - 1);
				static_assert(dividend >= 0);
				segment_index_ = int(unsigned(dividend) / unsigned(segment_size) + 1);
				max_segment_index_ = int(unsigned(-exponent_ + segment_size - 1) / unsigned(segment_size));
			}

			// Align the implicit bit to the MSB.
			significand_ <<= (carrier_bits - significand_bits - 1);

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

			// Get first nonzero segment.
			segment_ = compute_segment();
			while (segment_ == 0) {
				++segment_index_;
				on_increase_segment_index();
			}
		}

		std::uint32_t current_segment() const noexcept {
			return segment_;
		}

		int current_segment_index() const noexcept {
			return segment_index_;
		}

		// This function does divisibility test whenever called.
		// Cache the result of this function if it needs to be called several times.
		bool has_further_nonzero_segments() const noexcept {
			if (segment_index_ >= max_segment_index_) {
				return false;
			}
			else {
				// Check if there are reamining nonzero digits,
				// which is equivalent to that f * 2^e * 10^(n * eta) is not an integer.

				auto minus_pow5_exponent = -segment_index_ * segment_size;
				auto minus_pow2_exponent = -exponent_ + minus_pow5_exponent;

				// Check the exponent of 2.
				if (minus_pow2_exponent > 0 &&
					!detail::div::divisible_by_power_of_2(significand_,
						minus_pow2_exponent + (carrier_bits - significand_bits - 1)))
				{
					return true;
				}

				// Check the exponent of 5.
				if (minus_pow5_exponent > 0 &&
					(minus_pow5_exponent > max_power_of_factor_of_5 ||
						!detail::div::divisible_by_power_of_5<max_power_of_factor_of_5 + 1>(
							significand_, minus_pow5_exponent)))
				{
					return true;
				}

				return false;
			}
		}

		JKJ_FORCEINLINE std::uint32_t compute_next_segment() noexcept {
			++segment_index_;
			if (segment_index_ <= max_segment_index_) {
				on_increase_segment_index();
			}
			else {
				segment_ = 0;
			}
			return segment_;
		}

	private:
		JKJ_FORCEINLINE std::uint32_t compute_segment() const noexcept {
			auto index_info = cache_holder::get_index_info(segment_index_);
			auto cache = cache_holder::cache[index_info.starting_index + (exponent_index_ - index_info.min_k)];
			return multiply_shift_mod(significand_, cache,
				segment_bit_size + remainder_ - carrier_bits + significand_bits + 1);
		}

		JKJ_FORCEINLINE void on_increase_segment_index() noexcept {
			assert(segment_index_ <= max_segment_index_);
			remainder_ += segment_size;
			static_assert(segment_size < compression_factor);
			if (remainder_ >= compression_factor) {
				++exponent_index_;
				remainder_ -= compression_factor;
			}
			segment_ = compute_segment();
		}

		JKJ_SAFEBUFFERS static std::uint32_t multiply_shift_mod(carrier_uint x,
			cache_entry_type const& y, int shift_amount) noexcept
		{
			using namespace detail;

			if constexpr (format == ieee754_format::binary32) {
				static_assert(value_bits<carrier_uint> <= 32);
				assert(shift_amount > 0 && shift_amount <= 32);
				auto shift_result = wuint::umul128_upper64(x, y) >> (32 - shift_amount);

				return std::uint32_t(shift_result % segment_divisor);
			}
			else {
				static_assert(format == ieee754_format::binary64);
				static_assert(value_bits<carrier_uint> <= 64);
				auto mul_result = wuint::umul256_upper128(x, y);

				assert(shift_amount > 0 && shift_amount <= 64);
				auto shift_result = mul_result >> (64 - shift_amount);

				// Granlund-Montgomery style division by 10^9
				constexpr auto L = 29;
				auto const c = wuint::uint128{ 0x8970'5f41'36b4'a597, 0x3168'0a88'f895'3031 };
				static_assert(L + segment_bit_size < 64);

				// Since the end result will be only of 32 bits, we can just take the
				// lower 32 bits of the quotient when computing the remainder.
				// Since we shift the result by 29 bits, we need to know
				// lower 32 + 19 = 61 bits of the upper 128 bits of the total 256 bits.
				auto q = wuint::umul256_upper_middle64(shift_result, c);
				return std::uint32_t(shift_result.low())
					- segment_divisor * std::uint32_t(q >> L);
			}
		}
	};
}

#include "detail/undef_macros.h"
#endif