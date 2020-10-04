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
#include "jkj/fp/to_chars/shortest_roundtrip.h"
#include "random_float.h"
#include "ryu/ryu.h"
#include <iostream>
#include <string_view>
#include <memory>
#include <vector>

template <class Float, class TypenameString>
bool test_scientific(std::size_t number_of_samples, int max_precision, TypenameString&& type_name_string)
{
	assert(max_precision >= 0);
	auto buffer1 = std::make_unique<char[]>(10000);
	auto buffer2 = std::make_unique<char[]>(10000);
	char buffer3[41];
	bool success = true;

	// Generate random inputs
	std::cout << "Generating samples...\n";
	auto rg = jkj::fp::detail::generate_correctly_seeded_mt19937_64();
	std::vector<double> samples(number_of_samples);
	for (std::size_t i = 0; i < number_of_samples; ++i) {
		samples[i] = jkj::fp::detail::uniformly_randomly_generate_finite_float<Float>(rg);
	}
	std::cout << "Done.\n\n\n";

	// For each precision, test the output against ryu
	for (int precision = 0; precision <= max_precision; ++precision) {
		std::cout << "Testing for precision = " << precision << "...\n";

		for (auto const& sample : samples) {
			std::string_view s1{ buffer1.get(), std::size_t(
				jkj::fp::to_chars_fixed_precision_scientific_n(sample, buffer1.get(), precision) - buffer1.get()) };
			std::string_view s2{ buffer2.get(), std::size_t(
				d2exp_buffered_n(double(sample), precision, buffer2.get())) };

			if (s1 != s2) {
				jkj::fp::to_chars(sample, buffer3);
				std::cout << "Error detected! [sample = " << buffer3
					<< ", Ryu = " << s2 << ", fp = " << s1 << "]\n";
				success = false;
			}
		}

		std::cout << std::endl;
	}

	if (success) {
		std::cout << "\nUniform random test for " << type_name_string
			<< " with " << number_of_samples << " examples succeeded.\n";
	}

	return success;
}

int main()
{
	constexpr bool run_float = true;
	constexpr std::size_t number_of_samples_float = 100000;
	constexpr int max_precision_float = 120;	// max_nonzero_decimal_digits = 112

	constexpr bool run_double = true;
	constexpr std::size_t number_of_samples_double = 100000;
	constexpr int max_precision_double = 780;	// max_nonzero_decimal_digits = 767

	bool success = true;

	if constexpr (run_float) {
		std::cout << "[Testing fixed-precision scientific formatting with "
			<< "uniformly randomly generated float inputs...]\n";
		success &= test_scientific<float>(number_of_samples_float, max_precision_float, "float");
		std::cout << "Done.\n\n\n";
	}
	if constexpr (run_double) {
		std::cout << "[Testing fixed-precision scientific formatting with "
			<< "uniformly randomly generated double inputs...]\n";
		success &= test_scientific<double>(number_of_samples_double, max_precision_double, "double");
		std::cout << "Done.\n\n\n";
	}

	if (!success) {
		return -1;
	}
}