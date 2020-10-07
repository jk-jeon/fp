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
#include "jkj/fp/detail/util.h"
#include "cache_write_helper.h"
#include "minmax_euclid.h"
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

template <jkj::fp::ieee754_format format, std::size_t array_size>
struct generated_cache {
	using cache_holder = jkj::fp::detail::dragonbox::cache_holder<format>;
	using cache_entry_type = typename cache_holder::cache_entry_type;

	int min_k;
	int max_k;
	std::vector<jkj::fp::detail::bigint_impl<array_size>> cache;
};

template <class GeneratedCache>
void write_to(std::ostream& out, GeneratedCache const& results) {
	out << std::dec;
	out << "static constexpr int min_k = " << results.min_k << ";\n";
	out << "static constexpr int max_k = " << results.max_k << ";\n\n";
	out << "static constexpr cache_entry_type cache[] = {\n\t";
	for (std::size_t i = 0; i < results.cache.size(); ++i) {
		print_as<typename GeneratedCache::cache_entry_type>(out, results.cache[i]);
		if (i != results.cache.size() - 1) {
			out << ",\n\t";
		}
	}

	out << "\n};";
}

template <jkj::fp::ieee754_format format>
auto generate_cache_impl()
{
	using namespace jkj::fp::detail;
	using ieee754_format_info = jkj::fp::ieee754_format_info<format>;
	using cache_holder = dragonbox::cache_holder<format>;
	constexpr auto cache_bits = cache_holder::cache_bits;
	constexpr auto significand_bits = ieee754_format_info::significand_bits;

	constexpr auto kappa = dragonbox::impl_base<format>::kappa;
	constexpr auto min_k = dragonbox::impl_base<format>::min_k;
	constexpr auto max_k = dragonbox::impl_base<format>::max_k;
	static_assert(max_k + min_k >= 0 && min_k <= 0 && max_k >= 0);

	constexpr auto min_e = ieee754_format_info::min_exponent - significand_bits;
	constexpr auto max_e = ieee754_format_info::max_exponent - significand_bits;

	// Find minimum/maximum values of b for each k
	struct b_range_t {
		int min = 0;
		int max = 0;
	};
	std::map<int, b_range_t> b_ranges;
	auto update_minmax = [&](int e, int k) {
		int b;
		if (k >= 0) {
			b = -e - k;
		}
		else {
			b = e + k;
		}

		auto itr = b_ranges.find(k);
		if (itr == b_ranges.end()) {
			b_ranges.insert({ k, { b, b } });
		}
		else {
			if (itr->second.min > b) {
				itr->second.min = b;
			}
			if (itr->second.max < b) {
				itr->second.max = b;
			}
		}
	};

	for (int e = min_e; e <= max_e; ++e) {
		int k;

		// Normal case
		k = kappa - log::floor_log10_pow2(e);
		update_minmax(e, k);

		// Round-to-nearest, shorter interval case
		k = -log::floor_log10_pow2_minus_log10_4_over_3(e);
		update_minmax(e, k);

		// Right-closed directed rounding, shorter interval case
		k = kappa - log::floor_log10_pow2(e - 1);
		update_minmax(e, k);
	}

	constexpr auto required_bits = std::size_t(2000);
	using bigint_t = bigint<required_bits>;
	auto max_f = bigint_t((std::uint64_t(1) << (significand_bits + 1)) - 1);

	auto power_of_5 = bigint_t{ 1 };

	// Computes cache
	generated_cache<format, bigint_t::array_size> results;
	results.min_k = min_k;
	results.max_k = max_k;

	int k = 0;
	for (; k <= max_k; ++k) {
		// For nonnegative k
		{
			auto l = -cache_bits + log::floor_log2_pow5(k) + 1;
			// The minimum value of b = -e - k
			auto itr = b_ranges.find(k);
			assert(itr != b_ranges.end());

			auto shift_result = multiplier_right_shift(power_of_5,
				itr->second.min, l, max_f);

			if (!shift_result) {
				std::stringstream stream;
				stream << "Error: " << cache_bits
					<< " bits are not sufficient! (k = " << k << ")";
				throw std::exception{ stream.str().c_str() };
			}
			else {
				results.cache.push_back(shift_result->resulting_number);
			}
		}

		// For negative k
		if (k != 0 && -k >= min_k) {
			auto u = cache_bits - log::floor_log2_pow5(-k) - 1;
			// The maximum value of b = e + k
			auto itr = b_ranges.find(-k);
			assert(itr != b_ranges.end());

			auto shift_result = reciprocal_left_shift(power_of_5,
				itr->second.max, u, max_f);

			if (!shift_result) {
				std::stringstream stream;
				stream << "Error: " << cache_bits
					<< " bits are not sufficient! (k = " << -k << ")";
				throw std::exception{ stream.str().c_str() };
			}
			else {
				results.cache.insert(results.cache.begin(), shift_result->resulting_number);
			}
		}

		if (k != max_k) {
			power_of_5.multiply_5();
		}
	}

	return results;
}

#include <fstream>

int main()
{
	std::ofstream out;
	bool success = true;
	std::cout << "[Generating cache table for Dragonbox...]\n";

	try {
		std::cout << "\nGenerating cache table for IEEE-754 binary32 format...\n";
		out.open("results/dragonbox_binary32_fast_cache.txt");
		write_to(out, generate_cache_impl<jkj::fp::ieee754_format::binary32>());
		out.close();

		std::cout << "\nGenerating cache table for IEEE-754 binary64 format...\n";
		out.open("results/dragonbox_binary64_fast_cache.txt");
		write_to(out, generate_cache_impl<jkj::fp::ieee754_format::binary64>());
		out.close();
	}
	catch (std::exception& ex) {
		std::cout << ex.what() << std::endl;
		success = false;
	}

	std::cout << std::endl;
	std::cout << "Done.\n\n\n";

	if (!success) {
		return -1;
	}
}
