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
#include "ryu/ryu.h"

#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <string_view>
#include <stdexcept>
#include <memory>
#include <vector>

int main()
{
	using namespace jkj::fp::detail;

	while (true) {
		std::uint32_t significand;
		int exponent;

		std::cin >> significand >> exponent;
		jkj::fp::unsigned_fp_t<float> x{ significand, exponent };

		char buffer[41];
		f2s_buffered(jkj::fp::to_binary_limited_precision(x).to_float(), buffer);
		std::cout << buffer << std::endl;
	}
}