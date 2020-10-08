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

#include "jkj/fp/dragonbox.h"
#include "jkj/fp/dooly.h"
#include "ryu/ryu.h"
#include "random_float.h"
#include <iostream>
#include <string_view>

template <class Float, class TypenameString>
static bool uniform_random_test(std::size_t number_of_tests, TypenameString&& type_name_string)
{
	auto rg = jkj::fp::detail::generate_correctly_seeded_mt19937_64();
	bool success = true;
	for (std::size_t test_idx = 0; test_idx < number_of_tests; ++test_idx) {
		auto x = jkj::fp::detail::uniformly_randomly_generate_finite_float<Float>(rg);
		auto dec = jkj::fp::to_shortest_decimal(x);
		auto roundtrip = jkj::fp::to_binary_limited_precision(dec);

		if (x != roundtrip.to_float()) {
			char buffer1[64];
			char buffer2[64];
			if constexpr (jkj::fp::ieee754_traits<Float>::format ==
				jkj::fp::ieee754_format::binary32)
			{
				f2s_buffered(x, buffer1);
				f2s_buffered(roundtrip.to_float(), buffer2);
			}
			else
			{
				d2s_buffered(x, buffer2);
				d2s_buffered(roundtrip.to_float(), buffer2);
			}
			std::cout << "Error detected! [Input = " << buffer1
				<< ", Roundtrip = " << buffer2 << "]\n";
			success = false;
		}
	}

	if (success) {
		std::cout << "Uniform random test for " << type_name_string
			<< " with " << number_of_tests << " examples succeeded.\n";
	}

	return success;
}

int main()
{
	constexpr std::size_t number_of_uniform_random_tests = 10000000;
	bool success = true;

	std::cout << "[Testing Dragonbox for uniformly randomly generated binary64 inputs...]\n";
	success &= uniform_random_test<double>(number_of_uniform_random_tests, "double");
	std::cout << "Done.\n\n\n";

	if (!success) {
		return -1;
	}
}