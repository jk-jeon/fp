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

#include "../detail/log.h"
#include "../detail/util.h"
#include "../detail/macros.h"

namespace jkj::fp {
	namespace detail {
		extern char const radix_100_table[200];

		template <int max_length, class UInt>
		JKJ_FORCEINLINE constexpr std::uint32_t decimal_length(UInt const x) noexcept {
			static_assert(max_length > 0);
			static_assert(max_length <= log::floor_log10_pow2(value_bits<UInt>));
			constexpr auto threshold = compute_power<max_length - 1>(UInt(10));
			assert(x < compute_power<max_length>(UInt(10)));

			if constexpr (max_length == 1) {
				return 1;
			}
			else {
				if (x >= threshold) {
					return max_length;
				}
				else {
					return decimal_length<max_length - 1>(x);
				}
			}
		}

		char* print_number(char* buffer, std::uint32_t number, int length) noexcept;
		char* print_nine_digits(char* buffer, std::uint32_t number) noexcept;
		char* print_zeros(char* buffer, int length) noexcept;
		char* print_nines(char* buffer, int length) noexcept;
	}
}

#endif