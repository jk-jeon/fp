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

#include "fixed_precision_benchmark.h"
#include "jkj/fp/to_chars/fixed_precision.h"

namespace {
	void float_to_chars(float x, char* buffer, int precision)
	{
		jkj::fp::to_chars_fixed_precision_scientific_n(x, buffer, precision);
	}
	void double_to_chars(double x, char* buffer, int precision)
	{
		jkj::fp::to_chars_fixed_precision_scientific_n(x, buffer, precision);
	}

	auto dummy = []() -> register_function_for_fixed_precision_benchmark {
		return{ "fp",
			float_to_chars,
			double_to_chars
		};
	}();
}