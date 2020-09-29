#include "jkj/fp/to_chars/fixed_precision.h"
#include "random_float.h"

namespace jkj::fp {
	namespace detail {
		extern void verify_log_computation();

		namespace ryu_printf {
			extern void generate_cache();
		}
	}
}

#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <string_view>
#include <stdexcept>
#include <memory>
#include <vector>

#include "ryu/ryu.h"

int main()
{
	using namespace jkj::fp::detail;
	//ryu_printf::generate_cache();
	//verify_log_computation();

#if 0
	while (true) {
		double x;
		int precision;
		while (!(std::cin >> x))
		{
			std::cin.clear();
			while (std::cin.get() != '\n')
				continue;
			std::cout << "enter a new input: ";
		}
		while (!(std::cin >> precision))
		{
			std::cin.clear();
			while (std::cin.get() != '\n')
				continue;
			std::cout << "enter a new input: ";
		}

		char buffer[2048];
		std::string_view str{ buffer,
			std::size_t(jkj::fp::to_chars_fixed_precision_scientific(x, buffer, precision) - buffer) };

		std::cout << str << std::endl;
		std::cout << std::scientific << std::setprecision(precision) << x << std::endl;
	}
#else
	// Benchmark
	constexpr std::size_t number_of_samples = 100000;
	auto running_time = std::chrono::seconds(5);
	auto buffer = std::make_unique<char[]>(10000);

	// Generate random inputs
	std::cout << "Generating samples...\n";
	auto rg = jkj::fp::detail::generate_correctly_seeded_mt19937_64();
	std::vector<double> samples(number_of_samples);
	for (std::size_t i = 0; i < number_of_samples; ++i) {
		samples[i] = jkj::fp::detail::uniformly_randomly_generate_general_float<double>(rg);
	}
	std::cout << "Done.\n\n\n";

	while (true) {
		int precision;
		std::string str;
		while (true) {
			std::cout << "Input precision: ";
			std::getline(std::cin, str);
			try {
				precision = std::stoi(str);
				break;
			}
			catch (std::out_of_range const&) {
				std::cout << "Invalid input.\n";
			}
		}

		// Run benchmark
		std::size_t sample_idx = 0;
		std::size_t iteration_count = 0;
		auto from = std::chrono::steady_clock::now();
		auto to = from + running_time;
		std::chrono::time_point now = std::chrono::steady_clock::now();
		while (now <= to) {
			d2exp_buffered_n(samples[sample_idx], precision, buffer.get());
			if (++sample_idx == number_of_samples) {
				sample_idx = 0;
			}
			++iteration_count;
			now = std::chrono::steady_clock::now();
		}
		auto elapsed = double(std::chrono::duration_cast<std::chrono::nanoseconds>(now - from).count());
		std::cout << "Ryu: " << (elapsed / double(iteration_count)) << "ns per iteration\n";

		sample_idx = 0;
		iteration_count = 0;
		from = std::chrono::steady_clock::now();
		to = from + running_time;
		now = std::chrono::steady_clock::now();
		while (now <= to) {
			jkj::fp::to_chars_fixed_precision_scientific(samples[sample_idx], buffer.get(), precision);
			if (++sample_idx == number_of_samples) {
				sample_idx = 0;
			}
			++iteration_count;
			now = std::chrono::steady_clock::now();
		}
		elapsed = double(std::chrono::duration_cast<std::chrono::nanoseconds>(now - from).count());
		std::cout << " fp: " << (elapsed / double(iteration_count)) << "ns per iteration\n";
	}

#endif
}