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

#include "shortest_roundtrip_benchmark.h"
#include "schubfach_32.h"
#include "schubfach_64.h"

namespace {
	void float_to_chars(float x, char* buf) {
		schubfach::Ftoa(buf, x);
	}
	void double_to_chars(double x, char* buf) {
		schubfach::Dtoa(buf, x);
	}

	auto dummy = []() -> register_function_for_shortest_roundtrip_benchmark {
		if constexpr (benchmark_kind == benchmark_no_trailing_zero) {
			return {};
		}
		else {
			static_assert(benchmark_kind == benchmark_allow_trailing_zero);
			return { "Schubfach",
				float_to_chars,
				double_to_chars
			};
		}
	}();
}
