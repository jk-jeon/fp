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

#if defined(_MSC_VER) && _MSC_VER >= 1915

#include "from_chars_unlimited_precision_benchmark.h"
#include <charconv>

namespace {
	float float_from_chars(std::string const& str)
	{
		float value;
		std::from_chars(str.data(), str.data() + str.length(), value);
		return value;

	}
	double double_from_chars(std::string const& str)
	{
		double value;
		std::from_chars(str.data(), str.data() + str.length(), value);
		return value;
	}

	auto dummy = []() -> register_function_for_from_chars_unlimited_precision_benchmark {
		return{ "std::from_chars",
			float_from_chars,
			double_from_chars
		};
	}();
}

#endif