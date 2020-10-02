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

#include "minmax_euclid.h"
#include "jkj/fp/detail/wuint.h"
#include <cassert>
#include <iostream>
#include <iomanip>
#include <string_view>
#include <type_traits>

namespace {
	template <class CacheEntryType, std::size_t array_size>
	std::ostream& print_as(std::ostream& out, jkj::fp::detail::bigint_impl<array_size> const& x) {
		if constexpr (std::is_same_v<CacheEntryType, std::uint64_t>) {
			assert(x.leading_one_pos.element_pos == 0);
			out << "0x" << std::hex << std::setw(16) << std::setfill('0') << x.elements[0];
		}
		else if constexpr (std::is_same_v<CacheEntryType, jkj::fp::detail::wuint::uint96> ||
			std::is_same_v<CacheEntryType, std::uint32_t[3]>)
		{
			assert(x.leading_one_pos.element_pos < 1 ||
				(x.leading_one_pos.element_pos == 1 && x.leading_one_pos.bit_pos <= 32));
			out << "{ 0x" << std::hex << std::setw(8) << std::setfill('0') << std::uint32_t(x.elements[1])
				<< ", 0x" << std::hex << std::setw(8) << std::setfill('0') << std::uint32_t(x.elements[0] >> 32)
				<< ", 0x" << std::hex << std::setw(8) << std::setfill('0') << std::uint32_t(x.elements[0]) << " }";
		}
		else if constexpr (std::is_same_v<CacheEntryType, jkj::fp::detail::wuint::uint128> ||
			std::is_same_v<CacheEntryType, std::uint64_t[2]>)
		{
			assert(x.leading_one_pos.element_pos <= 1);
			out << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << x.elements[1]
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.elements[0] << " }";
		}
		else if constexpr(std::is_same_v<CacheEntryType, jkj::fp::detail::wuint::uint192> || 
			std::is_same_v<CacheEntryType, std::uint64_t[3]>)
		{
			assert(x.leading_one_pos.element_pos <= 2);
			out << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << x.elements[2]
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.elements[1]
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.elements[0] << " }";
		}
		else {
			static_assert(std::is_same_v<CacheEntryType, jkj::fp::detail::wuint::uint256> ||
				std::is_same_v<CacheEntryType, std::uint64_t[4]>);

			assert(x.leading_one_pos.element_pos <= 3);
			out << "{ 0x" << std::hex << std::setw(16) << std::setfill('0') << x.elements[3]
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.elements[2]
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.elements[1]
				<< ", 0x" << std::hex << std::setw(16) << std::setfill('0') << x.elements[0] << " }";
		}
		return out;
	}
}