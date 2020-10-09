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

#include "to_chars_shortest_roundtrip_benchmark.h"
#include "fp_to_chars.h"

namespace {
	void float_to_chars(float x, char* buffer)
	{
		jkj::fp_to_chars(x, buffer,
			jkj::grisu_exact_rounding_modes::nearest_to_even{},
			jkj::grisu_exact_correct_rounding::tie_to_even{});
	}
	void double_to_chars(double x, char* buffer)
	{
		jkj::fp_to_chars(x, buffer,
			jkj::grisu_exact_rounding_modes::nearest_to_even{},
			jkj::grisu_exact_correct_rounding::tie_to_even{});
	}

	auto dummy = []() -> register_function_for_to_chars_shortest_roundtrip_benchmark {
		if constexpr (benchmark_kind == benchmark_no_trailing_zero) {
			return { "Grisu-Exact",
				float_to_chars,
				double_to_chars
			};
		}
		else {
			return {};
		}
	}();
}
