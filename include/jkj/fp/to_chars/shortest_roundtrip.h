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

#ifndef JKJ_HEADER_FP_TO_CHARS_SHORTEST_ROUNDTRIP
#define JKJ_HEADER_FP_TO_CHARS_SHORTEST_ROUNDTRIP

#include "jkj/fp/dragonbox.h"

namespace jkj::fp {
	namespace detail {
		char* to_chars_shortest_scientific_n_impl(unsigned_fp_t<float> v, char* buffer);
		char* to_chars_shortest_scientific_n_impl(unsigned_fp_t<double> v, char* buffer);
	}

	// Returns the next-to-end position.
	template <class Float, class... Policies>
	char* to_chars_shortest_scientific_n(Float x, char* buffer, Policies&&... policies)
	{
		using namespace jkj::fp::detail::policy;
		using policy_holder_t = decltype(make_policy_holder(
			make_default_list(
				make_default<policy_kind::trailing_zero>(policy::trailing_zero::remove),
				make_default<policy_kind::binary_rounding>(policy::binary_rounding::nearest_to_even),
				make_default<policy_kind::decimal_rounding>(policy::decimal_rounding::to_even),
				make_default<policy_kind::cache>(policy::cache::fast)),
			std::forward<Policies>(policies)...));

		static_assert(!policy_holder_t::report_trailing_zeros,
			"jkj::fp::policy::trailing_zero::report is not valid for to_chars & to_chars_n");

		using ieee754_format_info = ieee754_format_info<ieee754_traits<Float>::format>;

		auto br = ieee754_bits(x);
		if (br.is_finite()) {
			if (br.is_negative()) {
				*buffer = '-';
				++buffer;
			}
			if (br.is_nonzero()) {
				return detail::to_chars_shortest_scientific_n_impl(dragonbox(x,
					policy::sign::ignore,
					std::forward<Policies>(policies)...),
					buffer);
			}
			else {
				std::memcpy(buffer, "0E0", 3);
				return buffer + 3;
			}
		}
		else {
			if ((br.u << (ieee754_format_info::exponent_bits + 1)) != 0)
			{
				std::memcpy(buffer, "NaN", 3);
				return buffer + 3;
			}
			else {
				if (br.is_negative()) {
					*buffer = '-';
					++buffer;
				}
				std::memcpy(buffer, "Infinity", 8);
				return buffer + 8;
			}
		}
	}

	// Null-terminates and bypass the return value of fp_to_chars_n.
	template <class Float, class... Policies>
	char* to_chars_shortest_scientific(Float x, char* buffer, Policies... policies)
	{
		auto ptr = to_chars_shortest_scientific_n(x, buffer, policies...);
		*ptr = '\0';
		return ptr;
	}
}

#endif

