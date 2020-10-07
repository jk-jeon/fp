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

#include "jkj/fp/to_chars/fixed_precision.h"

#include <iostream>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <string>

template <class Float>
static void live_test()
{
	auto buffer = std::make_unique<char[]>(100000);

	while (true) {
		Float x;
		std::string str;
		while (true) {
			std::cout << "Input a floating-point number: ";
			std::getline(std::cin, str);
			try {
				if constexpr (std::is_same_v<Float, float>) {
					x = std::stof(str);
				}
				else {
					x = std::stod(str);
				}
			}
			catch (...) {
				std::cout << "Not a valid input; input again.\n";
				continue;
			}
			break;
		}
		int precision;
		while (true) {
			std::cout << "Input decimal precision: ";
			std::getline(std::cin, str);
			try {
				precision = std::stoi(str);
				if (x < 0 || x > 9000) {
					throw std::out_of_range{ "out of range" };
				}
			}
			catch (...) {
				std::cout << "Not a valid input; input again.\n";
				continue;
			}
			break;
		}

		auto xx = jkj::fp::ieee754_bits<Float>{ x };
		std::cout << "              sign: " << (xx.is_negative() ? "-" : "+") << std::endl;
		std::cout << "     exponent bits: " << "0x" << std::hex << std::setfill('0')
			<< xx.extract_exponent_bits() << std::dec
			<< " (value: " << xx.binary_exponent() << ")\n";
		std::cout << "  significand bits: " << "0x" << std::hex << std::setfill('0');
		if constexpr (std::is_same_v<Float, float>) {
			std::cout << std::setw(8);
		}
		else {
			std::cout << std::setw(16);
		}
		std::cout << xx.extract_significand_bits()
			<< " (value: 0x" << xx.binary_significand() << ")\n" << std::dec;

		jkj::fp::to_chars_fixed_precision_scientific(x, buffer.get(), precision);
		std::cout << "output: " << buffer.get() << "\n\n";
	}
}

int main()
{
	constexpr enum {
		test_float,
		test_double
	} test = test_double;

	if constexpr (test == test_float) {
		std::cout << "[Start fixed-precision formatting live test for binary32]\n";
		live_test<float>();
	}
	else if constexpr (test == test_double) {
		std::cout << "[Start fixed-precision formatting live test for binary64]\n";
		live_test<double>();
	}
}
