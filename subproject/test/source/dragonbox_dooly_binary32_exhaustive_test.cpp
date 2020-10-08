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
#include "jkj/fp/dragonbox.h"
#include "ryu/ryu.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <future>
#include <iomanip>
#include <iostream>
#include <thread>
#include <vector>

int main()
{
	std::cout << "[Joint-testing Dragonbox and Dooly for every finite non-zero binary32 inputs...]\n";

	// Find the optimal number of threads
	auto const hw_concurrency = std::max(1u, std::thread::hardware_concurrency());

	// Divide the range of inputs
	std::vector<std::uint64_t> ranges(hw_concurrency + 1);
	ranges[0] = 0;

	auto q = (std::uint64_t(1) << 32) / hw_concurrency;
	auto r = (std::uint64_t(1) << 32) % hw_concurrency;

	for (std::size_t i = 0; i < hw_concurrency; ++i) {
		ranges[i + 1] = std::uint64_t(q * i);
	}
	for (std::size_t i = 0; i < r; ++i) {
		ranges[i + 1] += i + 1;
	}
	assert(ranges[hw_concurrency] == (std::uint64_t(1) << 32));

	// Spawn threads
	using ieee754_bits = jkj::fp::ieee754_bits<float>;
	struct failure_case_t {
		ieee754_bits input;
		ieee754_bits roundtrip;
	};
	std::vector<std::future<std::vector<failure_case_t>>> tasks(hw_concurrency);
	for (std::size_t i = 0; i < hw_concurrency; ++i) {
		tasks[i] = std::async([from = ranges[i], to = ranges[i + 1]]() {
			std::vector<failure_case_t> failure_cases;

			for (std::uint32_t u = std::uint32_t(from); u < to; ++u) {
				// Exclude 0 and non-finite inputs
				ieee754_bits x{ u };
				if (x.is_nonzero() && x.is_finite()) {
					auto dec = jkj::fp::to_shortest_decimal(x.to_float());
					auto roundtrip = jkj::fp::to_binary(dec);

					if (roundtrip.u != u) {
						failure_cases.push_back({ x, roundtrip });
					}
				}
			}

			return failure_cases;
		});
	}

	// Get merged list of failure cases
	std::vector<failure_case_t> failure_cases;
	for (auto& task : tasks) {
		auto failure_cases_subset = task.get();
		failure_cases.insert(failure_cases.end(),
			failure_cases_subset.begin(), failure_cases_subset.end());
	}

	if (failure_cases.empty()) {
		std::cout << "No error case was found.\n";
	}
	else {
		std::cout << "Roundtrip fails for:\n";
		for (auto const failure_case : failure_cases) {
			char buffer[64];
			f2s_buffered(failure_case.input.to_float(), buffer);
			std::cout << "[0x" << std::hex << std::setw(8) << failure_case.input.u << "] "
				<< buffer << " (roundtrip = ";
			f2s_buffered(failure_case.roundtrip.to_float(), buffer);
			std::cout << buffer << ")\n";
		}

		std::cout << "Done.\n\n\n";
		return -1;
	}

	std::cout << "Done.\n\n\n";
}