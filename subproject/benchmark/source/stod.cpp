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

#include "from_chars_unlimited_precision_benchmark.h"

namespace {
	float float_from_chars(std::string const& str)
	{
		// Ignore out_of_range error
		try {
			return std::stof(str);
		}
		catch (...) {
			return 0;
		}

	}
	double double_from_chars(std::string const& str)
	{
		// Ignore out_of_range error
		try {
			return std::stod(str);
		}
		catch (...) {
			return 0;
		}
	}

	auto dummy = []() -> register_function_for_from_chars_unlimited_precision_benchmark {
		return{ "stof/stod",
			float_from_chars,
			double_from_chars
		};
	}();
}