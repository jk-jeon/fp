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

#include "jkj/fp/from_chars/from_chars.h"
#include "ryu/ryu.h"
#include <iostream>
#include <iomanip>
#include <string>

template <class Float>
static void live_test()
{
	char buffer[64];
	std::string str;

	constexpr auto ieee754_format = jkj::fp::ieee754_traits<Float>::format;
	using ieee754_format_info = jkj::fp::ieee754_format_info<ieee754_format>;
	constexpr auto hex_precision = (ieee754_format_info::significand_bits + 3) / 4;

	while (true) {
		std::cout << "Input: ";
		std::cin >> str;
		auto x = jkj::fp::from_chars_unlimited<Float>(str.data(), str.data() + str.length());
		if constexpr (ieee754_format == jkj::fp::ieee754_format::binary32) {
			f2s_buffered(x.to_float(), buffer);
		}
		else {
			d2s_buffered(x.to_float(), buffer);
		}
		std::cout << "Parsing output: " << buffer << " ["
			<< std::hexfloat << std::setprecision(hex_precision) << x.to_float() << "]\n\n";
	}
}

int main()
{
	constexpr enum {
		test_float,
		test_double
	} test = test_double;

	if constexpr (test == test_float) {
		std::cout << "[Start unlimited-precision parsing live test for binary32]\n";
		live_test<float>();
	}
	else if constexpr (test == test_double) {
		std::cout << "[Start unlimited-precision parsing live test for binary64]\n";
		live_test<double>();
	}
}
