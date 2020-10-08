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

#include "jkj/fp/dooly.h"

#include <iostream>
#include <iomanip>
#include <string>

template <class Float>
static void live_test()
{
	char buffer[41];

	while (true) {
		// TODO
	}
}

int main()
{
	constexpr enum {
		test_float,
		test_double
	} test = test_double;

	if constexpr (test == test_float) {
		std::cout << "[Start limited-precision parsing live test for binary32]\n";
		live_test<float>();
	}
	else if constexpr (test == test_double) {
		std::cout << "[Start limited-precision parsing live test for binary64]\n";
		live_test<double>();
	}
}
