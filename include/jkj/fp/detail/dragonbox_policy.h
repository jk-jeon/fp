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

#ifndef JKJ_HEADER_FP_DRAGONBOX_POLICY
#define JKJ_HEADER_FP_DRAGONBOX_POLICY

#include "../decimal_fp.h"
#include "../ieee754_format.h"
#include "log.h"
#include "dragonbox_cache.h"

namespace jkj::fp {
	namespace detail {
		namespace dragonbox {
			// Forward declare the implementation class
			template <class Float>
			struct impl;

			namespace policy {
				// Sign policy
				namespace sign {
					struct base {};

					struct ignore : base {
						using sign_policy = ignore;
						static constexpr bool return_has_sign = false;

						template <class Float, class Fp>
						static constexpr void handle_sign(ieee754_bits<Float>, Fp&) noexcept {}
					};

					struct return_sign : base {
						using sign_policy = return_sign;
						static constexpr bool return_has_sign = true;

						template <class Float, class Fp>
						static constexpr void handle_sign(ieee754_bits<Float> br, Fp& fp) noexcept {
							fp.is_negative = br.is_negative();
						}
					};
				}

				// Trailing zero policy
				namespace trailing_zero {
					struct base {};

					struct ignore : base {
						using trailing_zero_policy = ignore;
						static constexpr bool report_trailing_zeros = false;

						template <class Fp>
						static constexpr void on_trailing_zeros(Fp&) noexcept {}

						template <class Fp>
						static constexpr void no_trailing_zeros(Fp&) noexcept {}
					};

					struct remove : base {
						using trailing_zero_policy = remove;
						static constexpr bool report_trailing_zeros = false;

						template <class Fp>
						static constexpr void on_trailing_zeros(Fp& fp) noexcept {
							fp.exponent +=
								impl<typename Fp::float_type>::remove_trailing_zeros(fp.significand);
						}

						template <class Fp>
						static constexpr void no_trailing_zeros(Fp&) noexcept {}
					};

					struct report : base {
						using trailing_zero_policy = report;
						static constexpr bool report_trailing_zeros = true;

						template <class Fp>
						static constexpr void on_trailing_zeros(Fp& fp) noexcept {
							fp.may_have_trailing_zeros = true;
						}

						template <class Fp>
						static constexpr void no_trailing_zeros(Fp& fp) noexcept {
							fp.may_have_trailing_zeros = false;
						}
					};
				}

				// Binary (input) rounding mode policy
				namespace binary_rounding {
					struct base {};

					enum class tag_t {
						to_nearest,
						left_closed_directed,
						right_closed_directed
					};
					namespace interval_type {
						struct symmetric_boundary {
							static constexpr bool is_symmetric = true;
							bool is_closed;
							constexpr bool include_left_endpoint() const noexcept {
								return is_closed;
							}
							constexpr bool include_right_endpoint() const noexcept {
								return is_closed;
							}
						};
						struct asymmetric_boundary {
							static constexpr bool is_symmetric = false;
							bool is_left_closed;
							constexpr bool include_left_endpoint() const noexcept {
								return is_left_closed;
							}
							constexpr bool include_right_endpoint() const noexcept {
								return !is_left_closed;
							}
						};
						struct closed {
							static constexpr bool is_symmetric = true;
							static constexpr bool include_left_endpoint() noexcept {
								return true;
							}
							static constexpr bool include_right_endpoint() noexcept {
								return true;
							}
						};
						struct open {
							static constexpr bool is_symmetric = true;
							static constexpr bool include_left_endpoint() noexcept {
								return false;
							}
							static constexpr bool include_right_endpoint() noexcept {
								return false;
							}
						};
						struct left_closed_right_open {
							static constexpr bool is_symmetric = false;
							static constexpr bool include_left_endpoint() noexcept {
								return true;
							}
							static constexpr bool include_right_endpoint() noexcept {
								return false;
							}
						};
						struct right_closed_left_open {
							static constexpr bool is_symmetric = false;
							static constexpr bool include_left_endpoint() noexcept {
								return false;
							}
							static constexpr bool include_right_endpoint() noexcept {
								return true;
							}
						};
					}

					struct nearest_to_even : base {
						using binary_rounding_policy = nearest_to_even;
						static constexpr auto tag = tag_t::to_nearest;

						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
							return f(nearest_to_even{});
						}

						template <class Float>
						static constexpr interval_type::symmetric_boundary
							interval_type_normal(ieee754_bits<Float> br) noexcept
						{
							return{ br.u % 2 == 0 };
						}
						template <class Float>
						static constexpr interval_type::closed
							interval_type_shorter(ieee754_bits<Float>) noexcept
						{
							return{};
						}
					};
					struct nearest_to_odd : base {
						using binary_rounding_policy = nearest_to_odd;
						static constexpr auto tag = tag_t::to_nearest;

						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
							return f(nearest_to_odd{});
						}

						template <class Float>
						static constexpr interval_type::symmetric_boundary
							interval_type_normal(ieee754_bits<Float> br) noexcept
						{
							return{ br.u % 2 != 0 };
						}
						template <class Float>
						static constexpr interval_type::closed
							interval_type_shorter(ieee754_bits<Float>) noexcept
						{
							return{};
						}
					};
					struct nearest_toward_plus_infinity : base {
						using binary_rounding_policy = nearest_toward_plus_infinity;
						static constexpr auto tag = tag_t::to_nearest;

						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
							return f(nearest_toward_plus_infinity{});
						}

						template <class Float>
						static constexpr interval_type::asymmetric_boundary
							interval_type_normal(ieee754_bits<Float> br) noexcept
						{
							return{ !br.is_negative() };
						}
						template <class Float>
						static constexpr interval_type::asymmetric_boundary
							interval_type_shorter(ieee754_bits<Float> br) noexcept
						{
							return{ !br.is_negative() };
						}
					};
					struct nearest_toward_minus_infinity : base {
						using binary_rounding_policy = nearest_toward_minus_infinity;
						static constexpr auto tag = tag_t::to_nearest;

						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
							return f(nearest_toward_minus_infinity{});
						}

						template <class Float>
						static constexpr interval_type::asymmetric_boundary
							interval_type_normal(ieee754_bits<Float> br) noexcept
						{
							return{ br.is_negative() };
						}
						template <class Float>
						static constexpr interval_type::asymmetric_boundary
							interval_type_shorter(ieee754_bits<Float> br) noexcept
						{
							return{ br.is_negative() };
						}
					};
					struct nearest_toward_zero : base {
						using binary_rounding_policy = nearest_toward_zero;
						static constexpr auto tag = tag_t::to_nearest;

						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
							return f(nearest_toward_zero{});
						}
						template <class Float>
						static constexpr interval_type::right_closed_left_open
							interval_type_normal(ieee754_bits<Float>) noexcept
						{
							return{};
						}
						template <class Float>
						static constexpr interval_type::right_closed_left_open
							interval_type_shorter(ieee754_bits<Float>) noexcept
						{
							return{};
						}
					};
					struct nearest_away_from_zero : base {
						using binary_rounding_policy = nearest_away_from_zero;
						static constexpr auto tag = tag_t::to_nearest;

						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
							return f(nearest_away_from_zero{});
						}
						template <class Float>
						static constexpr interval_type::left_closed_right_open
							interval_type_normal(ieee754_bits<Float>) noexcept
						{
							return{};
						}
						template <class Float>
						static constexpr interval_type::left_closed_right_open
							interval_type_shorter(ieee754_bits<Float>) noexcept
						{
							return{};
						}
					};

					namespace detail {
						struct nearest_always_closed {
							static constexpr auto tag = tag_t::to_nearest;

							template <class Float>
							static constexpr interval_type::closed
								interval_type_normal(ieee754_bits<Float>) noexcept
							{
								return{};
							}
							template <class Float>
							static constexpr interval_type::closed
								interval_type_shorter(ieee754_bits<Float>) noexcept
							{
								return{};
							}
						};
						struct nearest_always_open {
							static constexpr auto tag = tag_t::to_nearest;

							template <class Float>
							static constexpr interval_type::open
								interval_type_normal(ieee754_bits<Float>) noexcept
							{
								return{};
							}
							template <class Float>
							static constexpr interval_type::open
								interval_type_shorter(ieee754_bits<Float>) noexcept
							{
								return{};
							}
						};
					}

					struct nearest_to_even_static_boundary : base {
						using binary_rounding_policy = nearest_to_even_static_boundary;
						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
							if (br.u % 2 == 0) {
								return f(detail::nearest_always_closed{});
							}
							else {
								return f(detail::nearest_always_open{});
							}
						}
					};
					struct nearest_to_odd_static_boundary : base {
						using binary_rounding_policy = nearest_to_odd_static_boundary;
						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
							if (br.u % 2 == 0) {
								return f(detail::nearest_always_open{});
							}
							else {
								return f(detail::nearest_always_closed{});
							}
						}
					};
					struct nearest_toward_plus_infinity_static_boundary : base {
						using binary_rounding_policy = nearest_toward_plus_infinity_static_boundary;
						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
							if (br.is_negative()) {
								return f(nearest_toward_zero{});
							}
							else {
								return f(nearest_away_from_zero{});
							}
						}
					};
					struct nearest_toward_minus_infinity_static_boundary : base {
						using binary_rounding_policy = nearest_toward_minus_infinity_static_boundary;
						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
							if (br.is_negative()) {
								return f(nearest_away_from_zero{});
							}
							else {
								return f(nearest_toward_zero{});
							}
						}
					};

					namespace detail {
						struct left_closed_directed {
							static constexpr auto tag = tag_t::left_closed_directed;

							template <class Float>
							static constexpr interval_type::left_closed_right_open
								interval_type_normal(ieee754_bits<Float>) noexcept
							{
								return{};
							}
						};
						struct right_closed_directed {
							static constexpr auto tag = tag_t::right_closed_directed;

							template <class Float>
							static constexpr interval_type::right_closed_left_open
								interval_type_normal(ieee754_bits<Float>) noexcept
							{
								return{};
							}
						};
					}

					struct toward_plus_infinity : base {
						using binary_rounding_policy = toward_plus_infinity;
						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
							if (br.is_negative()) {
								return f(detail::left_closed_directed{});
							}
							else {
								return f(detail::right_closed_directed{});
							}
						}
					};
					struct toward_minus_infinity : base {
						using binary_rounding_policy = toward_minus_infinity;
						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float> br, Func&& f) noexcept {
							if (br.is_negative()) {
								return f(detail::right_closed_directed{});
							}
							else {
								return f(detail::left_closed_directed{});
							}
						}
					};
					struct toward_zero : base {
						using binary_rounding_policy = toward_zero;
						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
							return f(detail::left_closed_directed{});
						}
					};
					struct away_from_zero : base {
						using binary_rounding_policy = away_from_zero;
						template <class Float, class Func>
						static auto delegate(ieee754_bits<Float>, Func&& f) noexcept {
							return f(detail::right_closed_directed{});
						}
					};
				}

				// Decimal (output) rounding mode policy
				namespace decimal_rounding {
					struct base {};

					enum class tag_t {
						do_not_care,
						to_even,
						to_odd,
						away_from_zero,
						toward_zero
					};

					struct do_not_care : base {
						using decimal_rounding_policy = do_not_care;
						static constexpr auto tag = tag_t::do_not_care;

						template <class Fp>
						static constexpr void break_rounding_tie(Fp&) noexcept {}
					};

					struct to_even : base {
						using decimal_rounding_policy = to_even;
						static constexpr auto tag = tag_t::to_even;

						template <class Fp>
						static constexpr void break_rounding_tie(Fp& fp) noexcept
						{
							fp.significand = fp.significand % 2 == 0 ?
								fp.significand : fp.significand - 1;
						}
					};

					struct to_odd : base {
						using decimal_rounding_policy = to_odd;
						static constexpr auto tag = tag_t::to_odd;

						template <class Fp>
						static constexpr void break_rounding_tie(Fp& fp) noexcept
						{
							fp.significand = fp.significand % 2 != 0 ?
								fp.significand : fp.significand - 1;
						}
					};

					struct away_from_zero : base {
						using decimal_rounding_policy = away_from_zero;
						static constexpr auto tag = tag_t::away_from_zero;

						template <class Fp>
						static constexpr void break_rounding_tie(Fp& fp) noexcept {}
					};

					struct toward_zero : base {
						using decimal_rounding_policy = toward_zero;
						static constexpr auto tag = tag_t::toward_zero;

						template <class Fp>
						static constexpr void break_rounding_tie(Fp& fp) noexcept
						{
							--fp.significand;
						}
					};
				}

				// Cache policy
				namespace cache {
					struct base {};

					struct fast : base {
						using cache_policy = fast;
						template <ieee754_format format>
						static constexpr typename cache_holder<format>::cache_entry_type get_cache(int k) noexcept {
							assert(k >= cache_holder<format>::min_k && k <= cache_holder<format>::max_k);
							return cache_holder<format>::cache[std::size_t(k - cache_holder<format>::min_k)];
						}
					};

					struct compact : base {
						using cache_policy = compact;
						template <ieee754_format format>
						static constexpr typename cache_holder<format>::cache_entry_type get_cache(int k) noexcept {
							assert(k >= cache_holder<format>::min_k && k <= cache_holder<format>::max_k);

							if constexpr (format == ieee754_format::binary64)
							{
								// Compute base index
								auto cache_index = (k - cache_holder<format>::min_k) /
									compressed_cache_detail::compression_ratio;
								auto kb = cache_index * compressed_cache_detail::compression_ratio
									+ cache_holder<format>::min_k;
								auto offset = k - kb;

								// Get base cache
								auto base_cache = compressed_cache_detail::cache.table[cache_index];

								if (offset == 0) {
									return base_cache;
								}
								else {
									// Compute the required amount of bit-shift
									auto alpha = log::floor_log2_pow10(kb + offset) - log::floor_log2_pow10(kb) - offset;
									assert(alpha > 0 && alpha < 64);

									// Try to recover the real cache
									auto pow5 = compressed_cache_detail::pow5.table[offset];
									auto recovered_cache = wuint::umul128(base_cache.high(), pow5);
									auto middle_low = wuint::umul128(base_cache.low() - (kb < 0 ? 1 : 0), pow5);

									recovered_cache += middle_low.high();

									auto high_to_middle = recovered_cache.high() << (64 - alpha);
									auto middle_to_low = recovered_cache.low() << (64 - alpha);

									recovered_cache = wuint::uint128{
										(recovered_cache.low() >> alpha) | high_to_middle,
										((middle_low.low() >> alpha) | middle_to_low)
									};

									if (kb < 0) {
										recovered_cache += 1;
									}

									// Get error
									auto error_idx = (k - cache_holder<format>::min_k) / 16;
									auto error = (compressed_cache_detail::errors[error_idx] >>
										((k - cache_holder<format>::min_k) % 16) * 2) & 0x3;

									// Add the error back
									assert(recovered_cache.low() + error >= recovered_cache.low());
									recovered_cache = {
										recovered_cache.high(),
										recovered_cache.low() + error
									};

									return recovered_cache;
								}
							}
							else
							{
								return cache_holder<format>::cache[std::size_t(k - cache_holder<format>::min_k)];
							}
						}
					};
				}

				namespace input_validation {
					struct base {};

					struct assert_finite : base {
						using input_validation_policy = assert_finite;
						template <class Float>
						static void validate_input([[maybe_unused]] ieee754_bits<Float> br) noexcept
						{
							assert(br.is_finite());
						}
					};

					struct do_nothing : base {
						using input_validation_policy = do_nothing;
						template <class Float>
						static void validate_input(ieee754_bits<Float>) noexcept {}
					};
				}
			}
		}
	}

	namespace policy {
		namespace sign {
			static constexpr auto ignore = detail::dragonbox::policy::sign::ignore{};
			static constexpr auto return_sign = detail::dragonbox::policy::sign::return_sign{};
		}

		namespace trailing_zero {
			static constexpr auto ignore = detail::dragonbox::policy::trailing_zero::ignore{};
			static constexpr auto remove = detail::dragonbox::policy::trailing_zero::remove{};
			static constexpr auto report = detail::dragonbox::policy::trailing_zero::report{};
		}

		namespace binary_rounding {
			static constexpr auto nearest_to_even =
				detail::dragonbox::policy::binary_rounding::nearest_to_even{};
			static constexpr auto nearest_to_odd =
				detail::dragonbox::policy::binary_rounding::nearest_to_odd{};
			static constexpr auto nearest_toward_plus_infinity =
				detail::dragonbox::policy::binary_rounding::nearest_toward_plus_infinity{};
			static constexpr auto nearest_toward_minus_infinity =
				detail::dragonbox::policy::binary_rounding::nearest_toward_minus_infinity{};
			static constexpr auto nearest_toward_zero =
				detail::dragonbox::policy::binary_rounding::nearest_toward_zero{};
			static constexpr auto nearest_away_from_zero =
				detail::dragonbox::policy::binary_rounding::nearest_away_from_zero{};

			static constexpr auto nearest_to_even_static_boundary =
				detail::dragonbox::policy::binary_rounding::nearest_to_even_static_boundary{};
			static constexpr auto nearest_to_odd_static_boundary =
				detail::dragonbox::policy::binary_rounding::nearest_to_odd_static_boundary{};
			static constexpr auto nearest_toward_plus_infinity_static_boundary =
				detail::dragonbox::policy::binary_rounding::nearest_toward_plus_infinity_static_boundary{};
			static constexpr auto nearest_toward_minus_infinity_static_boundary =
				detail::dragonbox::policy::binary_rounding::nearest_toward_minus_infinity_static_boundary{};

			static constexpr auto toward_plus_infinity =
				detail::dragonbox::policy::binary_rounding::toward_plus_infinity{};
			static constexpr auto toward_minus_infinity =
				detail::dragonbox::policy::binary_rounding::toward_minus_infinity{};
			static constexpr auto toward_zero =
				detail::dragonbox::policy::binary_rounding::toward_zero{};
			static constexpr auto away_from_zero =
				detail::dragonbox::policy::binary_rounding::away_from_zero{};
		}

		namespace decimal_rounding {
			static constexpr auto do_not_care =
				detail::dragonbox::policy::decimal_rounding::do_not_care{};
			static constexpr auto to_even =
				detail::dragonbox::policy::decimal_rounding::to_even{};
			static constexpr auto to_odd =
				detail::dragonbox::policy::decimal_rounding::to_odd{};
			static constexpr auto away_from_zero =
				detail::dragonbox::policy::decimal_rounding::away_from_zero{};
			static constexpr auto toward_zero =
				detail::dragonbox::policy::decimal_rounding::toward_zero{};
		}

		namespace cache {
			static constexpr auto fast = detail::dragonbox::policy::cache::fast{};
			static constexpr auto compact = detail::dragonbox::policy::cache::compact{};
		}

		namespace input_validation {
			static constexpr auto assert_finite =
				detail::dragonbox::policy::input_validation::assert_finite{};
			static constexpr auto do_nothing =
				detail::dragonbox::policy::input_validation::do_nothing{};
		}
	}
}

#endif