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

#ifndef JKJ_HEADER_FP_DIV
#define JKJ_HEADER_FP_DIV

#include "bits.h"
#include "util.h"
#include <cassert>

namespace jkj::fp {
	namespace detail {
		namespace div {
			template <class UInt, UInt a>
			constexpr UInt modular_inverse(int bit_width = int(value_bits<UInt>)) noexcept {
				// By Euler's theorem, a^phi(2^n) == 1 (mod 2^n),
				// where phi(2^n) = 2^(n-1), so the modular inverse of a is
				// a^(2^(n-1) - 1) = a^(1 + 2 + 2^2 + ... + 2^(n-2)).
				std::common_type_t<UInt, unsigned int> mod_inverse = 1;
				for (int i = 1; i < bit_width; ++i) {
					mod_inverse = mod_inverse * mod_inverse * a;
				}
				if (bit_width < value_bits<UInt>) {
					auto mask = UInt((UInt(1) << bit_width) - 1);
					return UInt(mod_inverse & mask);
				}
				else {
					return UInt(mod_inverse);
				}
			}

			template <class UInt, UInt a, int N>
			struct table_t {
				static_assert(std::is_unsigned_v<UInt>);
				static_assert(a % 2 != 0);
				static_assert(N > 0);

				static constexpr int size = N;
				struct entry_t {
					UInt mod_inv;
					UInt max_quotient;
				} entries[N];

				constexpr entry_t const& operator[](std::size_t idx) const noexcept {
					assert(idx < N);
					return entries[idx];
				}
				constexpr entry_t& operator[](std::size_t idx) noexcept {
					assert(idx < N);
					return entries[idx];
				}
			};

			template <class UInt, UInt a, int N>
			struct table_holder {
				static constexpr table_t<UInt, a, N> table = [] {
					constexpr auto mod_inverse = modular_inverse<UInt, a>();
					table_t<UInt, a, N> table{};
					std::common_type_t<UInt, unsigned int> pow_of_mod_inverse = 1;
					UInt pow_of_a = 1;
					for (int i = 0; i < N; ++i) {
						table[i].mod_inv = UInt(pow_of_mod_inverse);
						table[i].max_quotient = UInt(std::numeric_limits<UInt>::max() / pow_of_a);

						pow_of_mod_inverse *= mod_inverse;
						pow_of_a *= a;
					}

					return table;
				}();
			};

			template <std::size_t table_size, class UInt>
			constexpr bool divisible_by_power_of_5(UInt x, unsigned int exp) noexcept {
				auto const& table = table_holder<UInt, 5, table_size>::table;
				assert(exp < (unsigned int)(table.size));
				return x * table[exp].mod_inv <= table[exp].max_quotient;
			}

			template <class UInt>
			constexpr bool divisible_by_power_of_2(UInt x, unsigned int exp) noexcept {
				assert(exp >= 1);
				assert(x != 0);
#if JKJ_HAS_COUNTR_ZERO_INTRINSIC
				return bits::countr_zero(x) >= int(exp);
#else
				if (exp >= int(value_bits<UInt>)) {
					return false;
				}
				return x == ((x >> exp) << exp);
#endif
			}
		}
	}
}

#include "undef_macros.h"
#endif