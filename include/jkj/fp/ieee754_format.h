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

#ifndef JKJ_HEADER_FP_IEEE754_FORMAT
#define JKJ_HEADER_FP_IEEE754_FORMAT

#include "detail/util.h"
#include <cstring>

namespace jkj::fp {
	enum class ieee754_format {
		binary32,
		binary64
	};

	template <ieee754_format format_>
	struct ieee754_format_info;

	template <>
	struct ieee754_format_info<ieee754_format::binary32> {
		static constexpr auto format = ieee754_format::binary32;
		static constexpr int significand_bits = 23;
		static constexpr int exponent_bits = 8;
		static constexpr int min_exponent = -126;
		static constexpr int max_exponent = 127;
		static constexpr int exponent_bias = -127;
		static constexpr int decimal_digits = 9;
	};

	template <>
	struct ieee754_format_info<ieee754_format::binary64> {
		static constexpr auto format = ieee754_format::binary64;
		static constexpr int significand_bits = 52;
		static constexpr int exponent_bits = 11;
		static constexpr int min_exponent = -1022;
		static constexpr int max_exponent = 1023;
		static constexpr int exponent_bias = -1023;
		static constexpr int decimal_digits = 17;
	};

	// To reduce boilerplates
	template <class T>
	struct default_ieee754_traits {
		static_assert(detail::physical_bits<T> == 32 || detail::physical_bits<T> == 64);

		using type = T;
		static constexpr ieee754_format format =
			detail::physical_bits<T> == 32 ? ieee754_format::binary32 : ieee754_format::binary64;

		using carrier_uint = std::conditional_t<
			detail::physical_bits<T> == 32,
			std::uint32_t,
			std::uint64_t>;
		static_assert(sizeof(carrier_uint) == sizeof(T));

		static constexpr int carrier_bits = int(detail::physical_bits<carrier_uint>);

		static T carrier_to_float(carrier_uint u) noexcept {
			T x;
			std::memcpy(&x, &u, sizeof(carrier_uint));
			return x;
		}
		static carrier_uint float_to_carrier(T x) noexcept {
			carrier_uint u;
			std::memcpy(&u, &x, sizeof(carrier_uint));
			return u;
		}

		static constexpr unsigned int extract_exponent_bits(carrier_uint u) noexcept {
			constexpr int significand_bits = ieee754_format_info<format>::significand_bits;
			constexpr int exponent_bits = ieee754_format_info<format>::exponent_bits;
			static_assert(detail::value_bits<unsigned int> > exponent_bits);
			constexpr auto exponent_bits_mask = (unsigned int)(((unsigned int)(1) << exponent_bits) - 1);
			return (unsigned int)((u >> significand_bits) & exponent_bits_mask);
		}
		static constexpr carrier_uint extract_significand_bits(carrier_uint u) noexcept {
			constexpr int significand_bits = ieee754_format_info<format>::significand_bits;
			constexpr auto significand_bits_mask = carrier_uint((carrier_uint(1) << significand_bits) - 1);
			return carrier_uint(u & significand_bits_mask);
		}

		// Allows positive zero and positive NaN's, but not allow negative zero.
		static constexpr bool is_positive(carrier_uint u) noexcept {
			return (u >> (carrier_bits - 1)) == 0;
		}
		// Allows negative zero and negative NaN's, but not allow positive zero.
		static constexpr bool is_negative(carrier_uint u) noexcept {
			return (u >> (carrier_bits - 1)) != 0;
		}

		static constexpr int exponent_bias = 1 - (1 << (carrier_bits - ieee754_format_info<format>::significand_bits - 2));

		static constexpr bool is_finite(carrier_uint u) noexcept {
			constexpr int significand_bits = ieee754_format_info<format>::significand_bits;
			constexpr int exponent_bits = ieee754_format_info<format>::exponent_bits;
			constexpr auto exponent_bits_mask =
				carrier_uint(((carrier_uint(1) << exponent_bits) - 1) << significand_bits);

			return (u & exponent_bits_mask) != exponent_bits_mask;
		}
		static constexpr bool is_nonzero(carrier_uint u) noexcept {
			return (u << 1) != 0;
		}
		// Allows positive and negative zeros.
		static constexpr bool is_subnormal(carrier_uint u) noexcept {
			constexpr int significand_bits = ieee754_format_info<format>::significand_bits;
			constexpr int exponent_bits = ieee754_format_info<format>::exponent_bits;
			constexpr auto exponent_bits_mask =
				carrier_uint(((carrier_uint(1) << exponent_bits) - 1) << significand_bits);

			return (u & exponent_bits_mask) == 0;
		}
		static constexpr bool is_positive_infinity(carrier_uint u) noexcept {
			constexpr int significand_bits = ieee754_format_info<format>::significand_bits;
			constexpr int exponent_bits = ieee754_format_info<format>::exponent_bits;
			constexpr auto positive_infinity =
				carrier_uint((carrier_uint(1) << exponent_bits) - 1) << significand_bits;
			return u == positive_infinity;
		}
		static constexpr bool is_negative_infinity(carrier_uint u) noexcept {
			constexpr int significand_bits = ieee754_format_info<format>::significand_bits;
			constexpr int exponent_bits = ieee754_format_info<format>::exponent_bits;
			constexpr auto negative_infinity =
				(carrier_uint((carrier_uint(1) << exponent_bits) - 1) << significand_bits)
				| (carrier_uint(1) << (carrier_bits - 1));
			return u == negative_infinity;
		}
		static constexpr bool is_infinity(carrier_uint u) noexcept {
			return is_positive_infinity(u) || is_negative_infinity(u);
		}
		static constexpr bool is_nan(carrier_uint u) noexcept {
			return !is_finite(u) && (extract_significand_bits(u) != 0);
		}
	};

	// Speciailze this class template for possible extensions.
	template <class T>
	struct ieee754_traits : default_ieee754_traits<T> {
		// I don't know if there is a truly reliable way of detecting
		// IEEE-754 binary32/binary64 formats; I just did my best here.
		static_assert(std::numeric_limits<T>::is_iec559&&
			std::numeric_limits<T>::radix == 2 &&
			(detail::physical_bits<T> == 32 || detail::physical_bits<T> == 64),
			"default_ieee754_traits only worsk for 32-bits or 64-bits types "
			"supporting binary32 or binary64 formats!");
	};

	// Convenient wrapper for ieee754_traits
	// In order to reduce the argument passing overhead,
	// this class should be as simple as possible
	// (e.g., no inheritance, no private non-static data member, etc.;
	// this is an unfortunate fact about x64 calling convention).
	template <class T>
	struct ieee754_bits {
		using carrier_uint = typename ieee754_traits<T>::carrier_uint;
		carrier_uint u;

		ieee754_bits() = default;
		constexpr explicit ieee754_bits(carrier_uint u) noexcept : u{ u } {}
		constexpr explicit ieee754_bits(T x) noexcept : u{ ieee754_traits<T>::float_to_carrier(x) } {}

		constexpr T to_float() const noexcept {
			return ieee754_traits<T>::carrier_to_float(u);
		}

		constexpr carrier_uint extract_significand_bits() const noexcept {
			return ieee754_traits<T>::extract_significand_bits(u);
		}
		constexpr unsigned int extract_exponent_bits() const noexcept {
			return ieee754_traits<T>::extract_exponent_bits(u);
		}

		constexpr carrier_uint binary_significand() const noexcept {
			using format_info = ieee754_format_info<ieee754_traits<T>::format>;
			auto s = extract_significand_bits();
			if (extract_exponent_bits() == 0) {
				return s;
			}
			else {
				return s | (carrier_uint(1) << format_info::significand_bits);
			}
		}
		constexpr int binary_exponent() const noexcept {
			using format_info = ieee754_format_info<ieee754_traits<T>::format>;
			auto e = extract_exponent_bits();
			if (e == 0) {
				return format_info::min_exponent;
			}
			else {
				return e + format_info::exponent_bias;
			}
		}

		constexpr bool is_finite() const noexcept {
			return ieee754_traits<T>::is_finite(u);
		}
		constexpr bool is_nonzero() const noexcept {
			return ieee754_traits<T>::is_nonzero(u);
		}
		// Allows positive and negative zeros
		constexpr bool is_subnormal() const noexcept {
			return ieee754_traits<T>::is_subnormal(u);
		}
		// Allows positive zero and positive NaN's, but not allow negative zero
		constexpr bool is_positive() const noexcept {
			return ieee754_traits<T>::is_positive(u);
		}
		// Allows negative zero and negative NaN's, but not allow positive zero
		constexpr bool is_negative() const noexcept {
			return ieee754_traits<T>::is_negative(u);
		}
		constexpr bool is_positive_infinity() const noexcept {
			return ieee754_traits<T>::is_positive_infinity(u);
		}

		constexpr bool is_negative_infinity() const noexcept {
			return ieee754_traits<T>::is_negative_infinity(u);
		}
		// Allows both plus and minus infinities
		constexpr bool is_infinity() const noexcept {
			return ieee754_traits<T>::is_infinity(u);
		}
		constexpr bool is_nan() const noexcept {
			return ieee754_traits<T>::is_nan(u);
		}
	};
}

#endif