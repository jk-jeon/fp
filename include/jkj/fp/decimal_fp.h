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


#ifndef JKJ_HEADER_FP_DECIMAL_FP
#define JKJ_HEADER_FP_DECIMAL_FP

#include "ieee754_format.h"

namespace jkj::fp {
	template <class Float, bool is_signed, bool trailing_zero_flag>
	struct decimla_fp;

	template <class Float>
	struct decimla_fp<Float, false, false> {
		using float_type = Float;
		using carrier_uint = typename ieee754_traits<Float>::carrier_uint;

		carrier_uint	significand;
		int				exponent;
	};

	template <class Float>
	struct decimla_fp<Float, true, false> {
		using float_type = Float;
		using carrier_uint = typename ieee754_traits<Float>::carrier_uint;

		carrier_uint	significand;
		int				exponent;
		bool			is_negative;
	};

	template <class Float>
	struct decimla_fp<Float, false, true> {
		using float_type = Float;
		using carrier_uint = typename ieee754_traits<Float>::carrier_uint;

		carrier_uint	significand;
		int				exponent;
		bool			may_have_trailing_zeros;
	};

	template <class Float>
	struct decimla_fp<Float, true, true> {
		using float_type = Float;
		using carrier_uint = typename ieee754_traits<Float>::carrier_uint;

		carrier_uint	significand;
		int				exponent;
		bool			is_negative;
		bool			may_have_trailing_zeros;
	};

	template <class Float>
	using unsigned_fp_t = decimla_fp<Float, false, false>;

	template <class Float>
	using signed_fp_t = decimla_fp<Float, true, false>;
}

#endif