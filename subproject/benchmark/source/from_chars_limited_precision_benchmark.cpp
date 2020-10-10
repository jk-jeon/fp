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

#include "from_chars_limited_precision_benchmark.h"
#include "random_float.h"
#include "jkj/fp/to_chars/fixed_precision.h"
#include <array>
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
	static constexpr auto max_digits =
		std::size_t(std::numeric_limits<Float>::max_digits10);

	static benchmark_holder& get_instance() {
		static benchmark_holder<Float> inst;
		return inst;
	}

	// Generate random samples
	void prepare_samples(std::size_t number_of_samples_per_digits)
	{
		auto buffer = std::make_unique<char[]>(10000);

		for (unsigned int digits = 1; digits <= max_digits; ++digits) {
			samples_[digits - 1].resize(number_of_samples_per_digits);
			for (auto& sample : samples_[digits - 1]) {
				auto x = jkj::fp::detail::uniformly_randomly_generate_general_float<Float>(rg_);
				sample.assign(buffer.get(),
					jkj::fp::to_chars_fixed_precision_scientific_n(x, buffer.get(), digits - 1));
			}
		}
	}

	// { "name" : [(digits, [(sample, measured_time)])] }
	using output_type = std::unordered_map<std::string,
		std::array<std::vector<std::pair<std::string, double>>, max_digits>
	>;
	void run(std::size_t number_of_iterations, std::string_view float_name, output_type& out)
	{
		assert(number_of_iterations >= 1);

		for (auto const& name_func_pair : name_func_pairs_) {
			auto [result_array_itr, is_inserted] = out.insert_or_assign(
				name_func_pair.first,
				std::array<std::vector<std::pair<std::string, double>>, max_digits>{});

			for (unsigned int digits = 1; digits <= max_digits; ++digits) {
				(*result_array_itr).second[digits- 1].resize(samples_[digits - 1].size());
				auto out_itr = (*result_array_itr).second[digits - 1].begin();

				std::cout << "Benchmarking " << name_func_pair.first <<
					" with random " << float_name <<
					" strings of " << digits << " digits...\n";

				for (auto const& sample : samples_[digits - 1]) {
					auto from = std::chrono::high_resolution_clock::now();
					for (std::size_t i = 0; i < number_of_iterations; ++i) {
						name_func_pair.second(sample);
					}
					auto dur = std::chrono::high_resolution_clock::now() - from;

					*out_itr = { sample,
						double(std::chrono::duration_cast<std::chrono::nanoseconds>(dur).count())
						/ double(number_of_iterations) };
					++out_itr;
				}
			}
		}
	}

	output_type run(std::size_t number_of_iterations, std::string_view float_name)
	{
		output_type out;
		run(number_of_iterations, float_name, out);
		return out;
	}

	void register_function(std::string_view name, Float(*func)(std::string const&))
	{
		name_func_pairs_.emplace(name, func);
	}

private:
	benchmark_holder() : rg_(jkj::fp::detail::generate_correctly_seeded_mt19937_64()) {}

	// Digits samples for [0] ~ [max_digits - 1]
	std::array<std::vector<std::string>, max_digits> samples_;
	std::mt19937_64 rg_;
	std::unordered_map<std::string, Float(*)(std::string const&)> name_func_pairs_;
};

register_function_for_from_chars_limited_precision_benchmark::register_function_for_from_chars_limited_precision_benchmark(
	std::string_view name,
	float(*func_float)(std::string const&))
{
	benchmark_holder<float>::get_instance().register_function(name, func_float);
};

register_function_for_from_chars_limited_precision_benchmark::register_function_for_from_chars_limited_precision_benchmark(
	std::string_view name,
	double(*func_double)(std::string const&))
{
	benchmark_holder<double>::get_instance().register_function(name, func_double);
};

register_function_for_from_chars_limited_precision_benchmark::register_function_for_from_chars_limited_precision_benchmark(
	std::string_view name,
	float(*func_float)(std::string const&),
	double(*func_double)(std::string const&))
{
	benchmark_holder<float>::get_instance().register_function(name, func_float);
	benchmark_holder<double>::get_instance().register_function(name, func_double);
};

#define RUN_MATLAB
#ifdef RUN_MATLAB
#include <cstdlib>

void run_matlab() {
	std::system("matlab -nosplash -r \"cd('matlab');addpath('../../3rdparty/shaded_plots');"
		"plot_digit_benchmark(\'../results/from_chars_limited_precision_benchmark_binary32.csv\');"
		"plot_digit_benchmark(\'../results/from_chars_limited_precision_benchmark_binary64.csv\');\"");
}
#endif

template <class Float>
void benchmark_test(std::string_view float_name,
	std::size_t number_of_samples_per_digits,
	std::size_t number_of_iterations)
{
	auto& inst = benchmark_holder<Float>::get_instance();
	std::cout << "Generating random samples...\n";
	inst.prepare_samples(number_of_samples_per_digits);
	auto out = inst.run(number_of_iterations, float_name);

	std::cout << "Benchmarking done.\n" << "Now writing to files...\n";

	// Write benchmark results
	auto filename = std::string("results/from_chars_limited_precision_benchmark_");
	filename += float_name;
	filename += ".csv";
	std::ofstream out_file{ filename };
	out_file << "number_of_samples_per_digits," << number_of_samples_per_digits << std::endl;;
	out_file << "name,digits,sample,time\n";

	for (auto& name_result_pair : out) {
		for (unsigned int digits = 1; digits <= benchmark_holder<Float>::max_digits; ++digits) {
			for (auto const& data_time_pair : name_result_pair.second[digits - 1]) {
				out_file << "\"" << name_result_pair.first << "\"," << digits << "," <<
					data_time_pair.first << "," << data_time_pair.second << "\n";
			}
		}

	}
	out_file.close();
}

int main() {
	constexpr bool benchmark_float = true;
	constexpr std::size_t number_of_samples_per_digits_float = 100000;
	constexpr std::size_t number_of_benchmark_iterations_float = 300;

	constexpr bool benchmark_double = true;
	constexpr std::size_t number_of_samples_per_digits_double = 100000;
	constexpr std::size_t number_of_benchmark_iterations_double = 300;

	if constexpr (benchmark_float) {
		std::cout << "[Running limited-precision parsing benchmark for binary32...]\n";
		benchmark_test<float>("binary32",
			number_of_samples_per_digits_float,
			number_of_benchmark_iterations_float);
		std::cout << "Done.\n\n\n";
	}
	if constexpr (benchmark_double) {
		std::cout << "[Running limited-precision parsing  benchmark for binary64...]\n";
		benchmark_test<double>("binary64",
			number_of_samples_per_digits_double,
			number_of_benchmark_iterations_double);
		std::cout << "Done.\n\n\n";
	}

#ifdef RUN_MATLAB
	run_matlab();
#endif
}