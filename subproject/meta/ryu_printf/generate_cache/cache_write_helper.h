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

#include "jkj/fp/detail/log.h"
#include "jkj/fp/ryu_printf.h"
#include "minmax_euclid.h"
#include <cassert>
#include <iostream>
#include <iomanip>
#include <optional>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {
	template <class OutputType, std::size_t array_size>
	OutputType convert_to(jkj::fp::detail::bigint_impl<array_size> const& n) {
		static_assert(std::is_same_v<std::uint64_t,
			typename jkj::fp::detail::bigint_impl<array_size>::element_type>);

		// Makes sure there is no overflow
		if constexpr (std::is_same_v<OutputType, std::uint64_t>) {
			assert(n.leading_one_pos.element_pos == 0);
			return n.elements[0];
		}
		else if constexpr (std::is_same_v<OutputType, jkj::fp::detail::wuint::uint96>) {
			assert(n.leading_one_pos.element_pos < 1 ||
				(n.leading_one_pos.element_pos == 1 && n.leading_one_pos.bit_pos <= 32));
			return{ std::uint32_t(n.elements[1]),
				std::uint32_t(n.elements[0] >> 32),
				std::uint32_t(n.elements[0]) };
		}
		else if constexpr (std::is_same_v<OutputType, jkj::fp::detail::wuint::uint128>) {
			assert(n.leading_one_pos.element_pos <= 1);
			return{ n.elements[1], n.elements[0] };
		}
		else if constexpr (std::is_same_v<OutputType, jkj::fp::detail::wuint::uint192>) {
			assert(n.leading_one_pos.element_pos <= 2);
			return{ n.elements[2], n.elements[1], n.elements[0] };
		}
		else {
			static_assert(std::is_same_v<OutputType, jkj::fp::detail::wuint::uint256>);
			assert(n.leading_one_pos.element_pos <= 3);
			return{ n.elements[3], n.elements[2], n.elements[1], n.elements[0] };
		}
	}

	template <std::size_t array_size, class InputType>
	jkj::fp::detail::bigint_impl<array_size> convert_from(InputType const& n) {
		static_assert(std::is_same_v<std::uint64_t,
			typename jkj::fp::detail::bigint_impl<array_size>::element_type>);

		jkj::fp::detail::bigint_impl<array_size> ret;

		// Makes sure there is no overflow
		if constexpr (std::is_same_v<InputType, std::uint64_t>) {
			assert(array_size >= 1);
			ret = n;
		}
		else if constexpr (std::is_same_v<InputType, jkj::fp::detail::wuint::uint96>) {
			assert(array_size >= 2);
			ret = n.high();
			ret *= jkj::fp::detail::bigint_impl<array_size>::power_of_2(64);
			ret += (std::uint64_t(n.middle()) << 32) | std::uint64_t(n.low());
		}
		else if constexpr (std::is_same_v<InputType, jkj::fp::detail::wuint::uint128>) {
			assert(array_size >= 2);
			ret = n.high();
			ret *= jkj::fp::detail::bigint_impl<array_size>::power_of_2(64);
			ret += n.low();
		}
		else if constexpr (std::is_same_v<InputType, jkj::fp::detail::wuint::uint192>) {
			assert(array_size >= 3);
			ret = n.high();
			ret *= jkj::fp::detail::bigint_impl<array_size>::power_of_2(64);
			ret += n.middle();
			ret *= jkj::fp::detail::bigint_impl<array_size>::power_of_2(64);
			ret += n.low();
		}
		else {
			static_assert(std::is_same_v<InputType, jkj::fp::detail::wuint::uint256>);
			assert(array_size >= 4);
			ret = n.high();
			ret *= jkj::fp::detail::bigint_impl<array_size>::power_of_2(64);
			ret += n.middle_high();
			ret *= jkj::fp::detail::bigint_impl<array_size>::power_of_2(64);
			ret += n.middle_low();
			ret *= jkj::fp::detail::bigint_impl<array_size>::power_of_2(64);
			ret += n.low();
		}
		
		return ret;
	}

	template <class CacheEntryType>
	std::ostream& print_to(CacheEntryType const& x, std::ostream& out) {
		if constexpr (std::is_same_v<CacheEntryType, std::uint64_t>) {
			out << "0x" << std::hex << std::setw(16) << std::setfill('0') << x;
		}
		else if constexpr (std::is_same_v<CacheEntryType, jkj::fp::detail::wuint::uint96>) {
			out << "{ 0x" << std::hex << std::setw(8) << std::setfill('0') << x.high()
				<< ", 0x" << std::hex << std::setw(8) << std::setfill('0') << x.middle()
				<< ", 0x" << std::hex << std::setw(8) << std::setfill('0') << x.low() << " }";
		}
		else if constexpr (std::is_same_v<CacheEntryType, jkj::fp::detail::wuint::uint128>) {
			out << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << x.high()
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.low() << " }";
		}
		else if constexpr(std::is_same_v<CacheEntryType, jkj::fp::detail::wuint::uint192>) {
			out << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << x.high()
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.middle()
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.low() << " }";
		}
		else {
			static_assert(std::is_same_v<CacheEntryType, jkj::fp::detail::wuint::uint256>);
			out << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << x.high()
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.middle_high()
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.middle_low()
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.low() << " }";
		}
		return out;
	}

	template <class T>
	std::string_view name_of() {
		if constexpr (std::is_same_v<T, std::uint8_t>) {
			return "std::uint8_t";
		}
		else if constexpr (std::is_same_v<T, std::uint16_t>) {
			return "std::uint16_t";
		}
		else if constexpr (std::is_same_v<T, std::uint32_t>) {
			return "std::uint32_t";
		}
		else if constexpr (std::is_same_v<T, std::uint64_t>) {
			return "std::uint8_t";
		}
		else if constexpr (std::is_same_v<T, jkj::fp::detail::wuint::uint96>) {
			return "wuint::uint96";
		}
		else if constexpr (std::is_same_v<T, jkj::fp::detail::wuint::uint128>) {
			return "wuint::uint128";
		}
		else if constexpr (std::is_same_v<T, jkj::fp::detail::wuint::uint192>) {
			return "wuint::uint192";
		}
		else {
			static_assert(std::is_same_v<T, jkj::fp::detail::wuint::uint256>);
			return "wuint::uint256";
		}
	}

	template <jkj::fp::ieee754_format format>
	struct generated_cache {
		using cache_holder = jkj::fp::detail::ryu_printf::cache_holder<format>;
		using cache_entry_type = typename cache_holder::cache_entry_type;
		using index_info_type = typename cache_holder::index_info_type;

		int min_n;
		int max_n;
		std::vector<cache_entry_type> cache;
		std::vector<index_info_type> index_info;
	};

	template <class GeneratedCache>
	void write_to(std::ostream& out, GeneratedCache const& results) {
		out << "static constexpr int min_n = " << results.min_n << ";\n";
		out << "static constexpr int max_n = " << results.max_n << ";\n\n";
		out << "static constexpr cache_entry_type cache[] = {\n\t";
		for (std::size_t i = 0; i < results.cache.size(); ++i) {
			print_to(results.cache[i], out);
			if (i != results.cache.size() - 1) {
				out << ",\n\t";
			}
		}
		out << "\n};\n\nstatic constexpr index_info_type index_info[] = {\n\t" << std::dec;
		for (std::size_t i = 0; i < results.index_info.size(); ++i) {
			out << "{ " << std::setfill(' ') << std::setw(4)
				<< int(results.index_info[i].starting_index)
				<< ", " << std::setfill(' ') << std::setw(4)
				<< int(results.index_info[i].min_k)
				<< " }";

			if (i != results.index_info.size() - 1) {
				if (i % 4 == 3) {
					out << ",\n\t";
				}
				else {
					out << ", ";
				}
			}
		}

		out << "\n};";
	}
}