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

#ifndef JKJ_HEADER_FP_SHORTEST_ROUNDTRIP_BENCHMARK
#define JKJ_HEADER_FP_SHORTEST_ROUNDTRIP_BENCHMARK

#include <string_view>

static constexpr enum {
	benchmark_no_trailing_zero,
	benchmark_allow_trailing_zero
} benchmark_kind = benchmark_no_trailing_zero;

struct register_function_for_to_chars_shortest_roundtrip_benchmark {
	register_function_for_to_chars_shortest_roundtrip_benchmark() = default;

	register_function_for_to_chars_shortest_roundtrip_benchmark(
		std::string_view name,
		void(*func)(float, char*));

	register_function_for_to_chars_shortest_roundtrip_benchmark(
		std::string_view name,
		void(*func)(double, char*));

	register_function_for_to_chars_shortest_roundtrip_benchmark(
		std::string_view name,
		void(*func_float)(float, char*),
		void(*func_double)(double, char*));
};

#endif
