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

#ifndef JKJ_HEADER_FP_TO_CHARS_COMMON
#define JKJ_HEADER_FP_TO_CHARS_COMMON

#include "../detail/util.h"

namespace jkj::fp {
	namespace detail {
		static constexpr char radix_100_table[] = {
			'0', '0', '0', '1', '0', '2', '0', '3', '0', '4',
			'0', '5', '0', '6', '0', '7', '0', '8', '0', '9',
			'1', '0', '1', '1', '1', '2', '1', '3', '1', '4',
			'1', '5', '1', '6', '1', '7', '1', '8', '1', '9',
			'2', '0', '2', '1', '2', '2', '2', '3', '2', '4',
			'2', '5', '2', '6', '2', '7', '2', '8', '2', '9',
			'3', '0', '3', '1', '3', '2', '3', '3', '3', '4',
			'3', '5', '3', '6', '3', '7', '3', '8', '3', '9',
			'4', '0', '4', '1', '4', '2', '4', '3', '4', '4',
			'4', '5', '4', '6', '4', '7', '4', '8', '4', '9',
			'5', '0', '5', '1', '5', '2', '5', '3', '5', '4',
			'5', '5', '5', '6', '5', '7', '5', '8', '5', '9',
			'6', '0', '6', '1', '6', '2', '6', '3', '6', '4',
			'6', '5', '6', '6', '6', '7', '6', '8', '6', '9',
			'7', '0', '7', '1', '7', '2', '7', '3', '7', '4',
			'7', '5', '7', '6', '7', '7', '7', '8', '7', '9',
			'8', '0', '8', '1', '8', '2', '8', '3', '8', '4',
			'8', '5', '8', '6', '8', '7', '8', '8', '8', '9',
			'9', '0', '9', '1', '9', '2', '9', '3', '9', '4',
			'9', '5', '9', '6', '9', '7', '9', '8', '9', '9'
		};

		template <int max_length, class UInt>
		static constexpr std::uint32_t decimal_length(UInt const x) {
			if constexpr (max_length == 9) {
				assert(x < 1000000000);
				if (x >= 100000000) { return 9; }
				if (x >= 10000000) { return 8; }
				if (x >= 1000000) { return 7; }
				if (x >= 100000) { return 6; }
				if (x >= 10000) { return 5; }
				if (x >= 1000) { return 4; }
				if (x >= 100) { return 3; }
				if (x >= 10) { return 2; }
				return 1;
			}
			else {
				static_assert(max_length == 17);
				assert(x < 100000000000000000L);
				if (x >= 10000000000000000L) { return 17; }
				if (x >= 1000000000000000L) { return 16; }
				if (x >= 100000000000000L) { return 15; }
				if (x >= 10000000000000L) { return 14; }
				if (x >= 1000000000000L) { return 13; }
				if (x >= 100000000000L) { return 12; }
				if (x >= 10000000000L) { return 11; }
				if (x >= 1000000000L) { return 10; }
				if (x >= 100000000L) { return 9; }
				if (x >= 10000000L) { return 8; }
				if (x >= 1000000L) { return 7; }
				if (x >= 100000L) { return 6; }
				if (x >= 10000L) { return 5; }
				if (x >= 1000L) { return 4; }
				if (x >= 100L) { return 3; }
				if (x >= 10L) { return 2; }
				return 1;
			}
		}
	}
}

#endif