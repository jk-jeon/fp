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
#include "random_float.h"
#include "jkj/fp/to_chars/shortest_precise.h"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>
#include <unordered_map>
#include <vector>

template <class Float>
class benchmark_holder
{
public:
	static benchmark_holder& get_instance() {
		static benchmark_holder<Float> inst;
		return inst;
	}

	// Generate random samples
	void prepare_samples(std::size_t number_of_samples)
	{
		auto buffer = std::make_unique<char[]>(10000);
		samples_.resize(number_of_samples);
		for (auto& sample : samples_) {
			auto x = jkj::fp::detail::uniformly_randomly_generate_general_float<Float>(rg_);
			sample.assign(buffer.get(), jkj::fp::to_chars_precise_scientific_n(x, buffer.get()));
		}
	}

	// { "name" : measured_time }
	using output_type = std::unordered_map<std::string, double>;
	void run(double duration, std::string_view float_name, output_type& out)
	{
		auto dur = std::chrono::duration<double>{ duration };

		for (auto const& name_func_pair : name_func_pairs_) {
			std::cout << "Benchmarking " << name_func_pair.first << "...\n";

			auto& measured_time = out[name_func_pair.first];

			std::size_t iterations = 0;
			std::size_t sample_idx = 0;
			auto from = std::chrono::steady_clock::now();
			auto to = from + dur;
			auto now = std::chrono::steady_clock::now();

			while (now <= to) {
				name_func_pair.second(samples_[sample_idx]);

				if (++sample_idx == samples_.size()) {
					sample_idx = 0;
				}
				++iterations;
				now = std::chrono::steady_clock::now();
			}

			measured_time =
				double(std::chrono::duration_cast<std::chrono::nanoseconds>(now - from).count())
				/ iterations;

			std::cout << "Average time per iteration: " << measured_time << "ns\n";
		}
	}

	output_type run(double duration, std::string_view float_name)
	{
		output_type out;
		run(duration, float_name, out);
		return out;
	}

	void register_function(std::string_view name, Float(*func)(std::string const&))
	{
		name_func_pairs_.emplace(name, func);
	}

private:
	benchmark_holder() : rg_(jkj::fp::detail::generate_correctly_seeded_mt19937_64()) {}

	// Digits samples for [1] ~ [max_digits], general samples for [0]
	std::vector<std::string> samples_;
	std::mt19937_64 rg_;
	std::unordered_map<std::string, Float(*)(std::string const&)> name_func_pairs_;
};

register_function_for_from_chars_unlimited_precision_benchmark::register_function_for_from_chars_unlimited_precision_benchmark(
	std::string_view name,
	float(*func_float)(std::string const&))
{
	benchmark_holder<float>::get_instance().register_function(name, func_float);
};

register_function_for_from_chars_unlimited_precision_benchmark::register_function_for_from_chars_unlimited_precision_benchmark(
	std::string_view name,
	double(*func_double)(std::string const&))
{
	benchmark_holder<double>::get_instance().register_function(name, func_double);
};

register_function_for_from_chars_unlimited_precision_benchmark::register_function_for_from_chars_unlimited_precision_benchmark(
	std::string_view name,
	float(*func_float)(std::string const&),
	double(*func_double)(std::string const&))
{
	benchmark_holder<float>::get_instance().register_function(name, func_float);
	benchmark_holder<double>::get_instance().register_function(name, func_double);
};

template <class Float>
static void benchmark_test(std::string_view float_name,
	std::size_t number_of_samples, double duration)
{
	auto& inst = benchmark_holder<Float>::get_instance();
	std::cout << "Generating random samples...\n";
	inst.prepare_samples(number_of_samples);
	auto out = inst.run(duration, float_name);

	std::cout << "Benchmarking done.\n";
}

int main() {
	constexpr bool benchmark_float = true;
	constexpr std::size_t number_of_benchmark_samples_float = 1000000;
	constexpr double duration_in_sec_float = 5;

	constexpr bool benchmark_double = true;
	constexpr std::size_t number_of_benchmark_samples_double = 1000000;
	constexpr double duration_in_sec_double = 5;

	if constexpr (benchmark_float) {
		std::cout << "[Running unlimited-precision parsing benchmark for binary32...]\n";
		benchmark_test<float>("binary32",
			number_of_benchmark_samples_float,
			duration_in_sec_float);
		std::cout << "Done.\n\n\n";
	}
	if constexpr (benchmark_double) {
		std::cout << "[Running unlimited-precision parsing  benchmark for binary64...]\n";
		benchmark_test<double>("binary64",
			number_of_benchmark_samples_double,
			duration_in_sec_double);
		std::cout << "Done.\n\n\n";
	}
}